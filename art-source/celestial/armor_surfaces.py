"""Surface lofts and rigid-joint bindings matching the MU single-influence renderer."""
import math

import bpy
import bmesh
from mathutils import Matrix, Vector

from celestial_geometry import mesh


def loft(name, rings, material, segments=24):
    """Each ring is (center xyz, horizontal radius, depth radius)."""
    vertices, faces = [], []
    for center, width, depth in rings:
        vertices.extend((center[0] + width * math.cos(math.tau * i / segments),
                         center[1] + depth * math.sin(math.tau * i / segments), center[2])
                        for i in range(segments))
    for row in range(len(rings) - 1):
        for i in range(segments):
            a, b = row * segments + i, row * segments + (i + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons:
        face.use_smooth = True
    return result


def segment_shell(name, endpoints, radii, material):
    start, end = map(Vector, endpoints)
    length = (end - start).length
    rings = [((0, 0, length * t), width, depth) for t, width, depth in radii]
    obj = loft(name, rings, material, 16)
    rotation = Vector((0, 0, 1)).rotation_difference((end - start).normalized())
    for vertex in obj.data.vertices:
        vertex.co = start + rotation @ vertex.co
    return obj


def seal_shell_ends(obj):
    topology = bmesh.new()
    topology.from_mesh(obj.data)
    boundary = [edge for edge in topology.edges if edge.is_boundary]
    bmesh.ops.holes_fill(topology, edges=boundary)
    bmesh.ops.recalc_face_normals(topology, faces=list(topology.faces))
    topology.to_mesh(obj.data)
    topology.free()
    obj.data.update()


def rear_facing(builder):
    """Reflect front-authored reliefs toward +Y while preserving outward winding."""
    previous = set(bpy.data.objects)
    builder()
    for obj in set(bpy.data.objects) - previous:
        obj.data.transform(Matrix.Diagonal((1, -1, 1, 1)))
        topology = bmesh.new()
        topology.from_mesh(obj.data)
        bmesh.ops.reverse_faces(topology, faces=list(topology.faces))
        topology.to_mesh(obj.data)
        topology.free()
        obj.data.update()
        obj['celestial_facing'] = 'rear'


def bind_objects(objects, resolver):
    for obj in objects:
        groups = {}
        for vertex in obj.data.vertices:
            bone = resolver(obj, obj.matrix_world @ vertex.co) if callable(resolver) else resolver
            groups.setdefault(bone, []).append(vertex.index)
        for bone, indices in groups.items():
            obj.vertex_groups.new(name=f'mu_{bone:03d}').add(indices, 1, 'REPLACE')


def section(bone, builder):
    previous = set(bpy.data.objects)
    builder()
    objects = set(bpy.data.objects) - previous
    bpy.context.view_layer.update()
    bind_objects(objects, bone)
    return objects


def add_skin(collection, rig):
    for obj in collection.objects:
        if not obj.vertex_groups:
            raise ValueError(f'Unbound Celestial part: {obj.name}')
        modifier = obj.modifiers.new('MU single-bone skin', 'ARMATURE')
        modifier.object = rig
