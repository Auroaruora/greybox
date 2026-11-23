import bpy, os, json, math, re, shutil
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
space = {"__name__": "room", "__file__": os.path.join(HERE, "edit_room.py"), "ROOT": ROOT}
exec(compile(open(space["__file__"]).read(), space["__file__"], "exec"), space)
S = space["S"]
OUT = os.path.join(os.path.dirname(ROOT), "assets")
SCENE = "room"
DEFAULTS = json.load(open(os.path.join(HERE, "layout_defaults.json")))
Y_UP = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
LIGHT_TYPES = {'SUN': "directional", 'POINT': "point", 'SPOT': "spot"}
LIGHT_SCALE = {"point": 1 / (4 * math.pi ** 2), "spot": 1 / (4 * math.pi ** 2), "directional": 1 / math.pi}


def position(obj):
    return list(Y_UP @ obj.matrix_world.translation)


def direction(obj, axis):
    return list(Y_UP.to_3x3() @ (obj.matrix_world.to_quaternion() @ Vector(axis)))


def camera(scene, obj):
    corner = obj.data.view_frame(scene=scene)[0]
    return {"position": position(obj), "look": direction(obj, (0, 0, -1)), "up": direction(obj, (0, 1, 0)),
            "heightAngle": math.degrees(2 * math.atan(abs(corner.y / corner.z)))}


def principled(material):
    if material is None or not material.use_nodes:
        return None
    return next((n for n in material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)


def emission(material):
    node = principled(material)
    if node is None:
        return None
    strength = node.inputs["Emission Strength"].default_value
    return [c * strength for c in node.inputs["Emission Color"].default_value[:3]] if strength > 0 else None


def light(obj):
    data = obj.data
    kind = LIGHT_TYPES[data.type]
    entry = {"name": obj["room_light"], **DEFAULTS["light"], "type": kind,
             "color": [c * data.energy * LIGHT_SCALE[kind] for c in data.color],
             "position": position(obj), "direction": direction(obj, (0, 0, -1))}
    if data.type == 'SPOT':
        half = data.spot_size / 2
        inner = math.acos(math.cos(half) + (1 - math.cos(half)) * data.spot_blend)
        entry["angle"] = math.degrees(half)
        entry["penumbra"] = math.degrees(half - inner)
    spec = S["data"]["lighting"]["lights"].get(obj["room_light"], {})
    if spec.get("attach") and spec.get("glowPart"):
        entry["emitter"] = "%s_%s" % (spec["attach"], spec["glowPart"])
    return entry


def lights():
    return [light(o) for o in space["light_objects"]()
            if o.type == 'LIGHT' and o.data.type in LIGHT_TYPES and o.data.energy > 0 and not o.hide_render]


def world_colour():
    world = bpy.context.scene.world
    node = next(n for n in world.node_tree.nodes if n.type == 'BACKGROUND')
    return [c * node.inputs[1].default_value for c in node.inputs[0].default_value[:3]]


def write_mesh(mesh, path):
    mesh.calc_loop_triangles()
    turn = Y_UP.to_3x3()
    uvs = mesh.uv_layers.active.data if mesh.uv_layers.active else None
    lines = []
    for index, triangle in enumerate(mesh.loop_triangles):
        smooth = mesh.polygons[triangle.polygon_index].use_smooth
        for loop in triangle.loops:
            point = turn @ mesh.vertices[mesh.loops[loop].vertex_index].co
            normal = turn @ (Vector(mesh.corner_normals[loop].vector) if smooth else triangle.normal)
            uv = uvs[loop].uv if uvs else (0.0, 0.0)
            lines.append("v %.6f %.6f %.6f" % tuple(point))
            lines.append("vt %.6f %.6f" % tuple(uv))
            lines.append("vn %.6f %.6f %.6f" % tuple(normal))
        first = index * 3 + 1
        lines.append("f " + " ".join("%d/%d/%d" % (i, i, i) for i in range(first, first + 3)))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


def mesh_path(asset, shape, part):
    return "meshes/%s/%s_%s.obj" % (asset, shape, part)


def texture(material, asset, written):
    node = principled(material)
    if node is None or not node.inputs["Base Color"].is_linked:
        return None
    source_node = node.inputs["Base Color"].links[0].from_node
    if source_node.type != 'TEX_IMAGE' or source_node.image is None:
        return None
    image = source_node.image
    source = os.path.normpath(bpy.path.abspath(image.filepath, library=image.library))
    library = material.library or (material.original.library if hasattr(material, "original") else None)
    shared = library and os.path.abspath(bpy.path.abspath(library.filepath)) == os.path.abspath(space["MATERIALS"])
    relative = "textures/%s/%s" % ("materials" if shared else asset, os.path.basename(source))
    if relative not in written:
        if not os.path.isfile(source):
            print("EXPORT missing image %s in %s" % (source, material.name))
            return None
        target = os.path.join(OUT, relative)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copyfile(source, target)
        written.add(relative)
    return relative


def material_names(spec):
    for value in spec.values():
        if isinstance(value, list):
            yield from value
        elif value:
            yield value


def export_library(textures_written):
    meshes = 0
    for asset in space["assets"]():
        doc = space["doc"](asset)
        names = ["%s_%s" % (shape, part) for shape in doc["shapes"] for part in doc["parts"]]
        for name, mesh in space["linked_meshes"](asset, names).items():
            shape, part = name.split("_", 1)
            write_mesh(mesh, os.path.join(OUT, mesh_path(asset, shape, part)))
            meshes += 1
        for spec in doc["colors"].values():
            for name in material_names(spec):
                material = space["linked_material"](asset, name)
                if material:
                    texture(material, asset, textures_written)
    print("EXPORT library  %d meshes, %d textures" % (meshes, len(textures_written)))


def model(obj, asset, path, written):
    material = obj.material_slots[0].material if obj.material_slots else None
    result = {"mesh": path, **DEFAULTS["material"]}
    node = principled(material)
    if node is not None:
        result["diffuse"] = list(node.inputs["Base Color"].default_value)[:3]
    glow = emission(material)
    if glow:
        result["emissive"] = glow
    image = texture(material, asset, written) if material else None
    if image:
        result["textureFile"] = image
        result["blend"] = 1.0
    return result


def parts_in_room():
    out = []
    for holder in space["holders"]():
        if holder.users_collection and holder.users_collection[0].name == "Room":
            for child in holder.children:
                if child.get("room_part"):
                    out.append((holder, child))
    return out


def compact(value):
    text = json.dumps(value, indent=2)
    text = re.sub(r"\[[^\[\]{}]*\]", lambda m: re.sub(r"\s+", " ", m.group(0)).replace("[ ", "[").replace(" ]", "]"), text)
    return re.sub(r"\[\s*(?:\[[^\[\]{}]*\],?\s*)+\]", lambda m: re.sub(r"\s+", " ", m.group(0)).replace("[ ", "[").replace(" ]", "]"), text) + "\n"


def clear_output():
    for folder in ("meshes", "textures", os.path.join("scenes", SCENE)):
        shutil.rmtree(os.path.join(OUT, folder), ignore_errors=True)


def export_scene(preset, textures_written):
    bpy.ops.wm.read_homefile(use_empty=True)
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 1920, 1080
    S["state"] = None
    S["camera"] = None
    space["load_preset"](preset)
    S["state"] = next(iter(S["data"]["lighting"]["states"]))
    space["build_all"]()
    bpy.context.view_layer.update()
    name = SCENE + "_" + preset
    folder = os.path.join(OUT, "scenes", SCENE)

    models, objects, owners = {}, [], {}
    for holder, part in parts_in_room():
        asset = holder["room_asset"]
        key = part.name
        models[key] = model(part, asset, mesh_path(asset, holder["room_shape"], part["room_part"]), textures_written)
        owners[key] = part
        matrix = Y_UP @ part.matrix_world @ Y_UP.inverted()
        objects.append({"model": key, "matrix": [list(row) for row in matrix]})

    first_camera = next(iter(S["data"]["cameras"]))
    layout = {"globalData": {**DEFAULTS["globalData"], "ambientLight": world_colour()},
              "cameraData": camera(scene, bpy.data.objects[first_camera]),
              "lights": lights(), "models": models, "objects": objects}
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, name + ".json"), "w") as f:
        f.write(compact(layout))
    viewpoints = {n: camera(scene, bpy.data.objects[n]) for n in sorted(S["data"]["cameras"])}
    with open(os.path.join(folder, name + "_viewpoints.json"), "w") as f:
        f.write(compact(viewpoints))

    groups = []
    for state in S["data"]["lighting"]["states"]:
        S["state"] = state
        space["apply_state"]()
        glow = {key: emission(obj.material_slots[0].material) for key, obj in owners.items() if obj.material_slots}
        groups.append({"name": state, "ambient": world_colour(), "lights": lights(),
                       "emissive": {key: colour for key, colour in glow.items() if colour}})
    with open(os.path.join(folder, name + "_lighting.json"), "w") as f:
        f.write(compact(groups))
    print("EXPORT scenes/%s/%s.json  %d models, %d lights, %d states, %d viewpoints" % (SCENE, name, len(models), len(layout["lights"]), len(groups), len(viewpoints)))


def main():
    bpy.context.preferences.filepaths.save_version = 0
    space["register"]()
    clear_output()
    textures_written = set()
    export_library(textures_written)
    for preset in space["files_of"]("preset"):
        export_scene(preset, textures_written)


if __name__ == "__main__":
    main()
