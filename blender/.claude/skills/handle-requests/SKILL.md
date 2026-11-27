---
name: handle-requests
description: Make or redo asset variants and surfaces from the open requests on the asset review page. Use for "/handle-requests", "/handle-requests L4", "/handle-requests mug r003", or when asked to work through asset requests.
---

# Handle requests

Usage: `/handle-requests [L1|L2|L3|L4] [asset [request]]`. With no arguments, handle every open request, whatever its level. Arguments only narrow the list.

Read `RULES.md` once before the first request; it says what a finished model must be.
`B` below is `/Applications/Blender.app/Contents/MacOS/Blender`.

## Run

1. `python3 tools/agent/requests.py plan [--level L] [--asset id] [--request rNNN]` lists the open requests, oldest first. Handle the whole list in order and nothing else.
2. For each request, do the steps below. A failure on one request does not stop the others.
3. At the end, one line per request: asset, request, what was made or why it failed. Nothing else.

## Request kinds

- **new**: a new variant (a new shape). With surface images or words asking for one, also a surface for it.
- **redo**: rebuild a variant's shape; or, when `redoSurface` is set, rebuild that surface's textures only.
- **surface**: a new surface for an existing variant. With `copyFrom`, recreate that surface's textures on this variant.

A surface-only request (`surface`, or `redo` with `redoSurface`) never changes the mesh except adding UVs to untextured parts.

## One request

You make the model and textures. `requests.py` handles status, backups and `asset.json`.

1. **Claim**: `python3 tools/agent/requests.py claim <asset> <request>`
   - Sets the request to `working`.
   - Copies the `.blend`, `asset.json` and `textures/` to a temp folder outside the project.
   - Picks `variant`, the letter to build or texture, and `colourFrom`.
   - For a new variant, adds the letter to `asset.json` and gives it a colour set.
2. **Show**: `python3 tools/agent/requests.py show <asset> <request>`
   - Read the text. Look at every image: `shape` images for the model, `surface` images for textures; an image can be both.
   - With `copyFrom` or `redoSurface`, look at that surface's textures and the words of the request that made it.
   - Size, parts, colours and brands follow RULES.md.
3. **Build**: run a Python script with `B -b models/<id>/model.blend --python <script>`. Keep the script in the scratchpad.
   - Redo of a variant: delete the objects and meshes in collection `<variant>`, build new ones there. Keep nothing of the old version.
   - New variant: add collection `<variant>`. With a base, start from a copy of the base's objects.
   - Surface-only: leave the mesh as it is.
   - New part: `python3 tools/agent/requests.py part <asset> <request> <name>` before using it.
   - Mesh colours: follow RULES.md › Colour, in order. Never choose colours outside it.
   - Detail: the level's `modelling` line.
   - No model yet (`hasModel` false): create `models/<id>/model.blend` and `../assets/models/<id>/asset.json` with `parts`, `shapes`, and one colour set `{"*": "<palette material>"}`.
   - Save without a `.blend1`: set `bpy.context.preferences.filepaths.save_version = 0`, then `bpy.ops.wm.save_mainfile()`.
4. **Textures** (only when the request needs a surface):
   - Draw each PNG in the scratchpad, then `python3 tools/agent/requests.py texture <asset> <request> <part> <png> [--slot N]`.
   - It returns a material name. In the `.blend`, make a material with that name whose Image Texture node loads the returned file, set `use_fake_user`, and save. Do not put it on a mesh slot.
5. **Check**: `B -b models/<id>/model.blend --python tools/agent/check.py -- <id> <variant>`
   - Fix every error, then check again.
   - Still failing after two fixes: `python3 tools/agent/requests.py fail <asset> <request> "<reason>"`. This restores the `.blend`, `asset.json` and textures, and deletes the copies. Go to the next request.
6. **Export**: `B -b --factory-startup --python tools/agent/export_review.py -- <id>`
   - Compare `review/cache/<id>/<variant>.png` with the request's images.
   - If the shape plainly misses the request, fix it and go back to step 5. At most twice.
7. **Finish**: `python3 tools/agent/requests.py finish <asset> <request> <variant> [--name "<surface name>"] [--note "<text>"]`
   - Builds the surface from what was exported plus the registered textures.
   - `--name`: two or three words for a new surface, such as "Bear sticker".
   - `--note`: new parts, textures made, colours picked for new parts, and any compromise.
   - Sets the request to `done` and deletes the temp copies.

## Never

- Edit `asset_list.json` by hand; only `requests.py` changes it.
- Edit `asset.json` of an asset that already has a model, or touch other assets, scene data (layouts, looks, lighting, viewpoints) or the palette.
- Mark a variant or surface ok, or retire one. Review is the user's.
- Render, commit, or start the review server.
