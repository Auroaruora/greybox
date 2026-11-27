# Asset rules

What every asset model must be. The `handle-requests` skill says how to work; this page says what the result must be.

## Where things are

| What | Where |
|---|---|
| Status, level, why, variants, surfaces, requests | `asset_list.json` (changed only by `tools/agent/requests.py` or the review page) |
| Model, one collection per variant | `models/<id>/model.blend` |
| Parts, variants, colour sets, own materials | `../assets/models/<id>/asset.json` (changed only by `requests.py`) |
| Textures | `../assets/models/<id>/textures/` |
| Request images | `models/<id>/refs/` |
| Palette | `materials/palette.blend`; `../assets/materials/palette.json` is exported from it |

## Names

- **id**: the asset's camelCase name (`deskLamp`). Used by folders, layouts, looks and game code. Never change it.
- **variant**: one shape of the asset, named by a letter.
- **surface**: one extra look of a variant (textures on some parts). It is a colour set in `asset.json` named `<variant>Surface<n>`, used only by that variant. A variant's default look is the materials on its mesh.
- **part**: one piece of the asset, from `parts` in `asset.json`. A variant may leave a part out. Add a part only with `requests.py part`.
- In the `.blend`, a variant is a collection named by its letter, holding one object per part named `<variant>_<part>` (`c_body`). The mesh has the same name.

## Shape

1. Real size in metres, +Z up.
2. The origin is where the asset rests or hangs: the middle of the base, or the middle of the back for wall items. Every part shares it, with no rotation or scale on the object.
3. A collection holds only its parts: no lights, cameras, helpers, leftovers or default names (`Cube`, `.001`).
4. Each part is a closed solid with outward normals, or a single-sided sheet for flat things seen from one side.
5. Nothing thinner than 5 mm except sheets. Where things touch, leave a 2 mm gap or overlap clearly.
6. Hard surfaces are flat shaded with sharp edges; rounded parts are smooth shaded.
7. A part that any surface textures has one UV map named `UVMap`, the picture upright and filling 0 to 1. Never change the UVs of a part an existing surface already textures.

## What the request leaves open

- Detail: the level's `modelling` line.
- Size: the real-world size of the thing in the images. With no image, stay close to the asset's other variants.
- Parts: add the ones the design needs.
- Glass and other see-through parts: leave them out, open.
- Brands: never real brands or logos. Invented logos, brands or text only when the request asks.

## Colour

Images give shape and surface detail, not colour. Each mesh slot takes the first that applies:

1. A colour the request text names, as the closest palette material.
2. A part that `colourFrom` has: its materials, slot for slot.
3. A new part that is another asset in the room (filament in a printer is `filamentSpool`): that asset's first colour set.
4. Screens, bulbs, indicator lights: a glowing palette material.
5. Any other new part: a material this asset already uses. Use the images only to choose lighter or darker.

Mesh slots use only palette materials or the asset's existing own materials. Never add to the palette.

## Textures

- Made only for a surface: when the request has surface images, asks for one in words, or is a surface request.
- Wear and materials (rust, wood, fabric): tint over the part's own colour, so the part keeps its colour.
- Prints and logos: keep their own colours, placed where the request says.
- One PNG per part slot, at most 1024 px, drawn by script or baked in Blender.
- Register each with `requests.py texture`. Never put a surface texture on a mesh slot.

## Never

- Change other assets, scene data (layouts, looks, lighting, viewpoints) or the palette.
- Delete or retire a variant or surface; that is the user's call.
- Create `.blend1` files.
- Commit to git.
