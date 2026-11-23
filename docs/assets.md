# Assets

Everything about the room's models lives in `blender/`. The game never reads it directly; `blender/scripts/export_room.py` turns it into `assets/` (see [setup.md](setup.md#export-from-blender)).

## Words

| Word | Meaning |
|---|---|
| **Asset** | One kind of thing in the room, named by a camelCase id (`deskLamp`). The id is used by files, layouts, presets and game code, so it never changes. |
| **Status** | `room` (placed in the layout), `idea` (not decided) or `shelved` (decided against for now). Nothing is deleted. |
| **Level** | How much an asset matters, from L1 (clicked or focused on) to L4 (background). Each level has one line saying how detailed its model should be. |
| **Variant** | One shape of an asset, named by a letter. |
| **Surface** | An extra look of one variant, with textures on some parts (a sticker on a mug). A variant's default look is the materials on its mesh. |
| **Look** | A whole-room style (`greybox`, `toon`, `retro`, …): for every object, which variant and colours it uses. One file per look in `blender/presets/`. |

## Files

| What | Where |
|---|---|
| Status, level, why, variants, surfaces, requests | `blender/asset_list.json` |
| An asset's model (one collection per variant), parts, colour sets, textures, request images | `blender/assets/<id>/` |
| Shared palette | `blender/assets/materials/` |
| Where things stand, looks, lights, cameras | `blender/layouts/`, `blender/presets/`, `blender/lighting/`, `blender/cameras/` |
| What every model must be | `blender/RULES.md` |
| The AI agent's instructions | `blender/.claude/skills/handle-requests/SKILL.md` |

## Review page

```bash
python3 blender/scripts/review_server.py
```

Open http://127.0.0.1:8765. It only runs on your computer.

- **Assets**: every asset by status (In room, Ideas, Shelved), with filters. The side panel changes status, level and why; `+ Add asset` starts a new idea.
- **An asset**: open one to see it in 3D (Blender controls: drag or middle-drag to orbit, Shift-drag to pan, scroll to zoom, 1 3 7 for front, side and top). Its variants and surfaces are below the view, each with a review dot and a `⋯` menu (Mark ok, Redo, New surface, Copy to variant, Retire).
- **Requests**: every request across assets, grouped Open, Working, Failed, Done. `Review →` jumps to what a request made.

The 3D view and thumbnails come from `blender/review/cache/`, which is not in git. Make it on a new computer, and after changing a `.blend` by hand:

```bash
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python blender/scripts/agent/export_review.py
```

## Asking for a model

On an asset's page, `+ New request`:

- **New variant**: a new shape, optionally starting from an existing variant.
- **Redo**: rebuild a variant's shape, or only a surface's textures.
- **New surface**: textures for an existing variant, from images or copied from another surface.

Write what you want, drop or paste images, or both. Images go in two boxes: **Shape** (what to build) and **Surface** (what to paint on it). Drag a picture from one box to the other to use it for both. Open and failed requests can be edited.

You do not need to say size, parts or colours. Without them the agent uses the real size of what the images show, adds the parts the design needs, copies colours from an existing variant, leaves glass open and never adds real brands. The full list is in `blender/RULES.md`.

## The AI agent

Open a Claude Code session in the `blender/` folder, then:

```
/handle-requests
```

This handles every open request, oldest first. Narrow it with a level or one request: `/handle-requests L4`, `/handle-requests printer r001`.

For each request the agent:

1. **Claims** it, which copies the asset's files to a temp folder outside the project.
2. **Reads** the words and images.
3. **Builds** the model in Blender in the background, or only the textures for a surface request.
4. **Checks** it with `blender/scripts/agent/check.py` (names, origin, materials, UVs) and fixes what fails. After two failed fixes it gives up, restores the files and marks the request failed with the reason.
5. **Exports** it for the review page and compares it with the images.
6. **Finishes**: the request becomes done and the new variant or surface waits for your review.

It ends with one line per request. It never edits `asset_list.json` by hand, touches other assets, looks, layouts, lights or the palette, marks anything ok, retires anything, or commits.

The agent's tools are in `blender/scripts/agent/`: `requests.py` (the only way requests change), `check.py`, `export_review.py` and `hub.py` (shared code).

## Reviewing

Open the result (from Requests, `Review →`), look at it in 3D, then from its `⋯` menu:

- **Mark ok** when it is right.
- **Redo…** with a sentence or an image when it is not. The new version replaces the old one.
- **Retire** to hide it. A variant a look still uses asks which variant the look should switch to.

## Editing the room in Blender

Open `blender/room.blend`. The **Room** tab in the 3D view sidebar (`N`) builds the room from the JSON files and saves your changes back: where things stand, which look is shown, lights and cameras. Export again afterwards to see it in the game.
