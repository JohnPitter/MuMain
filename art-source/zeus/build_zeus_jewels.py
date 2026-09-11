"""Build the original Zeus jewels on the audited Necklace02/Ring02 contracts.

Pendant "Olho de Zeus", Storm Ring and Wisdom Ring from regenerable Python
sources. Each jewel exports a diagnostic BMD with the exact native contract
skeleton - one static root bone (Tube04 for the pendant, Tube01 for both
rings), a single unlocked frame - and every vertex rigidly bound to it.
BMDs are never installable: the Zeus atlases do not exist yet.
Run: blender --background --python-exit-code 1 --python build_zeus_jewels.py [-- --skip-render]
"""
import argparse
import json
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from armor_surfaces import bind_objects
from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from export_skinned_bmd import ExportSpec, export
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare
from zeus_armor_preview import point_at, studio
from zeus_armor_rig import decoded_skinned_triangles
from zeus_jewels_shapes import ANCHORS, BUILDERS
from zeus_materials import atlas_dependencies, create_materials

JEWELS = (('Pendant', 'Tube04'), ('Ring_Storm', 'Tube01'), ('Ring_Wisdom', 'Tube01'))
# Measured against the Poseidon precedent lane: jewels are inventory-scale
# models, so the bands mirror the small body pieces (glove 2,400-3,610).
# The pendant carries frame, chain and eye; the rings share band grammar but
# carry distinct crowns, so each stays inside its own +/-20% window.
TRIANGLE_BUDGETS = {'Pendant': (1700, 3200), 'Ring_Storm': (1600, 2800),
                    'Ring_Wisdom': (1600, 2800)}
BUDGET_JUSTIFICATION = (
    'Inventory-scale jewel precedent from the measured Poseidon small pieces '
    '(glove 3004 triangles): the pendant adds the star frame, chain and eye '
    'gems, each ring shares the band grammar but carries its own crown, all '
    'inside a +/-20% band around 1.6-3.2k triangles. Native Necklace02/Ring02 '
    'are 82/50-triangle placeholders; readable authored jewels need real mass.')
MIN_TRIANGLE_AREA = 1e-8
CONTRACT_FRAMES = [1]
SHARED_SOURCES = ('armor_surfaces.py', 'celestial_geometry.py', 'export_prop_bmd.py',
                  'export_skinned_bmd.py', 'inspect_bmd_rig.py',
                  'verify_prop_roundtrip.py')


def digest(path):
    import hashlib
    return hashlib.sha256(path.read_bytes()).hexdigest()


def contract_model(bone_name):
    """Skeleton exactly as audited for Necklace02/Ring02: one static root."""
    return dict(bones=[dict(name=bone_name, parent=-1,
                            clips=[dict(positions=[(0.0, 0.0, 0.0)],
                                        rotations=[(0.0, 0.0, 0.0)])])],
                action_frames=list(CONTRACT_FRAMES), action_locks=[False])


def topology_report(objects):
    triangles, boundary, nonmanifold = 0, 0, []
    for obj in objects:
        data = obj.data
        data.calc_loop_triangles()
        triangles += len(data.loop_triangles)
        topology = bmesh.new()
        topology.from_mesh(data)
        for edge in topology.edges:
            users = len(edge.link_faces)
            if users > 2:
                nonmanifold.append(obj.name)
            boundary += users == 1
        topology.free()
    if nonmanifold:
        raise ValueError(f'Non-manifold component meshes: {sorted(set(nonmanifold))}')
    return dict(objects=len(objects), triangles=triangles, open_edges=boundary,
                nonmanifold_components=sorted(set(nonmanifold)))


def triangle_area_report(objects):
    minimum = float('inf')
    for obj in objects:
        obj.data.calc_loop_triangles()
        for triangle in obj.data.loop_triangles:
            a, b, c = [obj.matrix_world @ obj.data.vertices[i].co for i in triangle.vertices]
            minimum = min(minimum, (b - a).cross(c - a).length / 2)
    if minimum < MIN_TRIANGLE_AREA:
        raise ValueError(f'Degenerate triangle in authored Zeus jewel: {minimum}')
    return minimum


def expected_triangles(objects, bind):
    grouped = triangle_groups(objects, Matrix.Identity(4), bind)
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


def extents_report(objects):
    points = [obj.matrix_world @ vertex.co for obj in objects
              for vertex in obj.data.vertices]
    return [round(max(p[i] for p in points) - min(p[i] for p in points), 2)
            for i in range(3)]


def build_jewels(palette, output):
    collections, reports = {}, {}
    for name, bone_name in JEWELS:
        item = collection('Zeus ' + name,
                          lambda builder=BUILDERS[name]: builder(palette))
        bind_objects(item.objects, 0)
        bpy.context.view_layer.update()
        topology = topology_report(item.objects)
        minimum_area = triangle_area_report(item.objects)
        native = contract_model(bone_name)
        target = output / 'models' / f'Zeus_{name}.bmd'
        report = export(item, [Matrix.Identity(4)], ExportSpec(native, target))
        model = inspect(target, True)
        low, high = TRIANGLE_BUDGETS[name]
        if not low <= report['triangles'] <= high:
            raise ValueError(f'{name} outside its triangle budget: {report["triangles"]}')
        if model['action_frames'] != CONTRACT_FRAMES or model['action_locks'] != [False]:
            raise ValueError(f'{name}: jewel action contract changed')
        if [bone['name'] for bone in model['bones']] != [bone_name]:
            raise ValueError(f'{name}: skeleton must keep the native bone name')
        for mesh in model['meshes']:
            if mesh['used_bones'] != [0]:
                raise ValueError(f'{name}: every vertex must ride the root bone')
        report.update(topology=topology, minimum_triangle_area=minimum_area,
                      sha256=digest(target), contract_bone=bone_name,
                      used_bones=[0], extents=extents_report(item.objects),
                      anchors=ANCHORS[name],
                      roundtrip_max_error=compare(expected_triangles(item.objects, [Matrix.Identity(4)]),
                                                  decoded_skinned_triangles(model)))
        if report['roundtrip_max_error'] > 1e-6:
            raise ValueError(f'{name}: roundtrip above the 1e-6 gate')
        collections[name] = item
        reports[name] = report
        print('ZEUS_JEWEL_AUTHORED', json.dumps(report), flush=True)
    return collections, reports


def render_jewels(collections, output):
    studio(6)
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 900, 1280
    detail = {'Pendant': ((0, -46, 2), 34), 'Ring_Storm': ((0, -44, 12), 30),
              'Ring_Wisdom': ((0, -44, 10), 32)}
    for name, _ in JEWELS:
        for key, collection_ in collections.items():
            collection_.hide_render = key != name
        for view, location in (('front', (0, -95, 4)), ('side', (95, -18, 4))):
            scene.camera.location = location
            point_at(scene.camera, (0, 0, 4))
            scene.camera.data.ortho_scale = 62
            scene.camera.data.clip_end = 400
            scene.render.filepath = str(output / 'renders' / f'Zeus_{name}_{view}.png')
            bpy.ops.render.render(write_still=True)
        location, scale = detail[name]
        scene.camera.location = location
        point_at(scene.camera, (location[0], 0, location[2] * 0.4))
        scene.camera.data.ortho_scale = scale
        scene.render.filepath = str(output / 'renders' / f'Zeus_{name}_detail.png')
        bpy.ops.render.render(write_still=True)


def write_reports(output, reports, shared_hashes):
    (output / 'texture-dependencies.json').write_text(
        json.dumps(atlas_dependencies(), indent=2), encoding='utf-8')
    document = dict(status='AUTHORED_ZEUS_JEWELS_PROTOTYPE_PREVIEW_ONLY',
                    renderer='Blender Cycles / AgX, not MU runtime',
                    design_reference='art-source/zeus/design-spec.md',
                    model_origin='Original Zeus jewel geometry on the audited '
                                 'Necklace02/Ring02 contracts (one static root bone '
                                 'each, native names preserved); rigid single-bone '
                                 'binding, no native mesh reuse',
                    native_contract=dict(pendant='Data/Item/Necklace02.bmd (bone Tube04)',
                                         rings='Data/Item/Ring02.bmd (bone Tube01)',
                                         audit='native-reference-audit.json',
                                         audit_sha256=digest(ROOT / 'native-reference-audit.json')),
                    triangle_budgets=TRIANGLE_BUDGETS,
                    budget_justification=BUDGET_JUSTIFICATION,
                    shared_source_sha256=shared_hashes,
                    blend_sha256=digest(output / 'zeus-jewels.blend'),
                    jewels=reports)
    (output / 'jewels-build-report.json').write_text(json.dumps(document, indent=2),
                                                     encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skip-render', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    output = ROOT / 'prototype'
    for folder in ('models', 'renders'):
        (output / folder).mkdir(parents=True, exist_ok=True)
    shared_files = [SHARED / name for name in SHARED_SOURCES]
    before = {path.name: digest(path) for path in shared_files}
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    collections, reports = build_jewels(create_materials(), output)
    if not args.skip_render:
        render_jewels(collections, output)
    for index, (name, _) in enumerate(JEWELS):
        collections[name].hide_render = index != 0
        collections[name].hide_viewport = index != 0
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'zeus-jewels.blend'))
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, reports, before)
    print('ZEUS_JEWELS_VERIFIED', json.dumps(
        {name: dict(triangles=reports[name]['triangles'], meshes=reports[name]['meshes'],
                    contract_bone=reports[name]['contract_bone'],
                    extents=reports[name]['extents'],
                    roundtrip_max_error=reports[name]['roundtrip_max_error'])
         for name in reports}, sort_keys=True), flush=True)


if __name__ == '__main__':
    main()
