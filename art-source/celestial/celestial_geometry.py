"""Editable sculptural primitives for the Celestial reference reconstruction."""
import math
from pathlib import Path

import bpy
import bmesh
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

ROOT = Path(__file__).resolve().parent
TAU = math.tau


def materials():
    palette = {
        'Gold': ((0.83, 0.49, 0.12), 0.78, 0.23),
        'Ivory': ((0.92, 0.9, 0.8), 0.38, 0.25),
        'Sapphire': ((0.035, 0.54, 0.95), 0.35, 0.18),
        'Emissive': ((1.0, 0.83, 0.43), 0.25, 0.22),
    }
    result = {}
    for name, (color, metal, roughness) in palette.items():
        mat = bpy.data.materials.new(f'Celestial_{name}.jpg')
        mat.use_nodes = True
        mat.diffuse_color = (*color, 1)
        shader = next(node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
        shader.inputs['Base Color'].default_value = (*color, 1)
        shader.inputs['Metallic'].default_value = metal
        shader.inputs['Roughness'].default_value = roughness
        texture = mat.node_tree.nodes.new('ShaderNodeTexImage')
        texture.image = bpy.data.images.load(str(ROOT / 'textures' / mat.name))
        mat.node_tree.links.new(texture.outputs['Color'], shader.inputs['Base Color'])
        if name in ('Sapphire', 'Emissive'):
            shader.inputs['Emission Color'].default_value = (*color, 1)
            shader.inputs['Emission Strength'].default_value = 0.35
        result[name] = mat
    return result


def mesh(name, vertices, faces, material):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    topology = bmesh.new()
    topology.from_mesh(data)
    bmesh.ops.recalc_face_normals(topology, faces=list(topology.faces))
    topology.to_mesh(data)
    topology.free()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    layer = data.uv_layers.new(name='SurfaceUV')
    bounds = [(min(v.co[i] for v in data.vertices),
               max(v.co[i] for v in data.vertices)) for i in range(3)]
    axes = sorted(range(3), key=lambda i: bounds[i][1] - bounds[i][0], reverse=True)[:2]
    for polygon in data.polygons:
        for loop_id in polygon.loop_indices:
            co = data.vertices[data.loops[loop_id].vertex_index].co
            layer.data[loop_id].uv = tuple(
                (co[i] - bounds[i][0]) / max(bounds[i][1] - bounds[i][0], 0.001)
                for i in axes)
    return obj


def tube(name, path, radii, material, sides=8):
    points = [Vector(p) for p in path]
    if isinstance(radii, (int, float)):
        radii = [radii] * len(points)
    vertices, faces = [], []
    for index, point in enumerate(points):
        tangent = points[min(index + 1, len(points) - 1)] - points[max(index - 1, 0)]
        tangent.normalize()
        side = tangent.cross(Vector((0, -1, 0)))
        if side.length < 0.01:
            side = tangent.cross(Vector((1, 0, 0)))
        side.normalize()
        up = tangent.cross(side).normalized()
        vertices.extend(point + radii[index] * (side * math.cos(TAU * n / sides)
                        + up * math.sin(TAU * n / sides)) for n in range(sides))
    for row in range(len(points) - 1):
        for n in range(sides):
            a, b = row * sides + n, row * sides + (n + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    faces.extend((tuple(reversed(range(sides))),
                  tuple((len(points) - 1) * sides + n for n in range(sides))))
    obj = mesh(name, vertices, faces, material)
    for face in obj.data.polygons[:-2]:
        face.use_smooth = True
    return obj


def bezier(points, count=20):
    a, b, c, d = map(Vector, points)
    return [(1 - t) ** 3 * a + 3 * t * (1 - t) ** 2 * b
            + 3 * t ** 2 * (1 - t) * c + t ** 3 * d
            for t in (i / (count - 1) for i in range(count))]


def tendril(name, controls, width, material):
    points = bezier(controls)
    radii = [width * (0.82 * math.sin(math.pi * i / (len(points) - 1)) ** 0.65 + 0.18)
             for i in range(len(points))]
    return tube(name, points, radii, material)


def ellipse(name, center, axes, material, width=0.5, steps=64):
    center, u, v = Vector(center), Vector(axes[0]), Vector(axes[1])
    path = [center + u * math.cos(TAU * i / steps) + v * math.sin(TAU * i / steps)
            for i in range(steps + 1)]
    return tube(name, path, width, material)


def plate(name, outline, shape, material):
    """Convex raised plate: outline is X/Z; shape is front depth and crown rise."""
    depth, rise = shape
    rim = [Vector((x, depth, z)) for x, z in outline]
    center = sum(rim, Vector()) / len(rim)
    center.y -= rise
    vertices = rim + [center] + [Vector((v.x, depth + 0.6, v.z)) for v in rim]
    count = len(rim)
    faces = [(i, (i + 1) % count, count) for i in range(count)]
    faces.extend((i, count + 1 + i, count + 1 + (i + 1) % count,
                  (i + 1) % count) for i in range(count))
    back = vertices[count + 1:]
    for tri in tessellate_polygon([back]):
        faces.append(tuple(count + 1 + (v if isinstance(v, int) else back.index(v))
                           for v in reversed(tri)))
    return mesh(name, vertices, faces, material)


def rim(name, outline, depth, material, width=0.5):
    path = [(x, depth, z) for x, z in outline]
    return tube(name, path + [path[0]], width, material)


def gem(name, center, size, material):
    x, y, z = center
    w, d, h = size
    belt = [(x, y, z + h), (x + w, y, z), (x, y, z - h), (x - w, y, z)]
    table = [(x + (vx - x) * 0.38, y - d, z + (vz - z) * 0.38)
             for vx, _, vz in belt]
    vertices = belt + table + [(x, y + d * 0.7, z)]
    faces = [(4, 5, 6, 7)]
    for i in range(4):
        n = (i + 1) % 4
        faces.extend(((i, n, n + 4, i + 4), (n, i, 8)))
    return mesh(name, vertices, faces, material)


def feather(name, controls, dimensions, material):
    """Curved leaf with raised midrib, tapered tip, and a closed underside."""
    width, thickness = dimensions
    path = bezier(controls, 12)
    vertices = []
    for i, center in enumerate(path):
        t = i / (len(path) - 1)
        tangent = path[min(i + 1, len(path) - 1)] - path[max(i - 1, 0)]
        side = Vector((tangent.z, 0, -tangent.x)).normalized()
        span = width * (math.sin(math.pi * t) ** 0.65 * (1 - 0.35 * t) + 0.003)
        vertices.extend((center - side * span, center + Vector((0, -thickness, 0)),
                         center + side * span, center + Vector((0, thickness * 0.2, 0))))
    faces = []
    for i in range(len(path) - 1):
        for n in range(4):
            a, b = 4 * i + n, 4 * i + (n + 1) % 4
            faces.append((a, b, b + 4, a + 4))
    faces.extend(((3, 2, 1, 0), tuple(range(len(vertices) - 4, len(vertices)))))
    return mesh(name, vertices, faces, material)


def collection(name, builder):
    before = set(bpy.data.objects)
    builder()
    result = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(result)
    for obj in set(bpy.data.objects) - before:
        for old in list(obj.users_collection):
            old.objects.unlink(obj)
        result.objects.link(obj)
    return result
