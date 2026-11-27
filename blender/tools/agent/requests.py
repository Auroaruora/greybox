import argparse, json, os, re, shutil, struct, sys, tempfile, time

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hub

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MAX_TEXTURE = 1024
PART_NAME = re.compile(r"^[a-z][A-Za-z0-9]*$")


def out(value):
    print(json.dumps(value, ensure_ascii=False, indent=1))


def fail(message):
    print(json.dumps({"error": message}), file=sys.stderr)
    sys.exit(1)


def prev_folder(asset):
    return os.path.join(tempfile.gettempdir(), "asset-requests", asset)


def backup_paths(asset, request):
    folder = prev_folder(asset)
    return {hub.blend_path(asset): os.path.join(folder, "%s-%s.blend" % (asset, request)),
            hub.doc_path(asset): os.path.join(folder, "%s-%s.json" % (asset, request))}


def textures_folder(asset):
    return os.path.join(hub.data_folder(asset), "textures")


def textures_backup(asset, request):
    return os.path.join(prev_folder(asset), "%s-%s-textures" % (asset, request))


def save_backups(asset, request):
    for source, copy in backup_paths(asset, request).items():
        if os.path.isfile(source):
            os.makedirs(os.path.dirname(copy), exist_ok=True)
            shutil.copy2(source, copy)
    if os.path.isdir(textures_folder(asset)):
        shutil.copytree(textures_folder(asset), textures_backup(asset, request), dirs_exist_ok=True)


def restore_backups(asset, request):
    restored = []
    for source, copy in backup_paths(asset, request).items():
        if os.path.isfile(copy):
            shutil.copy2(copy, source)
            restored.append(os.path.basename(source))
    saved = textures_backup(asset, request)
    if os.path.isdir(textures_folder(asset)):
        shutil.rmtree(textures_folder(asset))
    if os.path.isdir(saved):
        shutil.copytree(saved, textures_folder(asset))
    return restored


def drop_backups(asset, request):
    for copy in backup_paths(asset, request).values():
        if os.path.isfile(copy):
            os.remove(copy)
    if os.path.isdir(textures_backup(asset, request)):
        shutil.rmtree(textures_backup(asset, request))
    if os.path.isdir(prev_folder(asset)) and not os.listdir(prev_folder(asset)):
        os.rmdir(prev_folder(asset))


def surface_only(request):
    return request["kind"] == "surface" or bool(request.get("redoSurface"))


def colour_source(entry, doc, request):
    if request["kind"] in ("redo", "surface"):
        return request["variant"]
    if request.get("base"):
        return request["base"]
    active = [v for v in (doc or {}).get("shapes", []) if v in hub.active_variants(entry)]
    return active[-1] if active else ""


def working(data, asset, request_id):
    entry = hub.get_entry(data, asset)
    request = hub.get_request(entry, request_id)
    if request["status"] != "working":
        fail("request is %s, not working; claim it first" % request["status"])
    return entry, request


def get_plan(args):
    data = hub.load_list()
    rows = []
    for name, entry in data["assets"].items():
        if args.asset and name != args.asset:
            continue
        if args.level and entry["level"] != args.level:
            continue
        for request in entry.get("requests", []):
            if request["status"] == "open" and (not args.request or request["id"] == args.request):
                rows.append((request["created"], name, request["id"], entry["level"], request["kind"]))
    return [{"asset": n, "request": r, "level": l, "kind": k} for _, n, r, l, k in sorted(rows)]


def surface_materials(doc, surface):
    return [m for m in doc.get("materials", {}) if m.startswith(surface) and m[len(surface):len(surface) + 1].isupper()]


def describe_surface(asset, entry, doc, variant, surface):
    own = (doc or {}).get("materials", {})
    textures = {m: os.path.join(hub.data_folder(asset), own[m]["color"]) for m in surface_materials(doc or {}, surface)}
    info = hub.surfaces_of(entry, variant).get(surface, {})
    source = next((r for r in entry.get("requests", []) if r["id"] == info.get("from")), {})
    return {"variant": variant, "surface": surface, "name": info.get("name", ""),
            "colourSet": (doc or {}).get("colors", {}).get(surface, {}), "textures": textures,
            "requestText": source.get("text", "")}


def show(args):
    data = hub.load_list()
    entry = hub.get_entry(data, args.asset)
    request = hub.get_request(entry, args.request)
    doc = hub.get_doc(args.asset)
    active = hub.active_variants(entry)
    images = [{"path": os.path.join(hub.source_folder(args.asset), i["file"]), "shape": i["shape"], "surface": i["surface"]}
              for i in hub.get_images(request)]
    result = {
        "request": {k: v for k, v in request.items() if k != "images"},
        "images": images,
        "makes": "surface only, keep the mesh" if surface_only(request) else "variant",
        "variant": request.get("variant") or hub.next_variant(entry, doc),
        "colourFrom": request.get("colourFrom") or colour_source(entry, doc, request),
        "asset": {"id": args.asset, "area": entry["area"], "level": entry["level"], "serves": entry["serves"], "why": entry["why"],
                  "modelling": data["levels"][entry["level"]]["modelling"],
                  "hasModel": doc is not None, "blend": hub.blend_path(args.asset), "parts": doc["parts"] if doc else [],
                  "activeVariants": active,
                  "surfaces": {v: sorted(hub.surfaces_of(entry, v)) for v in active},
                  "thumbnails": {v: os.path.join(hub.CACHE, args.asset, v + ".png") for v in active}},
        "rules": os.path.join(hub.ROOT, "RULES.md"),
        "palette": hub.PALETTE,
    }
    if request.get("copyFrom"):
        c = request["copyFrom"]
        result["copyFrom"] = describe_surface(args.asset, entry, doc, c["variant"], c["surface"])
    if request.get("redoSurface"):
        result["redoSurface"] = describe_surface(args.asset, entry, doc, request["variant"], request["surface"])
    return result


def claim(args):
    with hub.editing() as data:
        entry = hub.get_entry(data, args.asset)
        request = hub.get_request(entry, args.request)
        if request["status"] != "open":
            fail("request is %s, not open" % request["status"])
        doc = hub.get_doc(args.asset)
        save_backups(args.asset, args.request)
        if request["kind"] == "new":
            request["variant"] = hub.next_variant(entry, doc)
        request["colourFrom"] = colour_source(entry, doc, request)
        request["status"] = "working"
        request["claimedAt"] = time.time()
        for key in ("textures", "newParts", "reason"):
            request.pop(key, None)
        if doc and request["kind"] == "new":
            doc["shapes"].append(request["variant"])
            for spec in doc["colors"].values():
                if "shapes" in spec and request["colourFrom"] in spec["shapes"]:
                    spec["shapes"].append(request["variant"])
            hub.set_doc(args.asset, doc)
    return {"claimed": args.request, "makes": "surface only" if surface_only(request) else "variant",
            "variant": request["variant"], "surface": request.get("surface", ""), "colourFrom": request["colourFrom"]}


def add_part(args):
    if not PART_NAME.match(args.name):
        fail("part names are camelCase")
    with hub.editing() as data:
        entry, request = working(data, args.asset, args.request)
        if surface_only(request):
            fail("a surface request keeps the mesh; it cannot add parts")
        doc = hub.get_doc(args.asset)
        if doc is None:
            fail("the asset has no asset.json yet; list its parts there when creating it")
        if args.name in doc["parts"]:
            fail("part %s already exists" % args.name)
        doc["parts"].append(args.name)
        hub.set_doc(args.asset, doc)
        request.setdefault("newParts", []).append(args.name)
    return {"parts": doc["parts"]}


def png_size(path):
    with open(path, "rb") as f:
        head = f.read(24)
    if len(head) < 24 or head[:8] != PNG_SIGNATURE:
        fail("texture must be a PNG file")
    return struct.unpack(">II", head[16:24])


def add_texture(args):
    width, height = png_size(args.png)
    if max(width, height) > MAX_TEXTURE:
        fail("texture is %dx%d; at most %d px" % (width, height, MAX_TEXTURE))
    with hub.editing() as data:
        entry, request = working(data, args.asset, args.request)
        doc = hub.get_doc(args.asset)
        if doc is None or args.part not in doc["parts"]:
            fail("unknown part %s" % args.part)
        surface = request.get("surface") or hub.next_surface(doc, request["variant"])
        request["surface"] = surface
        material = hub.camel(surface, args.part, "" if args.slot == 0 else "slot%d" % (args.slot + 1))
        file = "textures/%s.png" % material
        os.makedirs(textures_folder(args.asset), exist_ok=True)
        shutil.copyfile(args.png, os.path.join(hub.data_folder(args.asset), file))
        doc.setdefault("materials", {})[material] = {"color": file}
        hub.set_doc(args.asset, doc)
        textures = [t for t in request.get("textures", []) if not (t["part"] == args.part and t["slot"] == args.slot)]
        textures.append({"part": args.part, "slot": args.slot, "material": material})
        request["textures"] = textures
    return {"surface": surface, "material": material, "file": os.path.join(hub.data_folder(args.asset), file),
            "next": "In the .blend make a material named %s whose Image Texture node loads %s, with use_fake_user on. Do not put it on a mesh slot." % (material, hub.blend_relative(args.asset, file))}


def built_slots(asset, variant, since):
    path = os.path.join(hub.CACHE, asset, "manifest.json")
    if not os.path.isfile(path) or os.path.getmtime(path) < since:
        fail("run export_review.py for %s after building" % asset)
    slots = hub.read_json(path)["variants"].get(variant, {}).get("slots")
    if slots is None:
        fail("variant %s was not exported" % variant)
    return slots


def write_surface(asset, request, slots):
    doc = hub.get_doc(asset)
    surface = request["surface"]
    spec = {"shapes": [request["variant"]]}
    for part, names in slots.items():
        names = list(names)
        for t in request.get("textures", []):
            if t["part"] == part and t["slot"] < len(names):
                names[t["slot"]] = t["material"]
        spec[part] = names[0] if len(names) == 1 else names
    doc["colors"][surface] = spec
    keep = {t["material"] for t in request.get("textures", [])}
    for material in surface_materials(doc, surface):
        if material not in keep:
            path = os.path.join(hub.data_folder(asset), doc["materials"][material]["color"])
            if os.path.isfile(path):
                os.remove(path)
            del doc["materials"][material]
    hub.set_doc(asset, doc)
    return surface


def finish(args):
    with hub.editing() as data:
        entry, request = working(data, args.asset, args.request)
        if args.variant != request.get("variant"):
            fail("this request builds variant %s" % request.get("variant"))
        slots = built_slots(args.asset, args.variant, request.get("claimedAt", 0))
        info = entry.setdefault("variants", {}).setdefault(args.variant, {"review": ""})
        result = {"variant": args.variant}
        if not surface_only(request):
            info["review"] = ""
            info["from"] = args.request
            for surface in info.get("surfaces", {}).values():
                surface["review"] = ""
        if request.get("textures"):
            surface = write_surface(args.asset, request, slots)
            record = info.setdefault("surfaces", {}).setdefault(surface, {})
            record.update({"name": args.name or record.get("name") or "Surface", "review": "", "from": args.request})
            result["surface"] = surface
        elif surface_only(request):
            fail("this request makes a surface; register at least one texture first")
        request["status"] = "done"
        request["result"] = result
        request["finished"] = hub.today()
        request.pop("claimedAt", None)
        if args.note:
            request["note"] = args.note
    drop_backups(args.asset, args.request)
    return {"done": args.request, **result}


def fail_request(args):
    with hub.editing() as data:
        entry = hub.get_entry(data, args.asset)
        request = hub.get_request(entry, args.request)
        if request["status"] not in ("open", "working"):
            fail("request is %s" % request["status"])
        request["status"] = "failed"
        request["reason"] = args.reason
        for key in ("colourFrom", "claimedAt", "textures", "newParts"):
            request.pop(key, None)
        if request["kind"] == "new":
            request.pop("variant", None)
        if not request.get("redoSurface"):
            request.pop("surface", None)
    restored = restore_backups(args.asset, args.request)
    drop_backups(args.asset, args.request)
    return {"failed": args.request, "restored": restored}


def main():
    parser = argparse.ArgumentParser(description="Read and update asset requests in asset_list.json")
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("plan", help="open requests to handle, oldest first; filters are optional")
    p.add_argument("--level", choices=hub.LEVELS)
    p.add_argument("--asset")
    p.add_argument("--request")
    for name in ("show", "claim"):
        p = sub.add_parser(name)
        p.add_argument("asset")
        p.add_argument("request")
    p = sub.add_parser("part", help="add a part name to the asset")
    p.add_argument("asset")
    p.add_argument("request")
    p.add_argument("name")
    p = sub.add_parser("texture", help="register a texture PNG for one part slot of this request's surface")
    p.add_argument("asset")
    p.add_argument("request")
    p.add_argument("part")
    p.add_argument("png")
    p.add_argument("--slot", type=int, default=0, help="material slot index, 0 for the first")
    p = sub.add_parser("finish")
    p.add_argument("asset")
    p.add_argument("request")
    p.add_argument("variant")
    p.add_argument("--name", default="", help="two or three words naming the surface, when one was made")
    p.add_argument("--note", default="")
    p = sub.add_parser("fail")
    p.add_argument("asset")
    p.add_argument("request")
    p.add_argument("reason")
    args = parser.parse_args()
    commands = {"plan": get_plan, "show": show, "claim": claim, "part": add_part, "texture": add_texture, "finish": finish, "fail": fail_request}
    try:
        out(commands[args.command](args))
    except KeyError as error:
        fail(str(error.args[0]))


if __name__ == "__main__":
    main()
