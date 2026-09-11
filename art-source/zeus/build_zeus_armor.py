"""Build original Zeus armor pieces on the audited native Class304 rig.

Generates Armor (cuirass with integrated pauldrons), Pant, Glove and Boot from
regenerable Python sources, skins them with single-influence bindings to the
frozen *Class304.bmd skeletons, exports diagnostic BMDs (never installable,
atlas files deliberately absent), verifies the BMD roundtrip against the
editable scene and renders front/side/back inspections.
Run: blender --background --python-exit-code 1 --python build_zeus_armor.py [-- --skip-render]
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from export_skinned_bmd import ExportSpec, export
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare
from zeus_armor_preview import render_piece, studio
from zeus_armor_rig import (decoded_skinned_triangles, digest, load_native_rig,
                            USED_BONE_CONTRACT)
from zeus_armor_shapes import armor, boots, gloves, pants
from zeus_materials import PALETTE, create_materials

PIECES = (('Armor', armor), ('Pant', pants), ('Glove', gloves), ('Boot', boots))
# +/-20% bands around the measured Poseidon lote pieces (armor 7842, pant 5620,
# glove 3004, boot 4192 triangles). The Zeus armor integrates the shoulder fans
# into the cuirass contract (native Duel Master armor also skins hip tassets on
# the pelvis/thigh bones), the pant adds the double-pointed front faldon on the
# native root bone, gloves keep the Poseidon pair scope with electric channels
# and the boots the tall faceted greave with a bolt-point knee cop.
TRIANGLE_BUDGETS = {'Armor': (6280, 9400), 'Pant': (4500, 6740),
                    'Glove': (2400, 3610), 'Boot': (3350, 5030)}
BUDGET_JUSTIFICATION = (
    'Proportional (+/-20%) to the measured Poseidon lote pieces (armor 7842, '
    'pant 5620, glove 3004, boot 4192 triangles): the Zeus armor folds the '
    'concept shoulder fans into the cuirass (MU has no separate shoulder slot) '
    'and adds the native hip tassets, the pant adds the double-pointed faldon '
    'hanging from the root bone, gloves and boots mirror the measured scope of '
    'their Poseidon counterparts in the blue/platina language.')
MIN_TRIANGLE_AREA = 1e-8
SHARED_SOURCES = ('armor_surfaces.py', 'celestial_geometry.py', 'export_prop_bmd.py',
                  'export_skinned_bmd.py', 'import_native_reference.py',
                  'inspect_bmd_rig.py', 'verify_prop_roundtrip.py')


def expected_triangles(objects, bind):
    grouped = triangle_groups(objects, Matrix.Identity(4), bind)
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


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
        raise ValueError(f'Degenerate triangle in authored Zeus armor: {minimum}')
    return minimum


def build_pieces(materials, rig, output):
    collections, reports = {}, {}
    for name, builder in PIECES:
        item = collection('Zeus ' + name, lambda b=builder: b(materials))
        collections[name] = item
        bpy.context.view_layer.update()
        topology = topology_report(item.objects)
        minimum_area = triangle_area_report(item.objects)
        native = rig['models'][name]
        target = output / 'models' / f'Zeus_{name}.bmd'
        report = export(item, rig['binds'][name], ExportSpec(native, target))
        model = inspect(target, True)
        low, high = TRIANGLE_BUDGETS[name]
        if not low <= report['triangles'] <= high:
            raise ValueError(f'{name} outside its triangle budget: {report["triangles"]}')
        report.update(topology=topology, minimum_triangle_area=minimum_area, sha256=digest(target),
                      used_bones=sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']}),
                      roundtrip_max_error=compare(expected_triangles(item.objects, rig['binds'][name]),
                                                  decoded_skinned_triangles(model)))
        reports[name] = report
        print('ZEUS_ARMOR_AUTHORED', json.dumps(report), flush=True)
    return collections, reports


def fit_anchors(rig):
    anchors = {}
    for name, bones in USED_BONE_CONTRACT.items():
        anchors[name] = {str(bone): [round(value, 4) for value in rig['binds'][name][bone].translation]
                         for bone in bones}
    return anchors


def write_reports(output, reports, rig, shared_hashes):
    (output / 'texture-dependencies.json').write_text(json.dumps(
        [dict(texture=f'Zeus_{role}.jpg', material_role=role,
              status='MISSING_ATLAS_NOT_FOR_CLIENT', linear_rgb=list(values[0]))
         for role, values in PALETTE.items()], indent=2), encoding='utf-8')
    report = dict(status='AUTHORED_ZEUS_ARMOR_PROTOTYPE_PREVIEW_ONLY',
                  renderer='Blender Cycles / AgX, not MU runtime',
                  design_reference='art-source/zeus/design-spec.md',
                  model_origin='Original Zeus geometry on the audited native Class304 skeletons; '
                               'single-influence rig binding, no native mesh reuse',
                  native_rig=dict(archive=rig['archive'], archive_sha256=rig['archive_sha256'],
                                  audit='native-reference-audit.json',
                                  audit_sha256=digest(ROOT / 'native-reference-audit.json'),
                                  piece_sha256={name: rig['models'][name]['sha256'] for name in rig['models']},
                                  used_bone_contract=rig['used_bone_contract']),
                  fit_anchors=fit_anchors(rig),
                  triangle_budgets=TRIANGLE_BUDGETS,
                  budget_justification=BUDGET_JUSTIFICATION,
                  shared_source_sha256=shared_hashes,
                  blend_sha256=digest(output / 'zeus-armor.blend'),
                  pieces=reports)
    (output / 'armor-build-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skip-render', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    output = ROOT / 'prototype'
    for folder in ('models', 'renders'):
        (output / folder).mkdir(parents=True, exist_ok=True)
    shared_files = [SHARED / name for name in SHARED_SOURCES]
    before = {path.name: digest(path) for path in shared_files}
    rig = load_native_rig()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    collections, reports = build_pieces(create_materials(), rig, output)
    studio()
    if not args.skip_render:
        for name, _ in PIECES:
            render_piece(collections, name, output / 'renders')
    for index, (name, _) in enumerate(PIECES):
        collections[name].hide_render = index != 0
        collections[name].hide_viewport = index != 0
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'zeus-armor.blend'))
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, reports, rig, before)
    print('ZEUS_ARMOR_VERIFIED', json.dumps(
        {name: dict(triangles=reports[name]['triangles'], meshes=reports[name]['meshes'],
                    used_bones=reports[name]['used_bones'],
                    roundtrip_max_error=reports[name]['roundtrip_max_error'])
         for name in reports}), flush=True)


if __name__ == '__main__':
    main()
