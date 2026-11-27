import bpy, os, sys, json, re

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hub

DEFAULT_NAME = re.compile(r"(\.\d{3}$)|^(Cube|Sphere|Cylinder|Plane|Cone|Torus|Circle|Mesh|Object|Empty|Camera|Light|Collection)(\b|_|$)")
SURFACE_MATERIAL = re.compile(r"^[a-z]+Surface\d+[A-Z]")
MAX_SIZE = 6.0
TOLERANCE = 1e-4


def get_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not argv:
        print("usage: Blender -b models/<id>/model.blend --python tools/agent/check.py -- <id> [variant ...]")
        sys.exit(2)
    return argv[0], argv[1:]


def is_identity(matrix):
    return all(abs(matrix[r][c] - (1.0 if r == c else 0.0)) <= TOLERANCE for r in range(4) for c in range(4))


def check_variant(asset, doc, palette, variant, errors, warnings):
    collection = bpy.data.collections.get(variant)
    if collection is None:
        errors.append("%s: no collection named %s" % (variant, variant))
        return
    objects = list(collection.all_objects)
    if not objects:
        errors.append("%s: collection is empty" % variant)
    own = doc.get("materials", {})
    textured = {name for name, m in own.items() if not m.get("color", "#").startswith("#")}
    sets = [doc["colors"][s] for s in hub.colour_sets_for(doc, variant)]
    if not sets:
        errors.append("%s: no colour set covers this variant" % variant)
    seen = set()
    for o in objects:
        where = "%s: %s" % (variant, o.name)
        if o.type != "MESH":
            errors.append("%s is a %s, only meshes belong in a variant" % (where, o.type.lower()))
            continue
        prefix, _, part = o.name.partition("_")
        if prefix != variant or part not in doc["parts"]:
            errors.append("%s should be named %s_<part> with a part from %s" % (where, variant, doc["parts"]))
        if part in seen:
            errors.append("%s: part %s appears twice" % (where, part))
        seen.add(part)
        if o.data.name != o.name:
            errors.append("%s: mesh is named %s, should match the object" % (where, o.data.name))
        if not is_identity(o.matrix_world):
            errors.append("%s: object has a location, rotation or scale; apply it so the origin is the asset's origin" % where)
        if not o.data.polygons:
            errors.append("%s: mesh has no faces" % where)
        if not o.material_slots:
            errors.append("%s: no material slot" % where)
        for i, slot in enumerate(o.material_slots):
            name = slot.material.name if slot.material else ""
            if name not in palette and name not in own:
                errors.append("%s: slot %d material '%s' is not in the palette" % (where, i + 1, name))
            if SURFACE_MATERIAL.match(name):
                errors.append("%s: slot %d uses surface texture %s; textures belong in surfaces, not on the mesh" % (where, i + 1, name))
        for spec in sets:
            value = spec.get(part, spec.get("*"))
            names = value if isinstance(value, list) else [value]
            if any(n in textured for n in names) and "UVMap" not in o.data.uv_layers:
                errors.append("%s: a colour set gives it a texture but it has no UV map named UVMap" % where)
                break
        size = max(o.dimensions)
        if size > MAX_SIZE:
            warnings.append("%s: %.2f m across, check the scale is metres" % (where, size))
        if 0 < size < 0.005:
            warnings.append("%s: under 5 mm across" % where)


def main():
    asset, variants = get_args()
    doc = hub.get_doc(asset)
    errors, warnings = [], []
    if doc is None:
        errors.append("no asset.json")
    else:
        palette = hub.get_palette()
        variants = variants or doc["shapes"]
        for variant in variants:
            if variant not in doc["shapes"]:
                errors.append("%s is not in asset.json shapes" % variant)
            check_variant(asset, doc, palette, variant, errors, warnings)
        for o in bpy.data.objects:
            if DEFAULT_NAME.search(o.name) or DEFAULT_NAME.search(o.data.name if o.data else ""):
                errors.append("%s: default or numbered name" % o.name)
            if not any(c.name in doc["shapes"] for c in o.users_collection):
                errors.append("%s: not in a variant collection; remove leftovers" % o.name)
        for name, material in doc.get("materials", {}).items():
            found = bpy.data.materials.get(name)
            if found is None:
                errors.append("material %s is listed in asset.json but missing from the .blend" % name)
                continue
            if found.users == 0:
                errors.append("material %s has no users; turn on use_fake_user so it is saved" % name)
            path = material.get("color", "")
            if not path.startswith("#") and not os.path.isfile(os.path.join(hub.data_folder(asset), path)):
                errors.append("material %s points to missing file %s" % (name, path))
        for c in bpy.data.collections:
            if c.name not in doc["shapes"]:
                warnings.append("collection %s is not a variant" % c.name)
        for name in doc["parts"]:
            if not re.match(r"^[a-z][A-Za-z0-9]*$", name):
                errors.append("part name %s should be camelCase" % name)
    if os.path.isfile(hub.blend_path(asset) + "1"):
        warnings.append("delete %s.blend1" % asset)
    result = {"asset": asset, "ok": not errors, "errors": errors, "warnings": warnings}
    print("CHECK " + json.dumps(result, ensure_ascii=False))
    sys.exit(0 if not errors else 1)


main()
