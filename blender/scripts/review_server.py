import json, os, sys, base64, re
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, unquote

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "agent"))
import hub

PAGE = os.path.join(hub.ROOT, "review")
TYPES = {".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8", ".css": "text/css; charset=utf-8",
         ".png": "image/png", ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".webp": "image/webp",
         ".glb": "model/gltf-binary", ".json": "application/json"}
MAX_BODY = 120 * 1024 * 1024
MAX_IMAGE = 15 * 1024 * 1024
MAX_IMAGES = 8
SAFE_FILE = re.compile(r"^[A-Za-z0-9_.-]+$")


class Problem(Exception):
    pass


def manifest(asset):
    path = os.path.join(hub.CACHE, asset, "manifest.json")
    return hub.read_json(path) if os.path.isfile(path) else {"variants": {}, "problems": []}


def live_redos(entry, variant, surface=""):
    return [r for r in entry.get("requests", []) if r["kind"] == "redo" and r.get("variant") == variant
            and r.get("surface", "") == surface and bool(r.get("redoSurface")) == bool(surface)
            and r["status"] in ("open", "working", "failed")]


def review_state(entry, info, variant, surface=""):
    redo = live_redos(entry, variant, surface)
    if redo:
        return "redoImage" if any(r.get("images") for r in redo) else "redo"
    return "ok" if info.get("review") == "ok" else "new"


def variant_state(entry, variant):
    return review_state(entry, entry["variants"][variant], variant)


def thumb(asset, variant, built):
    return "/cache/%s/%s.png" % (asset, variant) if variant in built else ""


def summary(name, entry):
    built = manifest(name)["variants"]
    active = hub.active_variants(entry)
    return {"id": name, "status": entry["status"], "level": entry["level"], "serves": entry["serves"], "area": entry["area"],
            "why": entry["why"],
            "variants": [{"id": v, "state": variant_state(entry, v), "thumb": thumb(name, v, built)} for v in active],
            "openRequests": sum(1 for r in entry.get("requests", []) if r["status"] in hub.OPEN)}


def get_list():
    data = hub.load_list()
    areas = sorted({e["area"] for e in data["assets"].values()})
    return {"serves": data["serves"], "levels": data["levels"], "areas": areas,
            "assets": [summary(n, e) for n, e in sorted(data["assets"].items())]}


def surface_label(entry, variant, surface):
    return hub.surfaces_of(entry, variant).get(surface, {}).get("name", surface)


def request_title(entry, r):
    if r["kind"] == "new":
        return "New variant from " + r["base"] if r.get("base") else "New variant"
    if r["kind"] == "redo":
        return "Redo %s · %s" % (r["variant"], surface_label(entry, r["variant"], r["surface"])) if r.get("redoSurface") else "Redo " + r["variant"]
    if r.get("copyFrom"):
        c = r["copyFrom"]
        return "Copy %s (%s) to %s" % (surface_label(entry, c["variant"], c["surface"]), c["variant"], r["variant"])
    return "New surface for " + r["variant"]


def request_result(entry, r):
    res = r.get("result")
    if not isinstance(res, dict):
        return None
    variant = entry.get("variants", {}).get(res["variant"], {})
    info = variant.get("surfaces", {}).get(res["surface"], {}) if res.get("surface") else variant
    label = "variant " + res["variant"] + (" · " + surface_label(entry, res["variant"], res["surface"]) if res.get("surface") else "")
    return {"variant": res["variant"], "surface": res.get("surface", ""), "label": label,
            "needsReview": info.get("review") != "ok" and not info.get("retired") and not variant.get("retired")}


def get_requests():
    data = hub.load_list()
    rows = []
    for name, entry in data["assets"].items():
        built = manifest(name)["variants"]
        first = next((v for v in hub.active_variants(entry) if v in built), "")
        for r in entry.get("requests", []):
            rows.append({"asset": name, "area": entry["area"], "level": entry["level"], "thumb": thumb(name, first, built) if first else "",
                         "id": r["id"], "kind": r["kind"], "status": r["status"], "title": request_title(entry, r), "text": r.get("text", ""),
                         "images": hub.get_images(r), "created": r.get("created", ""), "finished": r.get("finished", ""),
                         "reason": r.get("reason", ""), "note": r.get("note", ""), "result": request_result(entry, r)})
    areas = sorted({e["area"] for e in data["assets"].values()})
    return {"levels": data["levels"], "areas": areas, "requests": rows}


def get_asset(name):
    data = hub.load_list()
    entry = hub.get_entry(data, name)
    doc = hub.get_doc(name)
    built = manifest(name)
    usage = hub.get_usage(name, doc) if doc else {}
    variants = []
    for v, info in entry.get("variants", {}).items():
        looks = sorted(p for p, uses in usage.items() if any(u["variant"] == v for u in uses))
        surfaces = []
        for sid, sinfo in info.get("surfaces", {}).items():
            surface_looks = sorted(p for p, uses in usage.items() if any(u["colour"] == sid for u in uses))
            surfaces.append({"id": sid, "name": sinfo.get("name", sid), "state": review_state(entry, sinfo, v, sid),
                             "retired": bool(sinfo.get("retired")), "from": sinfo.get("from", ""), "looks": surface_looks})
        variants.append({"id": v, "state": variant_state(entry, v), "retired": bool(info.get("retired")), "from": info.get("from", ""),
                         "thumb": thumb(name, v, built["variants"]),
                         "glb": "/cache/%s/%s.glb" % (name, v) if v in built["variants"] else "", "looks": looks, "surfaces": surfaces})
    return {"id": name, "entry": entry, "levels": data["levels"], "serves": data["serves"],
            "doc": doc, "variants": variants, "problems": built["problems"],
            "palette": hub.get_palette()}


def text(value, field, required=False):
    if not isinstance(value, str):
        raise Problem(field + " must be text")
    value = value.strip()
    if required and not value:
        raise Problem(field + " is needed")
    return value


def set_fields(name, body):
    with hub.editing() as data:
        entry = hub.get_entry(data, name)
        status = body.get("status")
        if status is not None and status != entry["status"]:
            if status not in hub.STATUSES:
                raise Problem("bad status")
            entry["status"] = status
        for key, choices in (("level", hub.LEVELS), ("serves", tuple(data["serves"]))):
            value = body.get(key)
            if value is not None and value != entry[key]:
                if value not in choices:
                    raise Problem("bad " + key)
                entry[key] = value
        if "why" in body:
            entry["why"] = text(body["why"], "why", required=True)
        return summary(name, entry)


def add_asset(body):
    name = text(body.get("id", ""), "id", required=True)
    if not hub.ID.match(name):
        raise Problem("id is camelCase letters and digits, starting lower case")
    with hub.editing() as data:
        if name in data["assets"] or os.path.exists(hub.asset_folder(name)):
            raise Problem(name + " already exists")
        level, serves = body.get("level"), body.get("serves")
        if level not in hub.LEVELS or serves not in data["serves"]:
            raise Problem("pick a level and what it serves")
        entry = {"status": "idea", "level": level, "serves": serves, "area": text(body.get("area", ""), "area", required=True),
                 "why": text(body.get("why", ""), "why", required=True), "variants": {}, "requests": []}
        data["assets"][name] = entry
        data["assets"] = dict(sorted(data["assets"].items()))
        return summary(name, entry)


def decode_image(image):
    ext = os.path.splitext(str(image.get("name", "")))[1].lower()
    if ext not in hub.IMAGE_TYPES:
        raise Problem("images must be jpg, png or webp")
    try:
        raw = base64.b64decode(image.get("data", ""), validate=True)
    except ValueError:
        raise Problem("bad image data")
    if len(raw) > MAX_IMAGE:
        raise Problem("an image is over 15 MB")
    return (".jpg" if ext == ".jpeg" else ext), raw


def read_images(images, old_files):
    if not isinstance(images, list) or len(images) > MAX_IMAGES:
        raise Problem("at most %d images" % MAX_IMAGES)
    result = []
    for image in images:
        if not isinstance(image, dict):
            raise Problem("bad image")
        shape, surface = bool(image.get("shape")), bool(image.get("surface"))
        if not shape and not surface:
            raise Problem("each image is for shape, surface or both")
        if image.get("file"):
            if image["file"] not in old_files:
                raise Problem("unknown image " + str(image["file"]))
            result.append({"file": image["file"], "shape": shape, "surface": surface})
        else:
            ext, raw = decode_image(image)
            result.append({"raw": raw, "ext": ext, "shape": shape, "surface": surface})
    return result


def request_fields(entry, body, old_files):
    kind = body.get("kind")
    if kind not in ("new", "redo", "surface"):
        raise Problem("pick new variant, redo or new surface")
    description = text(body.get("text", ""), "text")
    active = hub.active_variants(entry)
    fields = {"kind": kind, "text": description}
    if kind == "new":
        base = body.get("base", "")
        if base and base not in active:
            raise Problem("unknown variant to start from")
        if base:
            fields["base"] = base
    else:
        variant = body.get("variant", "")
        if variant not in active:
            raise Problem("pick a variant")
        fields["variant"] = variant
        live = [s for s, i in hub.surfaces_of(entry, variant).items() if not i.get("retired")]
        surface = body.get("surface", "")
        if kind == "redo" and surface:
            if surface not in live:
                raise Problem("unknown surface")
            fields["surface"] = surface
            fields["redoSurface"] = True
        if kind == "surface" and body.get("copyFrom"):
            source = body["copyFrom"]
            if not isinstance(source, dict) or source.get("surface") not in hub.surfaces_of(entry, source.get("variant", "")):
                raise Problem("unknown surface to copy")
            fields["copyFrom"] = {"variant": source["variant"], "surface": source["surface"]}
    images = [] if fields.get("copyFrom") else read_images(body.get("images", []), old_files)
    if kind == "surface" or fields.get("redoSurface"):
        images = [dict(i, shape=False, surface=True) for i in images]
    if not description and not images and not fields.get("copyFrom"):
        raise Problem("write a description or add an image")
    return fields, images


def save_request_images(name, request_id, images, old_files):
    folder = os.path.join(hub.asset_folder(name), "refs")
    taken = {os.path.basename(f) for f in old_files}
    n = 1
    stored = []
    for image in images:
        if "raw" in image:
            while any(t.startswith("%s-%d." % (request_id, n)) for t in taken):
                n += 1
            file = "refs/%s-%d%s" % (request_id, n, image["ext"])
            taken.add(os.path.basename(file))
            os.makedirs(folder, exist_ok=True)
            with open(os.path.join(hub.asset_folder(name), file), "wb") as f:
                f.write(image["raw"])
            image = {"file": file, "shape": image["shape"], "surface": image["surface"]}
        stored.append(image)
    for old in old_files:
        if old not in {i["file"] for i in stored}:
            path = os.path.join(hub.asset_folder(name), old)
            if os.path.isfile(path):
                os.remove(path)
    return stored


def add_request(name, body):
    with hub.editing() as data:
        entry = hub.get_entry(data, name)
        fields, images = request_fields(entry, body, [])
        request_id = hub.new_request_id(entry)
        request = {"id": request_id, "status": "open", "created": hub.today(), **fields}
        request["images"] = save_request_images(name, request_id, images, [])
        entry.setdefault("requests", []).append(request)
        return request


def edit_request(name, request_id, body):
    with hub.editing() as data:
        entry = hub.get_entry(data, name)
        request = hub.get_request(entry, request_id)
        if request["status"] not in ("open", "failed"):
            raise Problem("only open or failed requests can be edited")
        old_files = [i["file"] for i in hub.get_images(request)]
        fields, images = request_fields(entry, body, old_files)
        for key in ("kind", "text", "base", "variant", "surface", "redoSurface", "copyFrom", "reason"):
            request.pop(key, None)
        request.update(fields)
        request["status"] = "open"
        request["edited"] = hub.today()
        request["images"] = save_request_images(name, request_id, images, old_files)
        return request


def change_request(name, request_id, body):
    action = body.get("action")
    if action == "edit":
        return edit_request(name, request_id, body)
    with hub.editing() as data:
        entry = hub.get_entry(data, name)
        request = hub.get_request(entry, request_id)
        if action == "cancel" and request["status"] in ("open", "failed"):
            request["status"] = "cancelled"
        elif action == "reopen" and request["status"] == "failed":
            request["status"] = "open"
            request.pop("reason", None)
        else:
            raise Problem("can't %s a %s request" % (action, request["status"]))
        return request


def change_surface(name, variant, surface, body):
    action = body.get("action")
    with hub.editing() as data:
        entry = hub.get_entry(data, name)
        info = hub.surfaces_of(entry, variant).get(surface)
        if info is None:
            raise Problem("unknown surface")
        if action == "ok":
            info["review"] = "ok"
        elif action == "unreview":
            info["review"] = ""
        elif action == "retire":
            doc = hub.get_doc(name)
            looks = sorted(p for p, uses in hub.get_usage(name, doc).items() if any(u["colour"] == surface for u in uses)) if doc else []
            if looks:
                raise Problem("looks %s use this surface; switch them first" % ", ".join(looks))
            info["retired"] = True
        elif action == "restore":
            info.pop("retired", None)
        elif action == "rename":
            info["name"] = text(body.get("name", ""), "name", required=True)
        else:
            raise Problem("unknown action")
        return info


def change_variant(name, variant, body):
    action = body.get("action")
    with hub.editing() as data:
        entry = hub.get_entry(data, name)
        info = entry.get("variants", {}).get(variant)
        if info is None:
            raise Problem("unknown variant")
        if action == "ok":
            info["review"] = "ok"
        elif action == "unreview":
            info["review"] = ""
        elif action == "retire":
            if info.get("retired"):
                raise Problem("already retired")
            doc = hub.get_doc(name)
            used = any(u["variant"] == variant for uses in hub.get_usage(name, doc).values() for u in uses) if doc else False
            replace = body.get("replace", "")
            changed = []
            if used:
                if replace == variant or replace not in hub.active_variants(entry):
                    raise Problem("pick a variant for the looks to use instead")
                changed = hub.replace_variant_in_presets(name, doc, variant, replace)
            info["retired"] = True
        elif action == "restore":
            if not info.get("retired"):
                raise Problem("not retired")
            info.pop("retired", None)
        else:
            raise Problem("unknown action")
        return info


class Handler(BaseHTTPRequestHandler):
    def send(self, code, body, kind):
        self.send_response(code)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def send_json(self, code, value):
        self.send(code, json.dumps(value, ensure_ascii=False).encode(), "application/json; charset=utf-8")

    def send_file(self, root, relative):
        file = os.path.normpath(os.path.join(root, relative))
        kind = TYPES.get(os.path.splitext(file)[1].lower())
        if not file.startswith(root + os.sep) or not os.path.isfile(file) or kind is None:
            return self.send(404, b"not found", "text/plain")
        with open(file, "rb") as f:
            self.send(200, f.read(), kind)

    def do_GET(self):
        parts = [unquote(p) for p in urlparse(self.path).path.split("/") if p]
        try:
            if parts[:2] == ["api", "list"]:
                return self.send_json(200, get_list())
            if parts[:2] == ["api", "requests"]:
                return self.send_json(200, get_requests())
            if parts[:2] == ["api", "asset"] and len(parts) == 3:
                return self.send_json(200, get_asset(parts[2]))
        except KeyError as error:
            return self.send_json(404, {"error": str(error.args[0])})
        if parts[:1] == ["cache"] and len(parts) == 3:
            return self.send_file(hub.CACHE, os.path.join(parts[1], parts[2]))
        if parts[:1] == ["files"] and len(parts) == 4 and parts[2] in ("refs", "textures") and all(SAFE_FILE.match(p) for p in parts[1:]):
            return self.send_file(hub.ASSETS, os.path.join(*parts[1:]))
        return self.send_file(PAGE, "index.html" if not parts else os.path.join(*parts))

    def do_POST(self):
        parts = [unquote(p) for p in urlparse(self.path).path.split("/") if p]
        length = int(self.headers.get("Content-Length", 0))
        if length > MAX_BODY:
            return self.send_json(413, {"error": "too large"})
        try:
            body = json.loads(self.rfile.read(length) or b"{}")
            if not isinstance(body, dict):
                raise Problem("bad request")
            if parts == ["api", "assets"]:
                return self.send_json(200, add_asset(body))
            if parts[:2] != ["api", "asset"] or len(parts) < 3:
                return self.send_json(404, {"error": "not found"})
            name, rest = parts[2], parts[3:]
            if not rest:
                return self.send_json(200, set_fields(name, body))
            if rest == ["requests"]:
                return self.send_json(200, add_request(name, body))
            if rest[0] == "requests" and len(rest) == 2:
                return self.send_json(200, change_request(name, rest[1], body))
            if rest[0] == "variants" and len(rest) == 2:
                return self.send_json(200, change_variant(name, rest[1], body))
            if rest[0] == "variants" and len(rest) == 4 and rest[2] == "surfaces":
                return self.send_json(200, change_surface(name, rest[1], rest[3], body))
            return self.send_json(404, {"error": "not found"})
        except ValueError:
            return self.send_json(400, {"error": "bad json"})
        except KeyError as error:
            return self.send_json(404, {"error": str(error.args[0])})
        except Problem as error:
            return self.send_json(400, {"error": str(error)})

    def log_message(self, *args):
        pass


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print("http://127.0.0.1:%d" % port, flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
