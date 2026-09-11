"""Build the original Poseidon pendant and rings prototypes as rigid items.

Same pipeline discipline as the weapons and armor batches: regenerable Python
sources, diagnostic BMD export on the audited native item contract (never
installable, atlases deliberately absent), BMD roundtrip proof and
front/side/detail inspection renders. The two rings are authored models -
the native precedent (one ring model equipped on both hands) is deliberately
not repeated.
Run: blender --background --python-exit-code 1 --python build_poseidon_jewels.py [-- --skip-render]
"""
import argparse
import json
import sys
from pathlib import Path

import bmesh
import bpy

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from artifact_proof import digest
from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from poseidon_armor_preview import studio
from poseidon_jewels_export import AUTHORED_ANCHORS, ITEM_TRANSFORMS, export_jewel
from poseidon_jewels_preview import render_piece
from poseidon_jewels_shapes import BUILDERS
from poseidon_materials import create_materials
from verify_prop_roundtrip import compare, decoded_triangles

PIECES = tuple(BUILDERS.items())
# Jewelry scale gate: the audited native item reference (saint.bmd) measures
# 313 triangles and the smallest lote-2 worn piece (helm) is 3484. The pendant
# carries the trident crest, bail and a fine chain; each ring is a complete
# distinct model (band + crest + rear detail), so both land above the native
# item floor yet far below any worn garment.
TRIANGLE_BUDGETS = {'Pendant': (1900, 3400), 'RingTide': (1600, 3000),
                    'RingEmperor': (1600, 3000)}
BUDGET_JUSTIFICATION = (
    'Native item reference saint.bmd is 313 triangles; the smallest worn piece '
    '(helm) is 3484. Jewelry must stay light for inventory/world rendering: the '
    'pendant adds the trident relief, bail and chain stub over the framed '
    'pedestal, and the two rings are full distinct models with engraved bands, '
    'crests and rear detail - each roughly half the helm, well above the native '
    'item floor.')
MIN_TRIANGLE_AREA = 1e-8
SHARED_SOURCES = ('artifact_proof.py', 'celestial_geometry.py', 'export_prop_bmd.py',
                  'inspect_bmd_rig.py', 'verify_prop_roundtrip.py')
NATIVE_ITEM_KEY = 'native_scepter_orientation_only'


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
        raise ValueError(f'Degenerate triangle in authored jewels: {minimum}')
    return minimum


def build_pieces(materials, output, audit):
    native_item = audit['models'][NATIVE_ITEM_KEY]
    collections, reports = {}, {}
    for name, builder in PIECES:
        item = collection('Poseidon ' + name, lambda b=builder: b(materials))
        collections[name] = item
        bpy.context.view_layer.update()
        topology = topology_report(item.objects)
        minimum_area = triangle_area_report(item.objects)
        target = output / 'models' / f'Poseidon_{name}.bmd'
        report = export_jewel(item, name, target)
        model = inspect(target, True)
        if model['action_frames'] != [1] or model['action_locks'] != [False]:
            raise ValueError(f'{name}: jewels must stay a rigid single-pose item')
        low, high = TRIANGLE_BUDGETS[name]
        if not low <= report['triangles'] <= high:
            raise ValueError(f'{name} outside its triangle budget: {report["triangles"]}')
        expected = triangle_groups(item.objects, ITEM_TRANSFORMS[name])
        report.update(topology=topology, minimum_triangle_area=minimum_area,
                      sha256=digest(target),
                      native_item_contract_sha256=native_item['sha256'],
                      roundtrip_max_error=compare(expected, decoded_triangles(model)))
        reports[name] = report
        print('POSEIDON_JEWELS_AUTHORED', json.dumps(report), flush=True)
    return collections, reports


def write_reports(output, reports, audit, shared_hashes):
    report = dict(status='AUTHORED_JEWELS_PROTOTYPE_PREVIEW_ONLY',
                  renderer='Blender Cycles / AgX, not MU runtime',
                  design_reference='art-source/poseidon/design-spec.md',
                  model_origin='Original Poseidon jewelry geometry as rigid item '
                               'prototypes on the audited native item contract '
                               '(Data/Item reference: rigid meshes, single static '
                               'pose, geometry on bone 0); anchor bones name the '
                               'mount points; no native mesh reuse and no player-rig '
                               'skinning - in-body appearance stays deferred per '
                               'design-spec',
                  native_item_contract=dict(
                      reference=NATIVE_ITEM_KEY,
                      member=audit['models'][NATIVE_ITEM_KEY]['member'],
                      sha256=audit['models'][NATIVE_ITEM_KEY]['sha256'],
                      audit='native-reference-audit.json',
                      audit_sha256=digest(ROOT / 'native-reference-audit.json'),
                      archive_sha256=audit['archive_sha256']),
                  triangle_budgets=TRIANGLE_BUDGETS,
                  budget_justification=BUDGET_JUSTIFICATION,
                  shared_source_sha256=shared_hashes,
                  blend_sha256=digest(output / 'poseidon-jewels.blend'),
                  pieces=reports)
    (output / 'jewels-build-report.json').write_text(json.dumps(report, indent=2),
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
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    collections, reports = build_pieces(create_materials(), output, audit)
    studio(8)
    if not args.skip_render:
        for name, _ in PIECES:
            render_piece(collections, name, output / 'renders')
    for index, (name, _) in enumerate(PIECES):
        collections[name].hide_render = index != 0
        collections[name].hide_viewport = index != 0
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'poseidon-jewels.blend'))
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, reports, audit, before)
    print('POSEIDON_JEWELS_VERIFIED', json.dumps(
        {name: dict(triangles=reports[name]['triangles'], meshes=reports[name]['meshes'],
                    anchors=list(AUTHORED_ANCHORS[name]),
                    roundtrip_max_error=reports[name]['roundtrip_max_error'])
         for name in reports}), flush=True)


if __name__ == '__main__':
    main()
