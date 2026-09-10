"""Export authored rigid props into the exact v10 BMD layout read by Open2."""
from collections import defaultdict
import struct

from mathutils import Matrix, Vector

TRIANGLES_PER_MESH = 2200
MAX_MESH_ELEMENTS = 10000
TRANSFORMS = {
    'Staff': Matrix(((0, -1, 0, 0), (0, 0, -1, 0), (1, 0, 0, 0), (0, 0, 0, 1))),
    'Shield': Matrix(((0, -1, 0, 5), (1, 0, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))),
    'Ring': Matrix.Identity(4),
    'Pendant': Matrix(((1, 0, 0, 0), (0, 0, -1, 7), (0, 1, 0, 3), (0, 0, 0, 1))),
}


def c_string(value):
    encoded = value.encode('ascii')
    if len(encoded) >= 32:
        raise ValueError(f'BMD name too long: {value}')
    return encoded.ljust(32, b'\0')


def triangle_groups(objects, transform, bind=None):
    grouped = defaultdict(list)
    for obj in sorted(objects, key=lambda item: item.name):
        data = obj.data
        data.calc_loop_triangles()
        matrix = transform @ obj.matrix_world
        normals = matrix.to_3x3().inverted().transposed()
        for triangle in data.loop_triangles:
            texture = data.materials[triangle.material_index].name
            corners = []
            for loop_id in triangle.loops:
                vertex = data.vertices[data.loops[loop_id].vertex_index]
                uv = data.uv_layers.active.data[loop_id].uv
                normal = (normals @ data.corner_normals[loop_id].vector).normalized()
                corner = (tuple(matrix @ vertex.co), tuple(normal), (uv.x, 1 - uv.y))
                corners.append(bound_corner(corner, vertex, obj, bind))
            grouped[texture].append(corners)
    return grouped


def bound_corner(corner, vertex, obj, bind):
    if bind is None:
        return corner
    influences = [group for group in vertex.groups if group.weight > 0]
    if len(influences) != 1 or abs(influences[0].weight - 1) > .0001:
        raise ValueError(f'MU requires one full influence: {obj.name}/{vertex.index}')
    node = int(obj.vertex_groups[influences[0].group].name.removeprefix('mu_'))
    inverse = bind[node].inverted()
    position, normal, uv = corner
    return (tuple(inverse @ Vector(position)),
            tuple((inverse.to_3x3() @ Vector(normal)).normalized()), uv, node)


def encode_mesh(triangles, texture, index):
    vertices, normals, uvs, faces = [], [], [], []
    vertex_ids, normal_ids, uv_ids = {}, {}, {}
    for triangle in triangles:
        face = [[], [], []]
        for corner in triangle:
            position, normal, uv = corner[:3]
            node = corner[3] if len(corner) == 4 else 0
            v = intern((node, *position), vertices, vertex_ids)
            n = intern((node, *normal, v), normals, normal_ids)
            t = intern(uv, uvs, uv_ids)
            for values, value in zip(face, (v, n, t)):
                values.append(value)
        faces.append(face)
    counts = (len(vertices), len(normals), len(uvs), len(faces))
    if max(counts) > MAX_MESH_ELEMENTS:
        raise ValueError(f'Client mesh limit exceeded: {counts}')
    result = bytearray(struct.pack('<5h', *counts, index))
    for node, *point in vertices:
        result.extend(struct.pack('<h2x3f', node, *point))
    for node, nx, ny, nz, vertex in normals:
        result.extend(struct.pack('<h2x3fh2x', node, nx, ny, nz, vertex))
    for uv in uvs:
        result.extend(struct.pack('<2f', *uv))
    for face in faces:
        record = bytearray(64)
        record[0] = 3
        for offset, values in zip((2, 10, 18), face):
            struct.pack_into('<4h', record, offset, *values, 0)
        result.extend(record)
    result.extend(c_string(texture))
    return result


def intern(value, values, indices):
    key = tuple(round(component, 6) for component in value)
    if key not in indices:
        indices[key] = len(values)
        values.append(value)
    return indices[key]


def export(collection, name, path):
    transform = TRANSFORMS[name]
    grouped = triangle_groups(collection.objects, transform)
    meshes = []
    for texture, triangles in sorted(grouped.items()):
        for start in range(0, len(triangles), TRIANGLES_PER_MESH):
            meshes.append(encode_mesh(triangles[start:start + TRIANGLES_PER_MESH], texture, len(meshes)))
    result = bytearray(b'BMD\x0a' + c_string('Celestial_' + name))
    anchors = [(0, 0, 0)]
    if name == 'Staff':
        anchors.extend(tuple(transform @ Vector(p)) for p in ((0, -3, 86), (0, 0, 128)))
    result.extend(struct.pack('<3h', len(meshes), len(anchors), 1))
    for mesh in meshes:
        result.extend(mesh)
    result.extend(struct.pack('<hB', 1, 0))
    for index, position in enumerate(anchors):
        result.extend(b'\0' + c_string(f'Celestial_{name}_{index}'))
        result.extend(struct.pack('<h6f', -1 if index == 0 else 0, *position, 0, 0, 0))
    path.write_bytes(result)
    return dict(file=path.name, meshes=len(meshes), triangles=sum(map(len, grouped.values())),
                bones=len(anchors), textures=sorted(grouped))
