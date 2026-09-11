"""Build the original Zeus Celestial Wings on the frozen Wing44 rig.

Generates the authored wing geometry from regenerable Python sources, exports
a diagnostic BMD with the exact audited Wing44 skeleton (47 bones, 9-frame
locked flap - no renumbering, no extra bones), proves the flap on at least
two native poses, verifies the BMD roundtrip against the editable scene and
renders front/side/back inspections plus flap poses and one view equipped on
the player mannequin. The BMD is never installable: the Zeus atlases do not
exist yet.
Run: blender --background --python-exit-code 1 --python build_zeus_wings.py [-- --skip-render]
"""
import argparse
import json
import sys
import tempfile
import zipfile
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from armor_animation import action_indices, set_pose, verify_pose
from armor_surfaces import add_skin
from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from export_skinned_bmd import ExportSpec, export
from import_native_reference import create_rig, world_matrices
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare
from zeus_armor_equipped import clear_scene, mannequin, mannequin_material, surface_objects
from zeus_armor_preview import point_at, studio
from zeus_armor_rig import decoded_skinned_triangles, load_player_rig
from zeus_materials import PALETTE, create_materials
from zeus_wings_rig import FLAP_FRAMES, USED_BONES, digest, load_wing_rig
from zeus_wings_shapes import wings

# Large, detailed centerpiece: smaller than the Celestial wing candidate
# (28,870 triangles over 114 feathers) because the Zeus design is angular
# plate fans instead of layered plumage, but well above any armor piece.
TRIANGLE_BUDGET = (12000, 20000)
BUDGET_JUSTIFICATION = (
    'Wings are the largest set piece: the authored Celestial wing candidate '
    'measures 28,870 triangles; the Zeus design replaces layered plumage with '
    '78 faceted blades, storm channels and a star crest, landing near half '
    'that cost while staying far above the armor pieces (max 9,400).')
MIN_TRIANGLE_AREA = 1e-8
PROOF_FRAMES = (0, 4, 8)
MIN_MOVED_FRACTION = 0.3
POSE_TOLERANCE = 2e-4
SHARED_SOURCES = ('armor_animation.py', 'armor_surfaces.py', 'celestial_geometry.py',
                  'export_prop_bmd.py', 'export_skinned_bmd.py',
                  'import_native_reference.py', 'inspect_bmd_rig.py',
                  'verify_prop_roundtrip.py')
WING_ATTACH_BONE = 47
WING_ATTACH_OFFSET = (0, 0, 15)


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
        raise ValueError(f'Degenerate triangle in authored Zeus wings: {minimum}')
    return minimum


def expected_triangles(objects, bind):
    grouped = triangle_groups(objects, Matrix.Identity(4), bind)
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


def evaluated_vertices(objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    return [vertex.co.copy() for obj in objects
            for vertex in obj.evaluated_get(depsgraph).data.vertices]


def prove_flap(rig_object, model, objects):
    """Skin every native flap frame and measure real, looping motion."""
    maximum, checked, poses = 0, 0, []
    for frame in range(FLAP_FRAMES):
        matrices = world_matrices(model, frame)
        set_pose(rig_object, model, matrices, frame * 4 + 1)
        error, count = verify_pose(rig_object, matrices, objects)
        maximum, checked = max(maximum, error), checked + count
        poses.append(evaluated_vertices(objects))
    for curve in rig_object.animation_data.action.fcurves:
        for point in curve.keyframe_points:
            point.interpolation = 'LINEAR'
    bpy.context.scene.frame_end = (FLAP_FRAMES - 1) * 4 + 1
    bpy.context.scene.frame_set(1)
    moved = sum((a - b).length > 0.05 for a, b in zip(poses[0], poses[4]))
    seam = max((a - b).length for a, b in zip(poses[0], poses[8]))
    if maximum > POSE_TOLERANCE:
        raise ValueError(f'Flap deformation mismatch: {maximum}')
    if moved < len(poses[0]) * MIN_MOVED_FRACTION:
        raise ValueError(f'Too few wing vertices move in the flap: {moved}')
    if seam > POSE_TOLERANCE:
        raise ValueError(f'Flap loop does not close: {seam}')
    return dict(proof_frames=list(PROOF_FRAMES), checked_vertices=checked,
                maximum_pose_error=maximum, vertices_per_pose=len(poses[0]),
                moving_vertices_pose4=moved, loop_seam_distance=seam)


def frame_wings(scene, view):
    import math
    target = Vector((0, 0, 25))
    angle = {'front': 0.0, 'side': 1.45, 'back': math.pi}[view]
    scene.camera.location = (600 * math.sin(angle), -600 * math.cos(angle), target.z)
    point_at(scene.camera, target)
    scene.camera.data.ortho_scale = 520
    scene.camera.data.clip_end = 2000


def render_wings(scene, destination):
    for view in ('front', 'side', 'back'):
        frame_wings(scene, view)
        scene.render.filepath = str(destination / f'Zeus_Wings_{view}.png')
        bpy.ops.render.render(write_still=True)


def render_flap(scene, destination):
    scene.camera.location = (30, -640, 45)
    point_at(scene.camera, (0, 0, 45))
    scene.camera.data.ortho_scale = 560
    for name, timeline in (('flap_rest', 1), ('flap_beat', 17)):
        bpy.context.scene.frame_set(timeline)
        bpy.context.view_layer.update()
        scene.render.filepath = str(destination / f'Zeus_Wings_{name}.png')
        bpy.ops.render.render(write_still=True)
    bpy.context.scene.frame_set(1)


def equipped_proof(palette_roles, gray):
    """Attach the exported wing BMD to the posed player rig like RenderLinkObject."""
    player = load_player_rig()
    clips = action_indices(ROOT.parents[1] / 'src' / 'source' / 'Core' / 'Globals' / '_enum.h')
    model = inspect(ROOT / 'prototype' / 'models' / 'Zeus_Wings.bmd', True)
    palette = {material.name: material for material in palette_roles.values()}
    clear_scene()
    studio(112)
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 1280, 960
    matrices = world_matrices(player, 0, clips['PLAYER_STOP_SWORD'])
    mannequin(matrices, gray)
    anchor = matrices[WING_ATTACH_BONE]
    offset = anchor.to_3x3() @ Vector(WING_ATTACH_OFFSET)
    wing_frames = [Matrix.Translation(offset) @ anchor @ matrix
                   for matrix in world_matrices(model, 0)]
    surface_objects(model, wing_frames, palette)
    center = Vector((0, 0, 0))
    count = 0
    for mesh in model['meshes']:
        for bone, point in mesh['points']:
            center += wing_frames[bone] @ Vector(point)
            count += 1
    center /= count
    scene.camera.location = (140, -680, 150)
    point_at(scene.camera, (0, 0, 130))
    scene.camera.data.ortho_scale = 640
    scene.camera.data.clip_end = 2000
    scene.render.filepath = str(ROOT / 'prototype' / 'renders' / 'Zeus_Wings_equipped_front.png')
    bpy.ops.render.render(write_still=True)
    # Plausibility gates: the fan must sit behind the torso, roughly centered,
    # towering above the shoulders (concept: raised blade fan) - gates catch a
    # flipped or misplaced attachment, not artistic acceptance.
    gates = dict(max_abs_center_x=25.0, center_y_range=(0.0, 60.0),
                 center_z_range=(150.0, 280.0))
    if not (abs(center.x) < gates['max_abs_center_x']
            and gates['center_y_range'][0] < center.y < gates['center_y_range'][1]
            and gates['center_z_range'][0] < center.z < gates['center_z_range'][1]):
        raise ValueError(f'Wing equipped center implausible: {tuple(round(v, 1) for v in center)}')
    return dict(status='ATTACHMENT_MATH_PREVIEW_NOT_INGAME',
                render='Zeus_Wings_equipped_front.png',
                player_sha256=player['sha256'], player_bones=len(player['bones']),
                clip='PLAYER_STOP_SWORD', attach_bone=WING_ATTACH_BONE,
                attach_local_offset=list(WING_ATTACH_OFFSET),
                attachment_precedent='RenderLinkObject wing link as applied by the '
                                     'authored Celestial equipped review (player bone '
                                     '47 + local Z 15)',
                wing_center_on_rig=[round(value, 2) for value in center],
                plausibility_gates=gates)


def write_reports(output, report, rig, shared_hashes):
    (output / 'texture-dependencies.json').write_text(json.dumps(
        [dict(texture=f'Zeus_{role}.jpg', material_role=role,
              status='MISSING_ATLAS_NOT_FOR_CLIENT', linear_rgb=list(values[0]))
         for role, values in PALETTE.items()], indent=2), encoding='utf-8')
    document = dict(status='AUTHORED_ZEUS_WINGS_PROTOTYPE_PREVIEW_ONLY',
                    renderer='Blender Cycles / AgX, not MU runtime',
                    design_reference='art-source/zeus/design-spec.md',
                    model_origin='Original Zeus wing geometry authored in the Wing44 '
                                 'frame-0 bind space; single-influence binding to the '
                                 'frozen 47-bone skeleton, no native mesh reuse',
                    native_rig=dict(archive=rig['archive'], archive_sha256=rig['archive_sha256'],
                                    audit='native-reference-audit.json',
                                    audit_sha256=digest(ROOT / 'native-reference-audit.json'),
                                    wing44_sha256=rig['model']['sha256'],
                                    bones=len(rig['model']['bones']),
                                    action_frames=rig['model']['action_frames'],
                                    action_locks=rig['model']['action_locks'],
                                    used_bones=list(USED_BONES)),
                    fit_anchors={str(bone): [round(value, 4) for value in rig['bind'][bone].translation]
                                 for bone in USED_BONES},
                    triangle_budget=list(TRIANGLE_BUDGET),
                    budget_justification=BUDGET_JUSTIFICATION,
                    shared_source_sha256=shared_hashes,
                    blend_sha256=digest(output / 'zeus-wings.blend'),
                    wings=report)
    (output / 'wings-build-report.json').write_text(json.dumps(document, indent=2),
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
    rig = load_wing_rig()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    palette = create_materials()
    group = collection('Zeus Wings', lambda: wings(palette, rig['bind']))
    bpy.context.view_layer.update()
    topology = topology_report(group.objects)
    minimum_area = triangle_area_report(group.objects)
    target = output / 'models' / 'Zeus_Wings.bmd'
    report = export(group, rig['bind'], ExportSpec(rig['model'], target, FLAP_FRAMES))
    model = inspect(target, True)
    low, high = TRIANGLE_BUDGET
    if not low <= report['triangles'] <= high:
        raise ValueError(f'Wings outside their triangle budget: {report["triangles"]}')
    used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
    if tuple(used) != USED_BONES:
        raise ValueError(f'Wing used bones {used} violate the contract {USED_BONES}')
    report.update(topology=topology, minimum_triangle_area=minimum_area,
                  sha256=digest(target), used_bones=used,
                  roundtrip_max_error=compare(expected_triangles(group.objects, rig['bind']),
                                              decoded_skinned_triangles(model)))
    if report['roundtrip_max_error'] > 1e-6:
        raise ValueError('Wing BMD roundtrip above the 1e-6 gate')
    print('ZEUS_WINGS_AUTHORED', json.dumps(report), flush=True)
    rig_object = create_rig(rig['model'], rig['bind'])
    rig_object.name = 'ZeusWingRig'
    add_skin(group, rig_object)
    motion = prove_flap(rig_object, rig['model'], list(group.objects))
    report['motion'] = motion
    print('ZEUS_WINGS_FLAP_PROVEN', json.dumps(motion), flush=True)
    # Save the editable scene (geometry + animation) before the equipped proof
    # rebuilds the scene around the mannequin.
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'zeus-wings.blend'))
    studio(40)
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 1280, 960
    if not args.skip_render:
        render_wings(scene, output / 'renders')
        render_flap(scene, output / 'renders')
        equipped = equipped_proof(palette, mannequin_material())
        report['equipped'] = equipped
        print('ZEUS_WINGS_EQUIPPED', json.dumps(equipped), flush=True)
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, report, rig, before)
    print('ZEUS_WINGS_VERIFIED', json.dumps(
        dict(triangles=report['triangles'], meshes=report['meshes'], used_bones=used,
             roundtrip_max_error=report['roundtrip_max_error'], motion=motion)), flush=True)


if __name__ == '__main__':
    main()
