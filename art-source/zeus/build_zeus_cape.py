"""Build the original Zeus blue cape prototype on the audited cape link.

Generates the "Manto do Herdeiro" cape from regenerable Python sources in the
player bind world space, binds every vertex to the single native "collar" bone
through the reconstructed runtime link matrix (zeus_cape_rig), exports a
diagnostic BMD (never installable, atlases deliberately absent), verifies the
BMD roundtrip against the editable scene and renders front/side/back
inspections. The fabric mass of the long blue cape is NOT in the BMD: like the
native Cape of the Emperor, it is procedural cloth owned by the runtime; this
file carries the rigid ferragem only (see cape-cloth-contract.md). Run:
blender --background --python-exit-code 1 --python build_zeus_cape.py [-- --skip-render]
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

from armor_surfaces import section
from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from export_skinned_bmd import ExportSpec, export
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare
from zeus_cape_materials import create_cape_materials
from zeus_cape_preview import render_cape, studio
from zeus_cape_rig import (cape_bind, decoded_skinned_triangles,
                           digest, load_cape_rig)
from zeus_cape_shapes import PANEL_PLANE, cape

# Long cape ferragem: structured collar, emblem shield and six lightning tails.
# Smaller than the Poseidon black cape (8,786) because the Zeus tails are
# jagged flat blades without gold crests, but well above any single armor
# piece; the fabric mass itself stays procedural cloth, so the ferragem alone
# carries the detail.
TRIANGLE_BUDGET = (6000, 10000)
BUDGET_JUSTIFICATION = (
    'Long cape with a structured twelve-facet collar, the bolt emblem shield '
    'and six rigid lightning-tail panels; the concept tails are pointed, so '
    'they cannot be cloth (a CPhysicsCloth grid always ends in a straight '
    'hem) and must be authored geometry, while the blue fabric mass itself '
    'stays procedural cloth owned by the runtime.')
MIN_TRIANGLE_AREA = 1e-8
SHARED_SOURCES = ('armor_surfaces.py', 'celestial_geometry.py', 'export_prop_bmd.py',
                  'export_skinned_bmd.py', 'import_native_reference.py',
                  'inspect_bmd_rig.py', 'verify_prop_roundtrip.py')
FAMILY_SOURCES = ('zeus_materials.py', 'zeus_shapes.py', 'zeus_armor_shapes.py')


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
        raise ValueError(f'Degenerate triangle in authored cape: {minimum}')
    return minimum


def build_cape(materials, rig, output):
    item = collection('Zeus Cape', lambda: _bound_cape(materials))
    bpy.context.view_layer.update()
    topology = topology_report(item.objects)
    minimum_area = triangle_area_report(item.objects)
    target = output / 'models' / 'Zeus_Cape.bmd'
    bind = cape_bind(rig)
    report = export(item, bind, ExportSpec(rig['native'], target))
    model = inspect(target, True)
    low, high = TRIANGLE_BUDGET
    if not low <= report['triangles'] <= high:
        raise ValueError(f'cape outside its triangle budget: {report["triangles"]}')
    report.update(topology=topology, minimum_triangle_area=minimum_area, sha256=digest(target),
                  used_bones=sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']}),
                  roundtrip_max_error=compare(expected_triangles(item.objects, bind),
                                              decoded_skinned_triangles(model)))
    if report['roundtrip_max_error'] > 1e-6:
        raise ValueError('Cape BMD roundtrip above the 1e-6 gate')
    print('ZEUS_CAPE_AUTHORED', json.dumps(report), flush=True)
    return item, report


def _bound_cape(materials):
    """Author every part and bind it to the single native cape bone."""
    section(0, lambda: cape(materials))


def write_reports(output, report, rig, shared_hashes):
    textures = [dict(texture=f'Zeus_{role}.jpg', material_role=role)
                for role in ('Blue', 'Platina', 'Emissive')]
    dependencies = [dict(item, status='MISSING_ATLAS_NOT_FOR_CLIENT') for item in textures]
    report_doc = dict(status='AUTHORED_CAPE_PROTOTYPE_PREVIEW_ONLY',
                      renderer='Blender Cycles / AgX, not MU runtime',
                      design_reference='art-source/zeus/design-spec.md',
                      cloth_contract='art-source/zeus/cape-cloth-contract.md',
                      model_origin='Original Zeus cape geometry authored in the player bind '
                                   'world space; single-influence binding to the native cape bone '
                                   '"collar" through the reconstructed RenderLinkObject matrix, '
                                   'no native mesh reuse',
                      native_contract=dict(
                          member=rig['contract']['member'],
                          native_sha256=rig['contract']['sha256'],
                          bone=rig['bone0']['name'], bones=len(rig['native']['bones']),
                          pose_frames=rig['contract']['action_frames'],
                          native_meshes=rig['contract']['meshes'],
                          native_triangles=rig['contract']['triangles'],
                          audit='native-reference-audit.json',
                          audit_sha256=digest(ROOT / 'native-reference-audit.json')),
                      link_math=rig['link'],
                      player_anchor=rig['player_anchor'],
                      cape_root_translation=rig['cape_root_translation'],
                      triangle_budget=dict(low=TRIANGLE_BUDGET[0], high=TRIANGLE_BUDGET[1],
                                           justification=BUDGET_JUSTIFICATION),
                      cloth_coordination=dict(
                          main_grid_pin='bone 19 anchor (0, 8, 10), top row 0.6 x 180 wide',
                          rigid_panel_plane_y=PANEL_PLANE,
                          collision_spheres='bones 17 (r 25-27) and 2 (r 30) kept clear',
                          class_gate='bCloak is CLASS_DARK_LORD/CLASS_RAGEFIGHTER (+ literal '
                                     'CLASS_DARK) only; a Duel Master cape needs its own cloth '
                                     'case - see cape-cloth-contract.md'),
                      texture_dependencies=dependencies,
                      shared_source_sha256=shared_hashes,
                      blend_sha256=digest(output / 'zeus-cape.blend'),
                      piece=report)
    (output / 'cape-build-report.json').write_text(json.dumps(report_doc, indent=2), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skip-render', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    output = ROOT / 'prototype'
    for folder in ('models', 'renders'):
        (output / folder).mkdir(parents=True, exist_ok=True)
    source_files = ([SHARED / name for name in SHARED_SOURCES] +
                    [ROOT / name for name in FAMILY_SOURCES])
    before = {path.name: digest(path) for path in source_files}
    rig = load_cape_rig()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    item, report = build_cape(create_cape_materials(), rig, output)
    studio()
    if not args.skip_render:
        render_cape(bpy.context.scene, output / 'renders')
    item.hide_render = True
    item.hide_viewport = False
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'zeus-cape.blend'))
    if before != {path.name: digest(path) for path in source_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, report, rig, before)
    print('ZEUS_CAPE_VERIFIED', json.dumps(
        dict(triangles=report['triangles'], meshes=report['meshes'],
             textures=report['textures'], used_bones=report['used_bones'],
             roundtrip_max_error=report['roundtrip_max_error'])), flush=True)


if __name__ == '__main__':
    main()
