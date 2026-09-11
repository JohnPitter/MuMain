"""Compare every exported bone-local corner with the editable armor source."""
from collections import Counter, defaultdict
import json
import sys
from pathlib import Path

import bpy
import bmesh
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from artifact_proof import digest
from export_prop_bmd import triangle_groups
from import_native_reference import world_matrices
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare

OUTPUT = ROOT / 'authored-armor'
NAMES = ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots')


def validate_area(group):
    minimum = float('inf')
    for obj in group.objects:
        obj.data.calc_loop_triangles()
        for triangle in obj.data.loop_triangles:
            a, b, c = [obj.matrix_world @ obj.data.vertices[i].co for i in triangle.vertices]
            area = (b - a).cross(c - a).length / 2
            if area < 1e-8:
                raise ValueError(f'Degenerate armor triangle: {obj.name}/{triangle.index}')
            minimum = min(minimum, area)
    return minimum


def verify_closed_sabatons(group):
    shoes = [obj for obj in group.objects if obj.name.startswith('Boots / pointed sabaton')]
    if len(shoes) != 2:
        raise ValueError('Expected two closed sabatons')
    for obj in shoes:
        counts = Counter()
        for face in obj.data.polygons:
            for edge in face.edge_keys:
                counts[tuple(sorted(edge))] += 1
        if not counts or any(count != 2 for count in counts.values()):
            raise ValueError(f'Open or non-manifold sabaton: {obj.name}')
    return len(shoes)


def verify_rear_reliefs(group):
    reliefs = [obj for obj in group.objects if obj.get('celestial_facing') == 'rear']
    for obj in reliefs:
        if sum(vertex.co.y for vertex in obj.data.vertices) <= 0:
            raise ValueError(f'Rear relief faces the front: {obj.name}')
        topology = bmesh.new()
        topology.from_mesh(obj.data)
        try:
            if any(not edge.is_manifold for edge in topology.edges) or topology.calc_volume(signed=True) <= 0:
                raise ValueError(f'Open or inverted rear relief: {obj.name}')
        finally:
            topology.free()
    return len(reliefs)


def verify_curved_plates(group):
    plates = [obj for obj in group.objects if obj.get('celestial_surface') == 'curved_front_plate']
    if len(plates) != 4:
        raise ValueError('Expected two pectorals and two curved pauldrons')
    for obj in plates:
        topology = bmesh.new()
        topology.from_mesh(obj.data)
        try:
            if any(not edge.is_manifold for edge in topology.edges) or topology.calc_volume(signed=True) <= 0:
                raise ValueError(f'Open or inverted curved plate: {obj.name}')
            depths = {round(vertex.co.y, 3) for vertex in obj.data.vertices}
            if len(depths) < 7 or sum(face.use_smooth for face in obj.data.polygons) < 100:
                raise ValueError(f'Curved plate lost its crown or normals: {obj.name}')
        finally:
            topology.free()
    return len(plates)


def decoded(model):
    grouped = defaultdict(list)
    for mesh in model['meshes']:
        for (vertices, uv_ids), normals in zip(mesh['faces'], mesh['normal_indices']):
            corners = []
            for vertex, normal, uv in zip(vertices, normals, uv_ids):
                bone, position = mesh['points'][vertex]
                norm = mesh['normal_vectors'][normal]
                if norm[0] != bone or norm[-1] != vertex:
                    raise ValueError('Normal bound to the wrong bone or vertex')
                corners.append((position, norm[1:4], mesh['texcoords'][uv], (bone,)))
            grouped[mesh['texture']].append(corners)
    return grouped


def main():
    bpy.ops.wm.open_mainfile(filepath=str(OUTPUT / 'celestial-authored-armor.blend'))
    bpy.context.scene.frame_set(1)
    reports = {}
    for name in NAMES:
        model = inspect(OUTPUT / 'Data' / 'Player' / f'Celestial_{name}.bmd', True)
        group = bpy.data.collections['Celestial ' + name]
        expected = triangle_groups(group.objects, Matrix.Identity(4), world_matrices(model, 0))
        expected = {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                              for triangle in triangles] for texture, triangles in expected.items()}
        reports[name] = dict(max_error=compare(expected, decoded(model)),
                             triangles=sum(map(len, expected.values())),
                             minimum_triangle_area=validate_area(group), model_sha256=model['sha256'],
                             blend_sha256=digest(OUTPUT / 'celestial-authored-armor.blend'))
        if name == 'Boots':
            reports[name]['closed_sabatons'] = verify_closed_sabatons(group)
        if name in ('Helm', 'Armor'):
            reports[name]['rear_reliefs'] = verify_rear_reliefs(group)
        if name == 'Armor':
            reports[name]['curved_front_plates'] = verify_curved_plates(group)
    (OUTPUT / 'roundtrip-report.json').write_text(json.dumps(reports, indent=2), encoding='utf-8')
    print('ARMOR_ROUNDTRIP_VERIFIED', json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
