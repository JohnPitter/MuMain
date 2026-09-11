"""Build the original Poseidon Imperial Eagle on the audited DarkSpirit rig.

Regenerable Python source skinned with single-influence bindings to the
frozen 77-bone darkspirit pet skeleton, diagnostic BMD export that preserves
the complete four-action native animation block (never installable, atlases
deliberately absent), bit-exact skeleton proof, BMD roundtrip and front/
side/back plus detail inspection renders.
Run: blender --background --python-exit-code 1 --python build_poseidon_eagle.py [-- --skip-render]
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

from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from poseidon_armor_rig import decoded_skinned_triangles, digest
from poseidon_armor_preview import studio
from poseidon_eagle_rig import ACTION_EVIDENCE, ACTION_NAMES, MODEL_NAME, load_native_rig
from poseidon_eagle_shapes import eagle
from poseidon_pets_export import PetExportSpec, export
from poseidon_pets_preview import frame_target
from poseidon_materials import create_materials
from verify_prop_roundtrip import compare

COLLECTION = 'Poseidon Imperial Eagle'
# Task envelope for the eagle: 3k-6k justified triangles, near twice the
# measured native darkspirit (1001) since the authored bird has full layered
# feather rows, gold coverts, beak, talons and tail fan.
TRIANGLE_BUDGET = (3000, 6000)
BUDGET_JUSTIFICATION = {
    'body_breast': 'Three black body sweeps with gold breast edge and ocean crystal.',
    'head_crest': 'Head sweep, hooked gold beak, blue eyes, crest and cheek feathers.',
    'wings': 'Two layered wings: arm shells plus ~58 feathers with gold coverts.',
    'tail': 'Black tail fan with a gold vane pair on the native tail chain.',
    'legs': 'Feathered thighs, gold rings, gold feet and real talons.',
}
PART_PREFIXES = (
    ('body_breast', ('Eagle_body', 'Eagle_chest', 'Eagle_breast', 'Eagle_collar')),
    ('head_crest', ('Eagle_head', 'Eagle_beak', 'Eagle_eye', 'Eagle_crest', 'Eagle_cheek')),
    ('wings', ('Eagle_wing', 'Eagle_wrist')),
    ('tail', ('Eagle_tail',)),
    ('legs', ('Eagle_leg', 'Eagle_talon')),
)
MIN_TRIANGLE_AREA = 1e-8
SHARED_SOURCES = ('armor_surfaces.py', 'celestial_geometry.py', 'export_prop_bmd.py',
                  'import_native_reference.py', 'inspect_bmd_rig.py',
                  'verify_prop_roundtrip.py')
VIEWS = (('front', 0, (0, -6, 40), 400), ('side', 72, (0, -6, 42), 330),
         ('back', 180, (0, 20, 40), 400), ('detail_head', -30, (0, -42, 26), 90),
         ('detail_wing', 92, (95, -20, 68), 170))


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
        raise ValueError(f'Degenerate triangle in authored eagle: {minimum}')
    return minimum


def part_triangles(objects):
    """Exact per-part triangle counts from the authored object names."""
    counts = {part: 0 for part, _ in PART_PREFIXES}
    for obj in objects:
        obj.data.calc_loop_triangles()
        for part, prefixes in PART_PREFIXES:
            if obj.name.startswith(prefixes):
                counts[part] += len(obj.data.loop_triangles)
                break
        else:
            raise ValueError(f'Object outside the part budgets: {obj.name}')
    return counts


def render_views(output):
    scene = bpy.context.scene
    for name, azimuth, center, scale in VIEWS:
        frame_target(scene, center, azimuth, scale=scale)
        scene.render.filepath = str(output / 'renders' / f'{MODEL_NAME}_{name}.png')
        bpy.ops.render.render(write_still=True)


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
    item = collection(COLLECTION, lambda: eagle(create_materials()))
    bpy.context.view_layer.update()
    topology = topology_report(item.objects)
    minimum_area = triangle_area_report(item.objects)
    target = output / 'models' / f'{MODEL_NAME}.bmd'
    report = export(item, rig['binds'], PetExportSpec(rig['model'], target))
    if not TRIANGLE_BUDGET[0] <= report['triangles'] <= TRIANGLE_BUDGET[1]:
        raise ValueError(f'Eagle outside its triangle budget: {report["triangles"]}')
    model = inspect(target, True)
    native = rig['model']
    # The authored file must carry the frozen skeleton bit-for-bit: names,
    # parents, every clip of every action, locks and locked root positions.
    if model['bones'] != native['bones'] or \
            model['action_frames'] != native['action_frames'] or \
            model['action_locks'] != native['action_locks'] or \
            model['action_positions'] != native['action_positions']:
        raise ValueError('Exported eagle skeleton diverges from the audited darkspirit contract')
    report.update(topology=topology, minimum_triangle_area=minimum_area,
                  sha256=digest(target),
                  used_bones=sorted({bone for mesh in model['meshes']
                                     for bone in mesh['used_bones']}),
                  roundtrip_max_error=compare(expected_triangles(item.objects, rig['binds']),
                                              decoded_skinned_triangles(model)),
                  action_names=ACTION_NAMES, action_evidence=ACTION_EVIDENCE,
                  part_triangles=part_triangles(item.objects))
    print('POSEIDON_EAGLE_AUTHORED', json.dumps(report), flush=True)
    studio(40)
    if not args.skip_render:
        render_views(output)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'poseidon-eagle.blend'))
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    full = dict(status='AUTHORED_EAGLE_PROTOTYPE_PREVIEW_ONLY',
                renderer='Blender Cycles / AgX, not MU runtime',
                design_reference='art-source/poseidon/design-spec.md',
                model_origin='Original Poseidon geometry on the audited native darkspirit skeleton; '
                             'single-influence rig binding, no native mesh reuse, full 4-action '
                             'animation block preserved bit-for-bit',
                native_rig=dict(archive=rig['archive'], archive_sha256=rig['archive_sha256'],
                                audit='native-reference-audit.json',
                                audit_sha256=digest(ROOT / 'native-reference-audit.json'),
                                member='Data/Skill/darkspirit.bmd',
                                model_sha256=native['sha256'],
                                bones=len(native['bones']),
                                action_frames=list(native['action_frames']),
                                action_names=ACTION_NAMES, action_evidence=ACTION_EVIDENCE,
                                used_bone_contract=rig['used_bone_contract']),
                design_decisions={
                    'contract': 'DarkSpirit pet rig per design-spec "Rig DarkSpirit separado do '
                                'personagem"; not a shoulder ornament (task decision documented)',
                    'effect_bones': 'crest 7/8/9 and tail tip 76 stay unused (animated without any '
                                    'native mesh binding - the effect-bone pattern)'},
                triangle_budget=list(TRIANGLE_BUDGET),
                budget_justification=BUDGET_JUSTIFICATION,
                shared_source_sha256=before,
                blend_sha256=digest(output / 'poseidon-eagle.blend'),
                pieces={MODEL_NAME: report})
    (output / 'eagle-build-report.json').write_text(json.dumps(full, indent=2), encoding='utf-8')
    print('POSEIDON_EAGLE_VERIFIED', json.dumps(
        dict(triangles=report['triangles'], meshes=report['meshes'], actions=report['actions'],
             used_bones=report['used_bones'],
             roundtrip_max_error=report['roundtrip_max_error'])), flush=True)


if __name__ == '__main__':
    main()
