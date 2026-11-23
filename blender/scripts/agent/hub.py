import json, os, re, fcntl, datetime
from contextlib import contextmanager

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ASSETS = os.path.join(ROOT, "assets")
LIST = os.path.join(ROOT, "asset_list.json")
PALETTE = os.path.join(ASSETS, "materials", "materials.json")
PRESETS = os.path.join(ROOT, "presets")
LAYOUTS = os.path.join(ROOT, "layouts")
CACHE = os.path.join(ROOT, "review", "cache")
BLENDER = "/Applications/Blender.app/Contents/MacOS/Blender"
ID = re.compile(r"^[a-z][A-Za-z0-9]*$")
STATUSES = ("room", "idea", "shelved")
LEVELS = ("L1", "L2", "L3", "L4")
OPEN = ("open", "working")
IMAGE_TYPES = {".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".png": "image/png", ".webp": "image/webp"}


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
    short = level > 0 and len(json.dumps(v, ensure_ascii=False, separators=(", ", ": "))) <= 150
    if isinstance(v, dict) and v and not (level > 0 and (flat(v) or short)):
        items = ["%s  %s: %s" % (pad, json.dumps(k, ensure_ascii=False), write_json(x, level + 1)) for k, x in v.items()]
        return "{\n" + ",\n".join(items) + "\n" + pad + "}"
    if isinstance(v, list) and v and not (flat(v) or short):
        items = ["%s  %s" % (pad, write_json(x, level + 1)) for x in v]
        return "[\n" + ",\n".join(items) + "\n" + pad + "]"
    return json.dumps(v, ensure_ascii=False, separators=(", ", ": "))


def read_json(path):
    with open(path) as f:
        return json.load(f)


def save_json(path, data):
    temporary = path + ".tmp"
    with open(temporary, "w") as f:
        f.write(write_json(clean(data)) + "\n")
    os.replace(temporary, path)


@contextmanager
def locked():
    handle = os.open(ROOT, os.O_RDONLY)
    try:
        fcntl.flock(handle, fcntl.LOCK_EX)
        yield
    finally:
        fcntl.flock(handle, fcntl.LOCK_UN)
        os.close(handle)


def load_list():
    return read_json(LIST)


@contextmanager
def editing():
    with locked():
        data = load_list()
        yield data
        save_json(LIST, data)


def today():
    return datetime.date.today().isoformat()


def asset_folder(asset):
    return os.path.join(ASSETS, asset)


def doc_path(asset):
    return os.path.join(ASSETS, asset, "asset.json")


def blend_path(asset):
    return os.path.join(ASSETS, asset, asset + ".blend")


def get_doc(asset):
    path = doc_path(asset)
    return read_json(path) if os.path.isfile(path) else None


def set_doc(asset, doc):
    save_json(doc_path(asset), doc)


def get_palette():
    return read_json(PALETTE)


def get_entry(data, asset):
    entry = data["assets"].get(asset)
    if entry is None:
        raise KeyError("unknown asset " + asset)
    return entry


def active_variants(entry):
    return [v for v, info in entry.get("variants", {}).items() if not info.get("retired")]


def next_variant(entry, doc):
    used = set(entry.get("variants", {})) | set((doc or {}).get("shapes", []))
    letter = 0
    while True:
        name = ""
        n = letter
        while True:
            name = chr(ord("a") + n % 26) + name
            n = n // 26 - 1
            if n < 0:
                break
        if name not in used:
            return name
        letter += 1


def get_request(entry, request):
    for item in entry.get("requests", []):
        if item["id"] == request:
            return item
    raise KeyError("unknown request " + request)


def new_request_id(entry):
    numbers = [int(r["id"][1:]) for r in entry.get("requests", []) if r["id"][1:].isdigit()]
    return "r%03d" % (max(numbers, default=0) + 1)


def colour_sets_for(doc, variant):
    return [name for name, spec in doc.get("colors", {}).items() if variant in spec.get("shapes", doc.get("shapes", []))]


def presets():
    return sorted(f[:-5] for f in os.listdir(PRESETS) if f.endswith(".json") and not f.startswith(("_", ".")))


def get_usage(asset, doc):
    usage = {}
    layouts = {}
    for name in presets():
        preset = read_json(os.path.join(PRESETS, name + ".json"))
        layout_name = preset.get("layout", "main")
        if layout_name not in layouts:
            layouts[layout_name] = read_json(os.path.join(LAYOUTS, layout_name + ".json"))["objects"]
        for obj, info in layouts[layout_name].items():
            if info["asset"] != asset:
                continue
            entry = preset["objects"].get(obj, {})
            shape = entry.get("shape", doc["shapes"][0] if doc and doc.get("shapes") else "")
            sets = colour_sets_for(doc, shape) if doc else []
            colour = entry.get("color", sets[0] if sets else "")
            usage.setdefault(name, []).append({"object": obj, "variant": shape, "colour": colour})
    return usage


def replace_variant_in_presets(asset, doc, old, new):
    changed = []
    sets = colour_sets_for(doc, new)
    for name, uses in get_usage(asset, doc).items():
        hits = [u for u in uses if u["variant"] == old]
        if not hits:
            continue
        path = os.path.join(PRESETS, name + ".json")
        preset = read_json(path)
        for use in hits:
            entry = preset["objects"].setdefault(use["object"], {})
            entry["shape"] = new
            entry["color"] = use["colour"] if use["colour"] in sets else (sets[0] if sets else use["colour"])
        save_json(path, preset)
        changed.append(name)
    return changed


def get_images(request):
    return [i if isinstance(i, dict) else {"file": i, "shape": True, "surface": True} for i in request.get("images", [])]


def surfaces_of(entry, variant):
    return entry.get("variants", {}).get(variant, {}).get("surfaces", {})


def next_surface(doc, variant):
    n = 1
    while "%sSurface%d" % (variant, n) in doc.get("colors", {}):
        n += 1
    return "%sSurface%d" % (variant, n)


def camel(*words):
    text = "".join(w[:1].upper() + w[1:] for w in words if w)
    return text[:1].lower() + text[1:]
