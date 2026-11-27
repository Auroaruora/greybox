import glob
import io
import json
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import schema

ROOT = os.path.dirname(HERE)
MODELS = os.path.join(ROOT, "models")
PALETTE_BLEND = os.path.join(ROOT, "materials", "palette.blend")
OUT = os.path.join(os.path.dirname(ROOT), "assets")
DATA = os.path.join(OUT, "models")
PALETTE_JSON = os.path.join(OUT, "materials", "palette.json")
TURN = np.array(Matrix(((1, 0, 0), (0, 0, 1), (0, -1, 0))))


def encode(value):
    return value * 12.92 if value <= 0.0031308 else 1.055 * value ** (1 / 2.4) - 0.055


def hex_colour(linear):
    return "#%02x%02x%02x" % tuple(round(max(0.0, min(1.0, encode(c))) * 255) for c in linear)


def principled(material):
    if material is None or not material.use_nodes:
        return None
    return next((n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)


def palette_entry(material):
    node = principled(material)
    linear = list(node.inputs["Base Color"].default_value)[:3]
    entry = {"color": hex_colour(linear), "linear": linear}
    strength = node.inputs["Emission Strength"].default_value
    if strength > 0:
        entry["emission"] = [c * strength for c in node.inputs["Emission Color"].default_value[:3]]
    return entry


def export_palette():
    with bpy.data.libraries.load(PALETTE_BLEND, link=False) as (source, target):
        target.materials = list(source.materials)
    palette = {m.name: palette_entry(m) for m in sorted(target.materials, key=lambda m: m.name)}
    schema.check(palette, schema.PALETTE, "palette")
    os.makedirs(os.path.dirname(PALETTE_JSON), exist_ok=True)
    with open(PALETTE_JSON, "w") as f:
        f.write(json.dumps(palette, indent=2) + "\n")
    return len(palette)


def rows(fmt, values):
    text = io.StringIO()
    np.savetxt(text, values, fmt=fmt)
    return text.getvalue()


def corner_normals(mesh, loops, faces):
    corners = np.empty(len(mesh.loops) * 3, dtype=np.float64)
    mesh.corner_normals.foreach_get("vector", corners)
    corners = corners.reshape(-1, 3)[loops.ravel()]
    triangles = np.empty(len(mesh.loop_triangles) * 3, dtype=np.float64)
    mesh.loop_triangles.foreach_get("normal", triangles)
    triangles = np.repeat(triangles.reshape(-1, 3), 3, axis=0)
    smooth = np.empty(len(mesh.polygons), dtype=bool)
    mesh.polygons.foreach_get("use_smooth", smooth)
    smooth = np.repeat(smooth[faces], 3)
    return np.where(smooth[:, None], corners, triangles)


def write_obj(mesh, path):
    mesh.calc_loop_triangles()
    count = len(mesh.loop_triangles)
    vertices = np.empty(count * 3, dtype=np.int64)
    loops = np.empty(count * 3, dtype=np.int64)
    faces = np.empty(count, dtype=np.int64)
    mesh.loop_triangles.foreach_get("vertices", vertices)
    mesh.loop_triangles.foreach_get("loops", loops)
    mesh.loop_triangles.foreach_get("polygon_index", faces)
    points = np.empty(len(mesh.vertices) * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", points)
    uvs = np.zeros(len(mesh.loops) * 2, dtype=np.float64)
    if mesh.uv_layers.active:
        mesh.uv_layers.active.data.foreach_get("uv", uvs)
    normals = np.round(corner_normals(mesh, loops, faces) @ TURN.T, 6)
    uvs = np.round(uvs.reshape(-1, 2)[loops], 6)
    normal_rows, normal_index = np.unique(normals, axis=0, return_inverse=True)
    uv_rows, uv_index = np.unique(uvs, axis=0, return_inverse=True)
    corners = np.stack([vertices, uv_index.ravel(), normal_index.ravel()], axis=1) + 1
    with open(path, "w") as f:
        if mesh.materials and mesh.materials[0]:
            f.write("usemtl %s\n" % mesh.materials[0].name)
        f.write(rows("v %.6f %.6f %.6f", points.reshape(-1, 3) @ TURN.T))
        f.write(rows("vt %.6f %.6f", uv_rows))
        f.write(rows("vn %.6f %.6f %.6f", normal_rows))
        f.write(rows("f %d/%d/%d %d/%d/%d %d/%d/%d", corners.reshape(-1, 9)))


def export_asset(asset):
    doc = schema.check(json.load(open(os.path.join(DATA, asset, "asset.json"))), schema.ASSET, asset)
    names = ["%s_%s" % (shape, part) for shape in doc["shapes"] for part in doc["parts"]]
    with bpy.data.libraries.load(os.path.join(MODELS, asset, "model.blend"), link=True) as (source, target):
        target.meshes = [n for n in names if n in source.meshes]
    for old in glob.glob(os.path.join(DATA, asset, "*.obj")):
        os.remove(old)
    for mesh in target.meshes:
        write_obj(mesh, os.path.join(DATA, asset, mesh.name + ".obj"))
    return len(target.meshes)


def main():
    bpy.ops.wm.read_homefile(use_empty=True)
    colours = export_palette()
    assets = sorted(a for a in os.listdir(MODELS) if os.path.isfile(os.path.join(MODELS, a, "model.blend")))
    meshes = sum(export_asset(asset) for asset in assets)
    print("EXPORT %d assets, %d meshes, %d palette colours" % (len(assets), meshes, colours))


main()
