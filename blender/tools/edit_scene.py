import bpy, os, json, math, re, shutil, sys
from mathutils import Matrix, Vector, Euler

ROOT = globals().get("ROOT") or os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(ROOT)
MODELS = os.path.join(ROOT, "models")
DATA = os.path.join(REPO, "assets", "models")
SCENES = os.path.join(REPO, "assets", "scenes")
MATERIALS = os.path.join(ROOT, "materials", "palette.blend")
FILES = {"layout": "layout.json", "lighting": "lighting.json", "viewpoints": "viewpoints.json"}
PARTS = ("layout", "look", "lighting", "viewpoints")
MODES = ("VIEW", "LAYOUT", "LOOK", "LIGHTING", "CAMERAS")
MODE_PART = {"LAYOUT": "layout", "LOOK": "look", "LIGHTING": "lighting", "CAMERAS": "viewpoints"}
SESSION = os.path.join(ROOT, ".scene.json")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import space
NAME = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
EPS = 1e-5
LIGHT_KEYS = ("type", "attach", "glowPart", "position", "direction", "angle", "penumbra", "softSize", "shadow")

S = {"mode": "VIEW", "scene": "room", "files": {"look": None}, "state": None, "camera": None,
     "data": {}, "disk": {}, "docs": {}, "glowing": set(), "busy": False}


def clean(v):
    if isinstance(v, float):
        return 0.0 if v == 0 else v
    if isinstance(v, list):
        return [clean(x) for x in v]
    if isinstance(v, dict):
        return {k: clean(x) for k, x in v.items()}
    return v


def flat(v):
    if isinstance(v, dict):
        return all(not isinstance(x, dict) and not (isinstance(x, list) and any(isinstance(y, (list, dict)) for y in x)) for x in v.values())
    return isinstance(v, list) and all(not isinstance(x, (list, dict)) for x in v)


def write_json(v, level=0):
    pad = "  " * level
    short = level > 0 and len(json.dumps(v, separators=(", ", ": "))) <= 150
    if isinstance(v, dict) and v and not (level > 0 and (flat(v) or short)):
        items = ["%s  %s: %s" % (pad, json.dumps(k, ensure_ascii=False), write_json(x, level + 1)) for k, x in v.items()]
        return "{\n" + ",\n".join(items) + "\n" + pad + "}"
    if isinstance(v, list) and v and not (flat(v) or short):
        items = ["%s  %s" % (pad, write_json(x, level + 1)) for x in v]
        return "[\n" + ",\n".join(items) + "\n" + pad + "]"
    return json.dumps(v, ensure_ascii=False, separators=(", ", ": "))


def dumps(data):
    return write_json(clean(data)) + "\n"


def write_text(path, text):
    temporary = path + ".tmp"
    with open(temporary, "w") as f:
        f.write(text)
    os.replace(temporary, path)


def scene_folder(scene=None):
    return os.path.join(SCENES, scene or S["scene"])


def path_of(part, name=None):
    if part == "look":
        return os.path.join(scene_folder(), "looks", (name or S["files"]["look"]) + ".json")
    return os.path.join(scene_folder(), FILES[part])


def files_of(part):
    folder = os.path.join(scene_folder(), "looks")
    return sorted(f[:-5] for f in os.listdir(folder) if f.endswith(".json") and not f.startswith(("_", "."))) if part == "look" else []


def scenes():
    return sorted(s for s in os.listdir(SCENES) if os.path.isfile(os.path.join(SCENES, s, "scene.json")))


def scene_settings():
    return json.load(open(os.path.join(scene_folder(), "scene.json")))


def label_of(part):
    return os.path.relpath(path_of(part), REPO)


def load_part(part, name=None):
    text = open(path_of(part, name)).read()
    if part == "look":
        S["files"]["look"] = name or S["files"]["look"]
    S["disk"][part] = text
    S["data"][part] = json.loads(text)


def doc(asset):
    if asset not in S["docs"]:
        S["docs"][asset] = json.load(open(os.path.join(DATA, asset, "asset.json")))
    return S["docs"][asset]


def assets():
    return sorted(a for a in os.listdir(DATA) if os.path.isfile(os.path.join(DATA, a, "asset.json")))


def save_session():
    keep = {"mode": S["mode"], "scene": S["scene"], "files": S["files"], "state": S["state"], "camera": S["camera"]}
    write_text(SESSION, dumps(keep))


def load_session():
    if os.path.isfile(SESSION):
        saved = json.load(open(SESSION))
        S["mode"] = saved.get("mode", "VIEW")
        S["state"], S["camera"] = saved.get("state"), saved.get("camera")
        if saved.get("scene") in scenes():
            S["scene"] = saved["scene"]
        look = saved.get("files", {}).get("look")
        if look and os.path.isfile(path_of("look", look)):
            S["files"]["look"] = look


def light_basis(data):
    location = space.to_blender_point(data["position"]) if "position" in data else Vector((0, 0, 0))
    turn = space.aim(space.to_blender_direction(data["direction"])) if "direction" in data else Euler((0, 0, 0))
    return Matrix.Translation(location) @ turn.to_matrix().to_4x4()


def close(a, b, eps=EPS):
    return len(a) == len(b) and all(abs(x - y) <= eps for x, y in zip(a, b))


def keep(old, new, digits, eps=EPS):
    return old if old is not None and close(old, new, eps) else [round(v, digits) for v in new]


def collection(name, hidden=False):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(c)
    layer = bpy.context.view_layer.layer_collection.children.get(name)
    if layer:
        layer.hide_viewport = hidden
    return c


def clear(name):
    c = bpy.data.collections.get(name)
    if c is None:
        return
    for o in list(c.objects):
        bpy.data.objects.remove(o, do_unlink=True)


def linked_meshes(asset, names):
    path = os.path.abspath(os.path.join(MODELS, asset, "model.blend"))
    found = {m.name: m for m in bpy.data.meshes
             if m.library and os.path.abspath(bpy.path.abspath(m.library.filepath)) == path and m.name in names}
    missing = [n for n in names if n not in found]
    if missing:
        with bpy.data.libraries.load(path, link=True, relative=False) as (src, dst):
            dst.meshes = [n for n in missing if n in src.meshes]
        found.update({m.name: m for m in dst.meshes if m})
    return found


def linked_material(asset, name):
    local = doc(asset).get("materials", {})
    path = os.path.join(MODELS, asset, "model.blend") if name in local else MATERIALS
    for m in bpy.data.materials:
        if m.name == name and m.library and os.path.abspath(bpy.path.abspath(m.library.filepath)) == os.path.abspath(path):
            return m
    with bpy.data.libraries.load(path, link=True, relative=False) as (src, dst):
        dst.materials = [name] if name in src.materials else []
    return dst.materials[0] if dst.materials else None


def choice(name, asset):
    entry = S["data"]["look"]["objects"].get(name, {})
    d = doc(asset)
    shape = entry.get("shape", d["shapes"][0])
    colours = [c for c, v in d["colors"].items() if shape in v.get("shapes", d["shapes"])]
    colour = entry.get("color", colours[0] if colours else None)
    return shape, colour, entry.get("scale")


def build_parts(holder):
    for child in list(holder.children):
        if child.get("room_part"):
            bpy.data.objects.remove(child, do_unlink=True)
    name, asset = holder["room_object"], holder["room_asset"]
    d = doc(asset)
    shape, colour, scale = holder["room_shape"], holder["room_color"], holder.get("room_scale")
    holder.scale = space.to_blender_scale(scale) if scale else (1, 1, 1)
    meshes = linked_meshes(asset, ["%s_%s" % (shape, p) for p in d["parts"]])
    spec = d["colors"].get(colour, {})
    for part in d["parts"]:
        mesh = meshes.get("%s_%s" % (shape, part))
        if mesh is None:
            continue
        child = bpy.data.objects.new("%s_%s" % (name, part), mesh)
        child["room_part"] = part
        for c in holder.users_collection:
            c.objects.link(child)
        child.parent = holder
        child.lock_location = child.lock_rotation = child.lock_scale = (True, True, True)
        value = spec.get(part, spec.get("*"))
        for i, m in enumerate(value if isinstance(value, list) else ([value] if value else [])):
            if i < len(child.material_slots):
                material = linked_material(asset, m)
                if material:
                    child.material_slots[i].link = 'OBJECT'
                    child.material_slots[i].material = material


def build_object(name, obj):
    holder = bpy.data.objects.new(name, None)
    holder.empty_display_type = 'PLAIN_AXES'
    holder.empty_display_size = 0.05
    holder["room_object"], holder["room_asset"] = name, obj["asset"]
    shape, colour, scale = choice(name, obj["asset"])
    holder["room_shape"], holder["room_color"] = shape, colour or ""
    if scale:
        holder["room_scale"] = scale
    collection("Maybe" if obj["group"] == "maybe" else "Room").objects.link(holder)
    holder.location = space.to_blender_point(obj["position"])
    holder.rotation_euler = space.to_blender_rotation(obj["rotation"])
    holder.lock_scale = (True, True, True)
    build_parts(holder)
    return holder


def holders():
    return [o for o in bpy.data.objects if o.get("room_object")]


def light_objects():
    return [o for o in bpy.data.objects if o.get("room_light")]


def build_layout():
    clear("Room")
    clear("Maybe")
    collection("Room")
    collection("Maybe", hidden=True)
    for name, obj in S["data"]["layout"]["objects"].items():
        build_object(name, obj)


def build_lights():
    clear("Lights")
    c = collection("Lights")
    for name, data in S["data"]["lighting"]["lights"].items():
        if data["type"] == "glow":
            o = bpy.data.objects.new(name, None)
            o.empty_display_type = 'SPHERE'
            o.empty_display_size = 0.08
        else:
            light = bpy.data.lights.new(name, data["type"].upper())
            light.use_shadow = data.get("shadow", True)
            if "angle" in data:
                light.spot_size, light.spot_blend = space.to_blender_spot(data["angle"], data["penumbra"])
            if "softSize" in data:
                light.shadow_soft_size = data["softSize"]
            o = bpy.data.objects.new(name, light)
        o["room_light"] = name
        c.objects.link(o)
        attach = bpy.data.objects.get(data.get("attach", ""))
        if attach is not None:
            o.parent = attach
            o.matrix_parent_inverse = Matrix.Identity(4)
            o.matrix_basis = light_basis(data)
            if attach.users_collection and attach.users_collection[0].name == "Maybe":
                o.hide_viewport = o.hide_render = True
        else:
            o.matrix_basis = light_basis(data)
    states = S["data"]["lighting"]["states"]
    if S["state"] not in states:
        start = scene_settings().get("start", {}).get("state")
        S["state"] = start if start in states else next(iter(states))
    apply_state()


def glow_part(o):
    data = S["data"]["lighting"]["lights"].get(o["room_light"], {})
    attach, part = data.get("attach"), data.get("glowPart")
    return bpy.data.objects.get("%s_%s" % (attach, part)) if attach and part else None


def set_glow(o):
    part = glow_part(o)
    if part is None:
        return
    entry = S["data"]["lighting"]["states"][S["state"]]["lights"].get(o["room_light"], {})
    glow = entry.get("glow")
    if glow is None:
        return
    set_part_glow(part, glow)


def set_part_glow(part, glow):
    for slot in part.material_slots:
        if slot.material is None:
            continue
        material = slot.material
        if not material.name.endswith("_glow"):
            material = material.copy()
            material.name = "%s_glow" % part.name
            slot.link = 'OBJECT'
            slot.material = material
        node = next(n for n in material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        node.inputs["Emission Strength"].default_value = glow["strength"]
        node.inputs["Emission Color"].default_value = list(glow["color"]) + [1.0]
        if "base" in glow:
            node.inputs["Base Color"].default_value = list(glow["base"]) + [1.0]


def apply_state():
    S["busy"] = True
    state = S["data"]["lighting"]["states"][S["state"]]
    for o in light_objects():
        entry = state["lights"].get(o["room_light"], {})
        if o.type == 'LIGHT':
            o.data.energy = entry.get("intensity", 0.0) / space.light_scale(o.data.type.lower())
            o.data.color = entry.get("color", [1.0, 1.0, 1.0])
            if "direction" in entry and o.parent is None:
                o.rotation_euler = space.aim(space.to_blender_direction(entry["direction"]))
        glow = entry.get("glow", {})
        o.room_glow_strength = glow.get("strength", 0.0)
        o.room_glow_color = glow.get("color", [1.0, 1.0, 1.0])
        set_glow(o)
    scene = bpy.context.scene
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    background = next(n for n in scene.world.node_tree.nodes if n.type == 'BACKGROUND')
    background.inputs[0].default_value = list(state["environment"]["world"]) + [1.0]
    apply_state_glow(state)
    S["busy"] = False


def apply_state_glow(state):
    glowing = set()
    for name, parts in state.get("glow", {}).items():
        for part, glow in parts.items():
            o = bpy.data.objects.get("%s_%s" % (name, part))
            if o is not None:
                set_part_glow(o, glow)
                glowing.add(o.name)
    for name in S["glowing"] - glowing:
        o = bpy.data.objects.get(name)
        if o is not None:
            set_part_glow(o, {"strength": 0.0, "color": [1.0, 1.0, 1.0]})
    S["glowing"] = glowing


def build_cameras():
    clear("Cameras")
    c = collection("Cameras")
    for name, cam in S["data"]["viewpoints"].items():
        o = bpy.data.objects.new(name, bpy.data.cameras.new(name))
        o["room_camera"] = name
        o.location, o.rotation_euler, o.data.angle = space.to_blender_camera(cam)
        c.objects.link(o)
    if S["camera"] not in S["data"]["viewpoints"]:
        start = scene_settings().get("start", {}).get("viewpoint")
        S["camera"] = start if start in S["data"]["viewpoints"] else next(iter(S["data"]["viewpoints"]), None)
    if S["camera"]:
        bpy.context.scene.camera = bpy.data.objects.get(S["camera"])


def rebuild_parts_after_glow():
    for o in light_objects():
        set_glow(o)


def viewing_camera():
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            if area.type == 'VIEW_3D' and area.spaces.active.region_3d.view_perspective == 'CAMERA':
                return True
    return False


def build_all(cameras=True):
    looking = viewing_camera()
    S["busy"] = True
    S["docs"] = {}
    build_layout()
    build_lights()
    if cameras:
        build_cameras()
    apply_mode()
    S["busy"] = False
    if looking and S["camera"]:
        jump(S["camera"])


def load_look(name):
    load_part("look", name)
    for part in ("layout", "lighting", "viewpoints"):
        load_part(part)


def apply_mode():
    mode = S["mode"]
    for o in bpy.data.objects:
        if o.get("room_object") or o.get("room_part"):
            o.hide_select = mode not in ("LAYOUT", "LOOK")
            if o.get("room_object"):
                locked = mode != "LAYOUT"
                o.lock_location = o.lock_rotation = (locked, locked, locked)
        elif o.get("room_light"):
            o.hide_select = mode != "LIGHTING"
        elif o.get("room_camera"):
            o.hide_select = mode != "CAMERAS"


def serialize_layout():
    old = S["data"]["layout"]["objects"]
    out = {}
    current = {o["room_object"]: o for o in holders()}
    for name in list(old) + sorted(n for n in current if n not in old):
        o = current.get(name)
        if o is None:
            continue
        prev = old.get(name, {})
        kept = prev.get("group") if prev.get("group") not in (None, "maybe") else "life"
        group = "maybe" if o.users_collection and o.users_collection[0].name == "Maybe" else kept
        out[name] = {"asset": o["room_asset"], "position": keep(prev.get("position"), space.to_engine_point(o.location), 5),
                     "rotation": keep(prev.get("rotation"), space.to_engine_rotation(o.rotation_euler), 4, 1e-3), "group": group}
    return {**S["data"]["layout"], "objects": out}


def serialize_look():
    data = S["data"]["look"]
    old = data["objects"]
    out = {}
    current = {o["room_object"]: o for o in holders()}
    for name in list(old) + sorted(n for n in current if n not in old):
        o = current.get(name)
        if o is None:
            continue
        d = doc(o["room_asset"])
        entry = dict(old.get(name, {}))
        default_shape = d["shapes"][0]
        default_colours = [c for c, v in d["colors"].items() if o["room_shape"] in v.get("shapes", d["shapes"])]
        if name not in old and o["room_shape"] == default_shape and default_colours and o["room_color"] == default_colours[0]:
            continue
        entry["shape"], entry["color"] = o["room_shape"], o["room_color"]
        out[name] = {k: entry[k] for k in ("shape", "color", "scale") if k in entry}
    return {**data, "objects": out}


def capture_state():
    lighting = S["data"]["lighting"]
    state = lighting["states"][S["state"]]
    lights = {}
    for o in light_objects():
        name = o["room_light"]
        prev = state["lights"].get(name, {})
        entry = dict(prev)
        if o.type == 'LIGHT':
            intensity = o.data.energy * space.light_scale(o.data.type.lower())
            entry["intensity"] = prev["intensity"] if "intensity" in prev and abs(prev["intensity"] - intensity) <= EPS else round(intensity, space.INTENSITY_DIGITS)
            entry["color"] = keep(prev.get("color"), o.data.color, 4)
            if "direction" in prev:
                entry["direction"] = keep(prev["direction"], space.to_engine_direction(space.facing(o.rotation_euler)), 6)
        if "glow" in prev or o.room_glow_strength > 0:
            glow = dict(prev.get("glow", {}))
            glow["strength"] = glow["strength"] if "strength" in glow and abs(glow["strength"] - o.room_glow_strength) <= EPS else round(o.room_glow_strength, 4)
            glow["color"] = keep(glow.get("color"), o.room_glow_color, 4)
            entry["glow"] = glow
        lights[name] = entry
    order = [n for n in state["lights"] if n in lights] + [n for n in lights if n not in state["lights"]]
    world = bpy.context.scene.world
    background = next(n for n in world.node_tree.nodes if n.type == 'BACKGROUND')
    state["environment"]["world"] = keep(state["environment"]["world"], background.inputs[0].default_value[:3], 4)
    state["lights"] = {n: lights[n] for n in order}
    for other in lighting["states"].values():
        if other is not state:
            other["lights"] = {n: v for n, v in other["lights"].items() if n in lights}
            for n in lights:
                other["lights"].setdefault(n, dict(lights[n]))


def state_driven(name):
    return any("direction" in state["lights"].get(name, {}) for state in S["data"]["lighting"]["states"].values())


def serialize_lighting():
    capture_state()
    lighting = S["data"]["lighting"]
    old = lighting["lights"]
    out = {}
    current = {o["room_light"]: o for o in light_objects()}
    for name in list(old) + sorted(n for n in current if n not in old):
        o = current.get(name)
        if o is None:
            continue
        prev = old.get(name, {})
        entry = dict(prev)
        if o.type == 'LIGHT':
            entry["type"] = {"SUN": "sun", "SPOT": "spot", "POINT": "point", "AREA": "area"}[o.data.type]
        else:
            entry["type"] = "glow"
        parent = o.parent.get("room_object") if o.parent else None
        if parent:
            entry["attach"] = parent
        else:
            entry.pop("attach", None)
            entry.pop("glowPart", None)
        location, turn, _ = o.matrix_basis.decompose()
        if entry["type"] in ("point", "spot") or "position" in prev:
            entry["position"] = keep(prev.get("position"), space.to_engine_point(location), 5)
        if entry["type"] in ("spot", "sun") and not state_driven(name):
            entry["direction"] = keep(prev.get("direction"), space.to_engine_direction(space.facing(turn)), 6)
        if o.type == 'LIGHT':
            d = o.data
            if d.type == 'SPOT':
                entry["angle"], entry["penumbra"] = keep([prev.get("angle"), prev.get("penumbra")] if "angle" in prev else None,
                                                         space.to_engine_spot(d.spot_size, d.spot_blend), 4, 1e-3)
            if "softSize" in prev:
                entry["softSize"] = prev["softSize"] if abs(prev["softSize"] - d.shadow_soft_size) <= EPS else round(d.shadow_soft_size, 6)
            entry["shadow"] = d.use_shadow
        ordered = {k: entry[k] for k in LIGHT_KEYS if k in entry}
        ordered.update({k: v for k, v in entry.items() if k not in ordered})
        out[name] = ordered
    return {**lighting, "lights": out, "states": lighting["states"]}


def serialize_viewpoints():
    old = S["data"]["viewpoints"]
    out = {}
    current = {o["room_camera"]: o for o in bpy.data.objects if o.get("room_camera")}
    for name in list(old) + sorted(n for n in current if n not in old):
        o = current.get(name)
        if o is None:
            continue
        prev = old.get(name, {})
        view = space.to_engine_camera(o.location, o.rotation_euler, o.data.angle)
        out[name] = {"position": keep(prev.get("position"), view["position"], 5),
                     "look": keep(prev.get("look"), view["look"], 6),
                     "up": keep(prev.get("up"), view["up"], 6),
                     "heightAngle": prev["heightAngle"] if "heightAngle" in prev and abs(prev["heightAngle"] - view["heightAngle"]) <= 1e-3 else view["heightAngle"]}
    return out


SERIALIZE = {"layout": serialize_layout, "look": serialize_look, "lighting": serialize_lighting, "viewpoints": serialize_viewpoints}


def text_of(part):
    return dumps(SERIALIZE[part]())


def dirty(part):
    try:
        return text_of(part) != S["disk"][part]
    except Exception:
        return True


def problems(part, data):
    found = []
    if part == "layout":
        for name, obj in data["objects"].items():
            if not NAME.match(name):
                found.append("bad name %s" % name)
    elif part == "look":
        layout = json.load(open(path_of("layout")))["objects"]
        for name, entry in data["objects"].items():
            if name not in layout:
                continue
            d = doc(layout[name]["asset"])
            if entry["shape"] not in d["shapes"]:
                found.append("%s: unknown shape %s" % (name, entry["shape"]))
            if entry["color"] not in d["colors"]:
                found.append("%s: unknown colour %s" % (name, entry["color"]))
    elif part == "lighting":
        layout = S["data"]["layout"]["objects"]
        for name, light in data["lights"].items():
            attach = light.get("attach")
            if attach and attach not in layout:
                found.append("%s attaches to missing %s" % (name, attach))
            if attach and light.get("glowPart") and light["glowPart"] not in doc(layout[attach]["asset"])["parts"]:
                found.append("%s glows missing part %s" % (name, light["glowPart"]))
            if light["type"] == "glow" and not attach:
                found.append("%s glows but is attached to nothing" % name)
    return found


def write_part(part, name, data):
    text = dumps(data)
    write_text(path_of(part, name), text)
    if part == "look":
        S["files"]["look"] = name or S["files"]["look"]
    S["data"][part] = json.loads(text)
    S["disk"][part] = text


def prune_looks(removed):
    for name in files_of("look"):
        path = path_of("look", name)
        data = json.load(open(path))
        kept = {k: v for k, v in data["objects"].items() if k not in removed}
        if len(kept) != len(data["objects"]):
            data["objects"] = kept
            write_text(path, dumps(data))
            if name == S["files"]["look"]:
                load_part("look", name)


def save(part, name=None):
    data = SERIALIZE[part]()
    issues = problems(part, data)
    if issues:
        return issues
    if part == "layout":
        removed = set(S["data"]["layout"]["objects"]) - set(data["objects"])
        if removed:
            prune_looks(removed)
    write_part(part, name, data)
    for o in holders():
        o.pop("room_new", None)
    save_session()
    return []


def revert(part):
    looking = viewing_camera()
    load_part(part)
    S["busy"] = True
    if part == "layout":
        build_layout()
        build_lights()
    elif part == "look":
        for o in holders():
            shape, colour, scale = choice(o["room_object"], o["room_asset"])
            o["room_shape"], o["room_color"] = shape, colour or ""
            build_parts(o)
        rebuild_parts_after_glow()
    elif part == "lighting":
        build_lights()
    elif part == "viewpoints":
        build_cameras()
    apply_mode()
    S["busy"] = False
    if looking and S["camera"]:
        jump(S["camera"])



def selected_holder():
    o = bpy.context.view_layer.objects.active
    while o is not None and not o.get("room_object") and o.parent is not None:
        o = o.parent
    return o if o is not None and o.get("room_object") else None


def selected_light():
    o = bpy.context.view_layer.objects.active
    return o if o is not None and o.get("room_light") else None


def next_name(asset):
    taken = {o["room_object"] for o in holders()} | set(S["data"]["layout"]["objects"])
    if asset not in taken:
        return asset
    n = 2
    while "%s%02d" % (asset, n) in taken:
        n += 1
    return "%s%02d" % (asset, n)


def nearest_objects(o, count=8):
    position = o.matrix_world.translation
    life = [h for h in holders() if h.users_collection and h.users_collection[0].name == "Room"]
    life.sort(key=lambda h: (h.matrix_world.translation - position).length)
    return [h["room_object"] for h in life[:count]]


def on_active(*_):
    if S["busy"]:
        return
    o = bpy.context.view_layer.objects.active
    if o is not None and o.get("room_part") and o.parent is not None and S["mode"] in ("LAYOUT", "LOOK"):
        S["busy"] = True
        o.select_set(False)
        o.parent.select_set(True)
        bpy.context.view_layer.objects.active = o.parent
        S["busy"] = False


def on_update(scene, depsgraph):
    if S["busy"]:
        return
    orphans = [o for o in bpy.data.objects if o.get("room_part") and o.parent is None]
    if orphans:
        S["busy"] = True
        for o in orphans:
            bpy.data.objects.remove(o, do_unlink=True)
        S["busy"] = False


def glow_update(self, context):
    if S["busy"] or not self.get("room_light") or S["state"] is None:
        return
    entry = S["data"]["lighting"]["states"][S["state"]]["lights"].setdefault(self["room_light"], {})
    glow = entry.setdefault("glow", {})
    glow["strength"], glow["color"] = self.room_glow_strength, list(self.room_glow_color)
    set_glow(self)


def jump(camera):
    o = bpy.data.objects.get(camera)
    if o is None:
        return
    S["camera"] = camera
    bpy.context.scene.camera = o
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            if area.type == 'VIEW_3D':
                area.spaces.active.region_3d.view_perspective = 'CAMERA'


def blocked(op, parts):
    busy = [p for p in parts if p in S["data"] and dirty(p)]
    if busy:
        op.report({'WARNING'}, "Save or revert first: " + ", ".join(busy))
        return True
    return False


class ROOM_OT_mode(bpy.types.Operator):
    bl_idname = "room.mode"
    bl_label = "Mode"
    bl_options = {'INTERNAL'}
    value: bpy.props.StringProperty()

    def execute(self, context):
        current = MODE_PART.get(S["mode"])
        if current and blocked(self, [current]):
            return {'CANCELLED'}
        S["mode"] = self.value
        apply_mode()
        save_session()
        return {'FINISHED'}


class ROOM_OT_scene(bpy.types.Operator):
    bl_idname = "room.scene"
    bl_label = "Scene"
    bl_options = {'INTERNAL'}
    value: bpy.props.StringProperty()

    def execute(self, context):
        if blocked(self, list(PARTS)):
            return {'CANCELLED'}
        S["scene"] = self.value
        S["files"]["look"] = start_look()
        S["state"] = S["camera"] = None
        load_look(S["files"]["look"])
        build_all()
        if S["camera"]:
            jump(S["camera"])
        save_session()
        return {'FINISHED'}


class ROOM_OT_preset(bpy.types.Operator):
    bl_idname = "room.preset"
    bl_label = "Look"
    bl_options = {'INTERNAL'}
    value: bpy.props.StringProperty()

    def execute(self, context):
        if blocked(self, list(PARTS)):
            return {'CANCELLED'}
        load_look(self.value)
        build_all(cameras=False)
        save_session()
        return {'FINISHED'}


class ROOM_OT_save(bpy.types.Operator):
    bl_idname = "room.save"
    bl_label = "Save"
    bl_options = {'INTERNAL'}
    part: bpy.props.StringProperty()

    def execute(self, context):
        issues = save(self.part)
        if issues:
            self.report({'ERROR'}, "Not saved: " + "; ".join(issues[:3]))
            return {'CANCELLED'}
        self.report({'INFO'}, "Saved " + label_of(self.part))
        return {'FINISHED'}


class ROOM_OT_revert(bpy.types.Operator):
    bl_idname = "room.revert"
    bl_label = "Revert"
    bl_options = {'INTERNAL'}
    part: bpy.props.StringProperty()

    def execute(self, context):
        revert(self.part)
        return {'FINISHED'}


class ROOM_OT_new(bpy.types.Operator):
    bl_idname = "room.new"
    bl_label = "New from this"
    bl_options = {'INTERNAL'}
    part: bpy.props.StringProperty()
    name: bpy.props.StringProperty(name="Name")

    def invoke(self, context, event):
        return context.window_manager.invoke_props_dialog(self)

    def execute(self, context):
        if not NAME.match(self.name) or os.path.exists(path_of(self.part, self.name)):
            self.report({'ERROR'}, "Pick a new name made of letters and digits")
            return {'CANCELLED'}
        issues = save(self.part, self.name)
        if issues:
            self.report({'ERROR'}, "Not saved: " + "; ".join(issues[:3]))
            return {'CANCELLED'}
        return {'FINISHED'}


class ROOM_OT_state(bpy.types.Operator):
    bl_idname = "room.state"
    bl_label = "State"
    bl_options = {'INTERNAL'}
    value: bpy.props.StringProperty()

    def execute(self, context):
        if S["mode"] == "LIGHTING":
            capture_state()
        S["state"] = self.value
        apply_state()
        save_session()
        return {'FINISHED'}


class ROOM_OT_state_edit(bpy.types.Operator):
    bl_idname = "room.state_edit"
    bl_label = "State"
    bl_options = {'INTERNAL'}
    action: bpy.props.StringProperty()
    name: bpy.props.StringProperty(name="Name")

    def invoke(self, context, event):
        if self.action == "delete":
            return self.execute(context)
        return context.window_manager.invoke_props_dialog(self)

    def execute(self, context):
        capture_state()
        states = S["data"]["lighting"]["states"]
        current = S["state"]
        if self.action == "delete":
            if len(states) == 1:
                self.report({'ERROR'}, "A lighting needs at least one state")
                return {'CANCELLED'}
            del states[current]
            S["state"] = next(iter(states))
        else:
            if not NAME.match(self.name) or self.name in states:
                self.report({'ERROR'}, "Pick a new state name")
                return {'CANCELLED'}
            if self.action == "copy":
                states[self.name] = json.loads(json.dumps(states[current]))
            else:
                S["data"]["lighting"]["states"] = {(self.name if k == current else k): v for k, v in states.items()}
            S["state"] = self.name
        apply_state()
        return {'FINISHED'}


class ROOM_OT_export(bpy.types.Operator):
    bl_idname = "room.export"
    bl_label = "Export models"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        import subprocess
        script = os.path.join(ROOT, "tools", "export_assets.py")
        result = subprocess.run([bpy.app.binary_path, "-b", "--factory-startup", "--python", script],
                                capture_output=True, text=True)
        lines = [l for l in result.stdout.splitlines() if l.startswith("EXPORT")]
        if result.returncode != 0 or "Traceback" in result.stdout + result.stderr:
            self.report({'ERROR'}, "Export failed, see the system console")
            print(result.stdout[-4000:], result.stderr[-4000:])
            return {'CANCELLED'}
        self.report({'INFO'}, lines[-1] if lines else "Exported")
        return {'FINISHED'}


class ROOM_OT_camera(bpy.types.Operator):
    bl_idname = "room.camera"
    bl_label = "Camera"
    bl_options = {'INTERNAL'}
    value: bpy.props.StringProperty()

    def execute(self, context):
        jump(self.value)
        save_session()
        return {'FINISHED'}


class ROOM_OT_group(bpy.types.Operator):
    bl_idname = "room.group"
    bl_label = "Group"
    bl_options = {'INTERNAL'}
    value: bpy.props.StringProperty()

    def execute(self, context):
        o = selected_holder()
        if o is None:
            return {'CANCELLED'}
        target = collection("Maybe", hidden=True) if self.value == "maybe" else collection("Room")
        for x in [o] + [c for c in o.children if c.get("room_part")]:
            for c in list(x.users_collection):
                c.objects.unlink(x)
            target.objects.link(x)
        collection("Maybe", hidden=True)
        return {'FINISHED'}


class ROOM_OT_add(bpy.types.Operator):
    bl_idname = "room.add"
    bl_label = "Add at cursor"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        asset = context.window_manager.room_asset
        name = next_name(asset)
        S["busy"] = True
        holder = build_object(name, {"asset": asset, "position": space.to_engine_point(context.scene.cursor.location), "rotation": [0.0, 0.0, 0.0], "group": "life"})
        holder["room_new"] = True
        apply_mode()
        S["busy"] = False
        for o in context.selected_objects:
            o.select_set(False)
        holder.select_set(True)
        context.view_layer.objects.active = holder
        return {'FINISHED'}


class ROOM_OT_look(bpy.types.Operator):
    bl_idname = "room.look"
    bl_label = "Look"
    bl_options = {'INTERNAL'}
    key: bpy.props.StringProperty()
    value: bpy.props.StringProperty()

    def execute(self, context):
        o = selected_holder()
        if o is None:
            return {'CANCELLED'}
        d = doc(o["room_asset"])
        if self.key == "shape":
            o["room_shape"] = self.value
            fits = [c for c, v in d["colors"].items() if self.value in v.get("shapes", d["shapes"])]
            if o["room_color"] not in fits and fits:
                o["room_color"] = fits[0]
        else:
            o["room_color"] = self.value
        S["busy"] = True
        build_parts(o)
        apply_mode()
        rebuild_parts_after_glow()
        S["busy"] = False
        return {'FINISHED'}


class ROOM_OT_attach(bpy.types.Operator):
    bl_idname = "room.attach"
    bl_label = "Attach"
    bl_options = {'INTERNAL'}
    key: bpy.props.StringProperty()
    value: bpy.props.StringProperty()

    def execute(self, context):
        o = selected_light()
        if o is None:
            return {'CANCELLED'}
        lights = S["data"]["lighting"]["lights"]
        entry = lights.setdefault(o["room_light"], {"type": "glow" if o.type != 'LIGHT' else o.data.type.lower()})
        old = glow_part(o)
        if self.key == "attach":
            context.view_layer.update()
            world = o.matrix_world.copy()
            target = bpy.data.objects.get(self.value) if self.value else None
            o.parent = target
            o.matrix_parent_inverse = Matrix.Identity(4)
            o.matrix_basis = (target.matrix_world.inverted() @ world) if target else world
            context.view_layer.update()
            if target is None:
                entry.pop("attach", None)
                entry.pop("glowPart", None)
            else:
                entry["attach"] = self.value
                entry.pop("glowPart", None)
        else:
            if self.value:
                entry["glowPart"] = self.value
            else:
                entry.pop("glowPart", None)
        if old is not None and old != glow_part(o):
            holder = old.parent
            S["busy"] = True
            build_parts(holder)
            apply_mode()
            rebuild_parts_after_glow()
            S["busy"] = False
        set_glow(o)
        return {'FINISHED'}


def buttons(layout, items, current, op, columns=0, **props):
    grid = layout.grid_flow(row_major=True, columns=columns, even_columns=False, align=True)
    for item in items:
        b = grid.operator(op, text=item, depress=item == current)
        for k, v in props.items():
            setattr(b, k, v)
        b.value = item


def file_row(layout, part):
    box = layout.column(align=True)
    if part == "look":
        box.label(text="Look")
        buttons(box, files_of("look"), S["files"]["look"], "room.preset")
    row = box.row(align=True)
    if part == "look":
        row.operator("room.new", text="New from this").part = part
    row.operator("room.save", text="Save").part = part
    row.operator("room.revert", text="Revert").part = part


class ROOM_PT_panel(bpy.types.Panel):
    bl_label = "Scene"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Scene"

    def draw(self, context):
        layout = self.layout
        if "look" not in S["data"]:
            layout.label(text="Scene is not loaded")
            return
        buttons(layout, MODES, S["mode"], "room.mode", columns=5)
        mode = S["mode"]
        part = MODE_PART.get(mode)
        if part:
            unsaved = dirty(part)
            layout.label(text=("Unsaved: " + label_of(part)) if unsaved else "Saved", icon='ERROR' if unsaved else 'CHECKMARK')
        layout.separator()
        if mode == "VIEW":
            layout.operator("room.export", text="Export models", icon='EXPORT')
            layout.label(text="Scene")
            buttons(layout, scenes(), S["scene"], "room.scene")
            layout.label(text="Look")
            buttons(layout, files_of("look"), S["files"]["look"], "room.preset")
            layout.label(text="State")
            buttons(layout, list(S["data"]["lighting"]["states"]), S["state"], "room.state")
            layout.label(text="Camera")
            buttons(layout, list(S["data"]["viewpoints"]), S["camera"], "room.camera")
        elif mode == "LAYOUT":
            file_row(layout, "layout")
            o = selected_holder()
            layout.separator()
            if o is not None:
                layout.label(text="%s · %s" % (o["room_object"], o["room_asset"]))
                group = "maybe" if o.users_collection and o.users_collection[0].name == "Maybe" else "life"
                buttons(layout, ["life", "maybe"], group, "room.group", columns=2)
            else:
                layout.label(text="Select a thing to move it or change its group")
            layout.separator()
            row = layout.row(align=True)
            row.prop(context.window_manager, "room_asset", text="")
            row.operator("room.add", text="Add at cursor")
        elif mode == "LOOK":
            file_row(layout, "look")
            o = selected_holder()
            layout.separator()
            if o is not None:
                d = doc(o["room_asset"])
                layout.label(text="%s · %s" % (o["room_object"], o["room_asset"]))
                layout.label(text="Shape")
                buttons(layout, d["shapes"], o["room_shape"], "room.look", key="shape")
                layout.label(text="Color")
                fits = [c for c, v in d["colors"].items() if o["room_shape"] in v.get("shapes", d["shapes"])]
                buttons(layout, fits, o["room_color"], "room.look", key="color")
            else:
                layout.label(text="Select a thing to change its shape or colour")
        elif mode == "LIGHTING":
            file_row(layout, "lighting")
            layout.label(text="State")
            buttons(layout, list(S["data"]["lighting"]["states"]), S["state"], "room.state")
            row = layout.row(align=True)
            row.operator("room.state_edit", text="+ Copy").action = "copy"
            row.operator("room.state_edit", text="− Delete").action = "delete"
            row.operator("room.state_edit", text="Rename").action = "rename"
            o = selected_light()
            layout.separator()
            if o is not None:
                entry = S["data"]["lighting"]["lights"].get(o["room_light"], {})
                attach = o.parent.get("room_object") if o.parent else ""
                layout.label(text=o["room_light"] + (" · on %s" % attach if attach else ""))
                layout.label(text="Attach")
                options = [""] + [n for n in nearest_objects(o) if n != attach] + ([attach] if attach else [])
                grid = layout.grid_flow(row_major=True, columns=0, even_columns=False, align=True)
                for n in options:
                    b = grid.operator("room.attach", text=n or "none", depress=n == attach)
                    b.key, b.value = "attach", n
                if attach:
                    layout.label(text="Glow part")
                    parts = doc(o.parent["room_asset"])["parts"]
                    grid = layout.grid_flow(row_major=True, columns=0, even_columns=False, align=True)
                    for p in [""] + parts:
                        b = grid.operator("room.attach", text=p or "none", depress=p == entry.get("glowPart", ""))
                        b.key, b.value = "glow", p
                    if entry.get("glowPart"):
                        row = layout.row(align=True)
                        row.prop(o, "room_glow_strength", text="Glow")
                        row.prop(o, "room_glow_color", text="")
            else:
                layout.label(text="Select a light; strength and colour are in its light properties")
        elif mode == "CAMERAS":
            file_row(layout, "viewpoints")
            layout.label(text="Camera")
            buttons(layout, list(S["data"]["viewpoints"]), S["camera"], "room.camera")


CLASSES = [ROOM_OT_mode, ROOM_OT_scene, ROOM_OT_preset, ROOM_OT_save, ROOM_OT_revert, ROOM_OT_new,
           ROOM_OT_state, ROOM_OT_state_edit, ROOM_OT_export, ROOM_OT_camera, ROOM_OT_group, ROOM_OT_add, ROOM_OT_look,
           ROOM_OT_attach, ROOM_PT_panel]
_ASSET_ITEMS = []


def asset_items(self, context):
    _ASSET_ITEMS[:] = [(a, a, "") for a in assets()]
    return _ASSET_ITEMS


def unregister():
    for cls in reversed(CLASSES):
        if hasattr(bpy.types, cls.__name__):
            bpy.utils.unregister_class(getattr(bpy.types, cls.__name__))
    for handler in list(bpy.app.handlers.depsgraph_update_post):
        if getattr(handler, "__name__", "") == "on_update":
            bpy.app.handlers.depsgraph_update_post.remove(handler)
    bpy.msgbus.clear_by_owner(S)


def register():
    unregister()
    for cls in CLASSES:
        bpy.utils.register_class(cls)
    bpy.types.WindowManager.room_asset = bpy.props.EnumProperty(items=asset_items, name="Asset")
    bpy.types.Object.room_glow_strength = bpy.props.FloatProperty(name="Glow", min=0.0, update=glow_update)
    bpy.types.Object.room_glow_color = bpy.props.FloatVectorProperty(name="Glow colour", subtype='COLOR', size=3, min=0.0, max=1.0, update=glow_update)
    bpy.app.handlers.depsgraph_update_post.append(on_update)
    bpy.msgbus.subscribe_rna(key=(bpy.types.LayerObjects, "active"), owner=S, args=(), notify=on_active)


def start_look():
    look = scene_settings().get("start", {}).get("look")
    looks = files_of("look")
    return look if look in looks else looks[0]


def open_scene():
    load_session()
    if not S["files"]["look"]:
        S["files"]["look"] = start_look()
    load_look(S["files"]["look"])
    build_all()
    if S["camera"]:
        try:
            jump(S["camera"])
        except Exception:
            pass
    save_session()


def start():
    register()
    bpy.app.timers.register(lambda: (open_scene(), None)[1], first_interval=0.1)
