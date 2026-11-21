"""Writes every scene in scenes/*.blend as a layout into assets/scenes/<scene>.json.

Run from the blender folder:
    /Applications/Blender.app/Contents/MacOS/Blender -b --python scripts/export_layout.py

To write somewhere other than the project's assets folder, name the folder after "--":
    /Applications/Blender.app/Contents/MacOS/Blender -b --python scripts/export_layout.py -- /tmp/out

What is written, all Y up:
  <scene>.json             the active camera, every light that is switched on, one entry per mesh
                           (models) and one entry per object with its matrix (objects).
  <scene>_viewpoints.json  every camera, by name.
  <scene>_lighting.json    one group per state, if the file has a text block called room_states
                           that defines STATES and apply_state(name). Each group holds the world
                           colour, the lights that are on and the models that glow in that state.

A light's color is its Blender colour times its energy times LIGHT_SCALE, so the renderer can use it
as is. A spot's penumbra is the angle over which Blender's blend fades the cone, measured in from its
edge. The world colour is the scene's ambient light (globalData.ambientLight). A light whose parent is a mesh is
paired with that mesh's model (emitter), so the renderer can switch both off together.

A mesh that has no OBJ under assets/meshes is written by this script into assets/meshes/<scene>/,
in its own local space. That folder is rebuilt every time; do not edit it by hand. The folders this
script writes for other scenes are not searched, so two scenes can use the same mesh name.

An object that is switched off for rendering, or sits in a collection that is, is not written.
An object that carries its own material gets a model of its own (<mesh>__<material>), which shares
the mesh's OBJ.
"""
import bpy, os, sys, glob, json, math, re, shutil
from mathutils import Matrix, Vector

SCRIPTS = os.path.dirname(os.path.abspath(__file__))
BLENDER = os.path.dirname(SCRIPTS)
SCENES = os.path.join(BLENDER, "scenes")
TEXTURES = os.path.join(BLENDER, "textures")
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ASSETS = os.path.abspath(ARGS[0]) if ARGS else os.path.join(os.path.dirname(BLENDER), "assets")

Y_UP = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))
LIGHT_TYPES = {'SUN': "directional", 'POINT': "point", 'SPOT': "spot"}
LIGHT_SCALE = {"point": 1 / (4 * math.pi ** 2), "spot": 1 / (4 * math.pi ** 2), "directional": 1 / math.pi}
STATES_TEXT = "room_states"
LAYOUTS = {os.path.splitext(os.path.basename(path))[0] for path in glob.glob(os.path.join(SCENES, "*.blend"))}


def position(obj):
    return list(Y_UP @ obj.matrix_world.translation)


def direction(obj, axis):
    return list(Y_UP.to_3x3() @ (obj.matrix_world.to_quaternion() @ Vector(axis)))


def custom(block):
    return {key: value.to_list() if hasattr(value, "to_list") else value for key, value in block.items()}


def camera(scene, obj):
    corner = obj.data.view_frame(scene=scene)[0]
    return {"position": position(obj), "look": direction(obj, (0, 0, -1)),
            "up": direction(obj, (0, 1, 0)),
            "heightAngle": math.degrees(2 * math.atan(abs(corner.y / corner.z)))}


def light(obj, defaults, names):
    kind = LIGHT_TYPES[obj.data.type]
    result = {"name": obj.name, **defaults, "type": kind,
              "color": [c * obj.data.energy * LIGHT_SCALE[kind] for c in obj.data.color],
              "position": position(obj), "direction": direction(obj, (0, 0, -1))}
    if obj.data.type == 'SPOT':
        half = obj.data.spot_size / 2
        inner = math.acos(math.cos(half) + (1 - math.cos(half)) * obj.data.spot_blend)
        result["angle"] = math.degrees(half)
        result["penumbra"] = math.degrees(half - inner)
    if obj.parent and obj.parent.type == 'MESH' and obj.parent.data in names:
        result["emitter"] = names[obj.parent.data]
    return {**result, **custom(obj.data)}


def lights(scene, defaults, names):
    return [light(obj, defaults, names) for obj in scene.objects
            if obj.type == 'LIGHT' and obj.data.type in LIGHT_TYPES and obj.data.energy > 0]


def world_colour(scene):
    if scene.world and scene.world.use_nodes:
        for node in scene.world.node_tree.nodes:
            if node.type == 'BACKGROUND':
                return [c * node.inputs[1].default_value for c in node.inputs[0].default_value[:3]]
    return [0.0, 0.0, 0.0]


def emission(obj):
    if obj.active_material and obj.active_material.use_nodes:
        for node in obj.active_material.node_tree.nodes:
            if node.type == 'BSDF_PRINCIPLED':
                strength = node.inputs["Emission Strength"].default_value
                if strength > 0:
                    return [c * strength for c in node.inputs["Emission Color"].default_value[:3]]
    return None


def texture_file(obj):
    if not obj.active_material or not obj.active_material.use_nodes:
        return None
    shader = next((n for n in obj.active_material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if shader is None or not shader.inputs["Base Color"].is_linked:
        return None
    node = shader.inputs["Base Color"].links[0].from_node
    if node.type != 'TEX_IMAGE' or node.image is None:
        return None
    source = os.path.normpath(bpy.path.abspath(node.image.filepath))
    if not os.path.isfile(source):
        print("EXPORT missing image %s on %s" % (source, obj.name))
        return None
    inside = os.path.commonpath([source, TEXTURES]) == TEXTURES
    relative = os.path.relpath(source, TEXTURES) if inside else os.path.basename(source)
    target = os.path.join(ASSETS, "textures", relative)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    shutil.copyfile(source, target)
    return os.path.relpath(target, ASSETS).replace(os.sep, "/")


def model(obj, mesh_file, defaults):
    result = {"mesh": os.path.relpath(mesh_file, ASSETS).replace(os.sep, "/"), **defaults}
    if obj.active_material and obj.active_material.use_nodes:
        for node in obj.active_material.node_tree.nodes:
            if node.type == 'BSDF_PRINCIPLED':
                result["diffuse"] = list(node.inputs["Base Color"].default_value)[:3]
    glow = emission(obj)
    if glow:
        result["emissive"] = glow
    image = texture_file(obj)
    if image:
        result["textureFile"] = image
        result["blend"] = 1.0
    return {**result, **custom(obj.data)}


def lighting(scene, defaults, owners, names):
    script = bpy.data.texts.get(STATES_TEXT)
    if script is None:
        return []
    module = {}
    exec(script.as_string(), module)
    groups = []
    for name in module["STATES"]:
        module["apply_state"](name)
        glow = {mesh: emission(obj) for mesh, obj in owners.items()}
        groups.append({"name": name, "ambient": world_colour(scene), "lights": lights(scene, defaults, names),
                       "emissive": {mesh: colour for mesh, colour in glow.items() if colour}})
    return groups


def write_mesh(obj, path):
    evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
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
    count = len(mesh.loop_triangles)
    evaluated.to_mesh_clear()
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")
    return count


def mesh_names(objects):
    names, taken = {}, set()
    for obj in objects:
        if obj.data in names:
            continue
        name = re.sub(r"\.\d{3}$", "", obj.data.name).lower()
        if name in taken:
            name = obj.data.name.lower().replace(".", "_")
        names[obj.data] = name
        taken.add(name)
    return names


def hidden_collections(collection, hidden=False):
    result = set()
    for child in collection.children:
        off = hidden or child.hide_render
        if off:
            result.add(child)
        result |= hidden_collections(child, off)
    return result


def rendered_meshes(scene):
    off = hidden_collections(scene.collection)
    return [obj for obj in scene.objects if obj.type == 'MESH' and not obj.hide_render
            and not any(collection in off for collection in obj.users_collection)]


def model_key(obj, mesh):
    slot = obj.material_slots[0] if obj.material_slots else None
    if slot and slot.link == 'OBJECT' and slot.material:
        return mesh + "__" + slot.material.name.lower()
    return mesh


def shared_mesh(mesh):
    for path in sorted(glob.glob(os.path.join(ASSETS, "meshes", "**", mesh + ".obj"), recursive=True)):
        if os.path.basename(os.path.dirname(path)) not in LAYOUTS:
            return path
    return None


def one_line(match):
    return re.sub(r"\s+", " ", match.group(0)).replace("[ ", "[").replace(" ]", "]")


def compact(layout):
    text = re.sub(r"\[[^\[\]{}]*\]", one_line, json.dumps(layout, indent=2))
    return re.sub(r"\[\s*(?:\[[^\[\]{}]*\],?\s*)+\]", one_line, text) + "\n"


def export(path, defaults):
    bpy.ops.wm.open_mainfile(filepath=path)
    scene = bpy.context.scene
    name = os.path.splitext(os.path.basename(path))[0]
    local = os.path.join(ASSETS, "meshes", name)
    for old in glob.glob(os.path.join(local, "*.obj")):
        os.remove(old)

    meshes = rendered_meshes(scene)
    names = mesh_names(meshes)
    world = world_colour(scene)
    layout = {"globalData": {**defaults["globalData"], "ambientLight": world}, "cameraData": camera(scene, scene.camera),
              "lights": lights(scene, defaults["light"], names), "models": {}, "objects": []}
    owners = {}
    files = {}
    written = triangles = 0
    for obj in meshes:
        mesh = names[obj.data]
        if mesh not in files:
            files[mesh] = shared_mesh(mesh)
            if files[mesh] is None:
                os.makedirs(local, exist_ok=True)
                files[mesh] = os.path.join(local, mesh + ".obj")
                triangles += write_mesh(obj, files[mesh])
                written += 1
        key = model_key(obj, mesh)
        if key not in layout["models"]:
            layout["models"][key] = model(obj, files[mesh], defaults["material"])
            owners[key] = obj
        matrix = Y_UP @ obj.matrix_world @ Y_UP.inverted()
        layout["objects"].append({"model": key, "matrix": [list(row) for row in matrix]})

    os.makedirs(os.path.join(ASSETS, "scenes"), exist_ok=True)
    with open(os.path.join(ASSETS, "scenes", name + ".json"), "w") as f:
        f.write(compact(layout))
    viewpoints = {obj.name: camera(scene, obj) for obj in sorted(scene.objects, key=lambda o: o.name) if obj.type == 'CAMERA'}
    with open(os.path.join(ASSETS, "scenes", name + "_viewpoints.json"), "w") as f:
        f.write(compact(viewpoints))
    print("EXPORT scenes/%s.json  %d models, %d objects, %d lights" % (name, len(layout["models"]), len(layout["objects"]), len(layout["lights"])))
    print("EXPORT scenes/%s_viewpoints.json  %d viewpoints" % (name, len(viewpoints)))
    paired = {entry["emitter"] for entry in layout["lights"] if "emitter" in entry}
    pairs = ["%s > %s" % (entry["name"], entry["emitter"]) for entry in layout["lights"] if "emitter" in entry]
    unpaired = [mesh for mesh, entry in layout["models"].items() if "emissive" in entry and mesh not in paired]
    print("EXPORT pairs  %s" % (", ".join(pairs) or "none"))
    print("EXPORT unpaired emissive  %s" % (", ".join(unpaired) or "none"))
    if written:
        print("EXPORT meshes/%s/  %d meshes, %d triangles" % (name, written, triangles))

    groups = lighting(scene, defaults["light"], owners, names)
    if groups:
        with open(os.path.join(ASSETS, "scenes", name + "_lighting.json"), "w") as f:
            f.write(compact(groups))
        print("EXPORT scenes/%s_lighting.json  %s" % (name, ", ".join(group["name"] for group in groups)))


with open(os.path.join(SCRIPTS, "layout_defaults.json")) as f:
    defaults = json.load(f)
shutil.rmtree(os.path.join(ASSETS, "textures"), ignore_errors=True)
for path in sorted(glob.glob(os.path.join(SCENES, "*.blend"))):
    if os.path.splitext(os.path.basename(path))[0] not in defaults["exclude"]:
        export(path, defaults)
