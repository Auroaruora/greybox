import bpy, os, sys, json, math, shutil
from mathutils import Vector

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hub

SIZE = 256
VIEW = Vector((0.75, -1.0, 0.65)).normalized()
FOV = math.radians(30)


def get_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    names = [a for a in argv if not a.startswith("--")]
    if not names:
        names = sorted(a for a in os.listdir(hub.ASSETS) if os.path.isfile(hub.blend_path(a)) and os.path.isfile(hub.doc_path(a)))
    return names, "--force" in argv


def get_stamp(asset):
    files = [hub.blend_path(asset), hub.doc_path(asset), hub.PALETTE, os.path.abspath(__file__)]
    textures = os.path.join(hub.asset_folder(asset), "textures")
    if os.path.isdir(textures):
        files += [os.path.join(textures, f) for f in sorted(os.listdir(textures))]
    return {os.path.relpath(f, hub.ROOT): os.path.getmtime(f) for f in files if os.path.isfile(f)}


def is_fresh(asset, stamp):
    path = os.path.join(hub.CACHE, asset, "stamp.json")
    return os.path.isfile(path) and hub.read_json(path) == stamp


def to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_colour(text):
    text = text.lstrip("#")
    return [to_linear(int(text[i:i + 2], 16) / 255) for i in (0, 2, 4)]


def texture_colour(path):
    image = bpy.data.images.load(path, check_existing=True)
    pixels = list(image.pixels)
    count = max(1, len(pixels) // 4)
    rgb = [sum(pixels[i::4]) / count for i in range(3)]
    if image.colorspace_settings.name == "sRGB" and not image.is_float:
        rgb = [to_linear(c) for c in rgb]
    return rgb


def material_colour(asset, doc, palette, name, cache):
    if name not in cache:
        own = doc.get("materials", {}).get(name)
        if own and not own.get("color", "#").startswith("#"):
            path = os.path.join(hub.asset_folder(asset), own["color"])
            cache[name] = texture_colour(path) if os.path.isfile(path) else [0.5, 0.5, 0.5]
        elif own:
            cache[name] = hex_colour(own["color"])
        elif name in palette:
            cache[name] = hex_colour(palette[name]["color"])
        else:
            cache[name] = [0.5, 0.5, 0.5]
    return cache[name]


def show_everything():
    def walk(layer):
        layer.exclude = False
        layer.hide_viewport = False
        for child in layer.children:
            walk(child)
    walk(bpy.context.view_layer.layer_collection)
    for o in bpy.data.objects:
        o.hide_set(False)
        o.hide_viewport = False
        o.hide_select = False


def variant_objects(variant):
    collection = bpy.data.collections.get(variant)
    if collection is None:
        return []
    return [o for o in collection.all_objects if o.type == "MESH"]


def slot_material(index, name, made):
    key = "slot%d:%s" % (index, name)
    if key not in made:
        made[key] = bpy.data.materials.new(key)
    return made[key]


def export_glb(objects, path, made):
    for o in objects:
        mesh = o.data
        if not mesh.materials:
            mesh.materials.append(slot_material(0, "", made))
        for i, m in enumerate(mesh.materials):
            if m is None or not m.name.startswith("slot"):
                mesh.materials[i] = slot_material(i, m.name if m else "", made)
    bpy.ops.object.select_all(action="DESELECT")
    for o in objects:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_apply=True,
                              export_yup=True, export_texcoords=True, export_normals=True, export_materials="EXPORT")


def set_engine(scene):
    for engine in ("BLENDER_WORKBENCH", "WORKBENCH"):
        try:
            scene.render.engine = engine
            return
        except TypeError:
            continue


def camera_for(scene, objects):
    corners = [o.matrix_world @ Vector(c) for o in objects for c in o.bound_box]
    low = Vector([min(c[i] for c in corners) for i in range(3)])
    high = Vector([max(c[i] for c in corners) for i in range(3)])
    centre = (low + high) / 2
    radius = max(0.01, max((c - centre).length for c in corners))
    data = bpy.data.cameras.new("ReviewCamera")
    data.angle = FOV
    data.clip_start = radius * 0.01
    data.clip_end = radius * 20
    camera = bpy.data.objects.new("ReviewCamera", data)
    scene.collection.objects.link(camera)
    camera.location = centre + VIEW * (radius / math.sin(FOV / 2) * 1.05)
    camera.rotation_euler = (-VIEW).to_track_quat("-Z", "Y").to_euler()
    scene.camera = camera
    return camera


def render_thumb(asset, doc, palette, variant, objects, path, colours):
    scene = bpy.context.scene
    set_engine(scene)
    scene.render.resolution_x = scene.render.resolution_y = SIZE
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = "MATERIAL"
    shading.show_cavity = False
    made = {}
    for o in bpy.data.objects:
        o.hide_render = o not in objects
    for o in objects:
        for slot in o.material_slots:
            name = slot.material.name.split(":", 1)[-1] if slot.material else ""
            if name not in made:
                m = bpy.data.materials.new("thumb:" + name)
                m.diffuse_color = material_colour(asset, doc, palette, name, colours) + [1.0]
                made[name] = m
            slot.link = "OBJECT"
            slot.material = made[name]
    camera = camera_for(scene, objects)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(camera, do_unlink=True)


def export_asset(asset, palette):
    doc = hub.get_doc(asset)
    bpy.ops.wm.open_mainfile(filepath=hub.blend_path(asset))
    show_everything()
    folder = os.path.join(hub.CACHE, asset)
    if os.path.isdir(folder):
        shutil.rmtree(folder)
    os.makedirs(folder)
    manifest, problems, made, colours = {}, [], {}, {}
    groups = {v: variant_objects(v) for v in doc["shapes"]}
    for variant, objects in groups.items():
        if not objects:
            problems.append("variant %s has no collection or meshes" % variant)
            continue
        slots = {o.name.split("_", 1)[1]: [m.name if m else "" for m in o.data.materials] for o in objects if "_" in o.name}
        export_glb(objects, os.path.join(folder, variant + ".glb"), made)
        manifest[variant] = {"glb": variant + ".glb", "png": variant + ".png", "parts": sorted(slots), "slots": slots}
    for variant, objects in groups.items():
        if objects:
            render_thumb(asset, doc, palette, variant, objects, os.path.join(folder, variant + ".png"), colours)
    with open(os.path.join(folder, "manifest.json"), "w") as f:
        json.dump({"variants": manifest, "problems": problems}, f, indent=1)
    return problems


def main():
    names, force = get_args()
    palette = hub.get_palette()
    done = 0
    for asset in names:
        if not os.path.isfile(hub.blend_path(asset)):
            print("SKIP %s: no .blend" % asset)
            continue
        stamp = get_stamp(asset)
        if not force and is_fresh(asset, stamp):
            continue
        problems = export_asset(asset, palette)
        with open(os.path.join(hub.CACHE, asset, "stamp.json"), "w") as f:
            json.dump(stamp, f)
        done += 1
        print("EXPORTED %s%s" % (asset, "" if not problems else " (" + "; ".join(problems) + ")"), flush=True)
    print("DONE %d" % done)


main()
