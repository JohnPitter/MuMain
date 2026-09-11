"""Pose the authored pieces on the real player rig for fit inspection.

Attachment-math preview reconstructed in Blender, not an ingame capture: the
exported BMDs are decoded and placed with the player's own bone matrices from
the frozen base, next to a neutral mannequin built from generic primitives.
Run: blender --background --python-exit-code 1 --python poseidon_armor_equipped.py
"""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from armor_animation import action_indices
from celestial_geometry import tube
from import_native_reference import create_mesh, world_matrices
from inspect_bmd_rig import inspect
from poseidon_armor_preview import point_at, studio
from poseidon_armor_rig import USED_BONE_CONTRACT, load_player_rig
from poseidon_materials import create_materials

ENUM_PATH = ROOT.parents[1] / 'src' / 'source' / 'Core' / 'Globals' / '_enum.h'
OUTPUT = ROOT / 'prototype'
POSES = (('equipped_front', 'PLAYER_STOP_SWORD', 0, (0, -620, 158), (0, 0, 112)),
         ('equipped_side', 'PLAYER_WALK_SWORD', 3, (620, -40, 132), (0, 0, 100)))
ORTHO_SCALE = 430
NEAR_ANCHOR, FAR_ANCHOR = 20.0, 65.0


def mannequin_material():
    material = bpy.data.materials.new('PREVIEW_fit_mannequin')
    material.use_nodes = True
    shader = next(node for node in material.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    shader.inputs['Base Color'].default_value = (.28, .3, .34, 1)
    shader.inputs['Metallic'].default_value = .1
    shader.inputs['Roughness'].default_value = .85
    material['preview_only'] = True
    return material


def mannequin(matrices, material):
    """Neutral body stand-in from generic tubes; never exported."""
    at = lambda index: matrices[index].translation
    up = lambda index, height: at(index) + matrices[index].to_3x3() @ Vector((0, 0, height))
    parts = (('torso', [at(2), at(17), at(18), at(19)], 7),
             ('head', [at(20), up(20, 12)], 6.2),
             ('arm_R', [at(25), at(26), at(27), at(28)], 2.6),
             ('arm_L', [at(34), at(35), at(36), at(37)], 2.6),
             ('shoulder_bar', [at(26), at(19), at(35)], 2.4),
             ('leg_R', [at(10), at(11), at(12)], 3.1),
             ('leg_L', [at(3), at(4), at(5)], 3.1),
             ('foot_R', [at(12), at(13)], 2),
             ('foot_L', [at(5), at(6)], 2))
    for name, path, radius in parts:
        tube(f'PREVIEW mannequin {name}', [tuple(point) for point in path], radius, material, sides=10)


def piece_center(model, matrices):
    corners = [matrices[bone] @ Vector(point)
               for mesh in model['meshes'] for bone, point in mesh['points']]
    return sum(corners, Vector()) / len(corners)


def fit_deviation(model, matrices):
    """Pose every decoded vertex and measure how far it sits from its own bone."""
    distances = [(matrices[bone].translation -
                  (matrices[bone] @ Vector(point)).xyz).length
                 for mesh in model['meshes'] for bone, point in mesh['points']]
    return dict(nearest=round(min(distances), 4), farthest=round(max(distances), 4))


def surface_objects(model, matrices, palette):
    objects = []
    for index, source in enumerate(model['meshes']):
        obj = create_mesh(source, None, matrices, index)
        material = palette.get(source['texture'])
        if material is None:
            raise ValueError(f'No preview material for {source["texture"]}')
        obj.data.materials.append(material)
        objects.append(obj)
    return objects


def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def main():
    player = load_player_rig()
    clips = action_indices(ENUM_PATH)
    models = {name: inspect(OUTPUT / 'models' / f'Poseidon_{name}.bmd', True)
              for name in USED_BONE_CONTRACT}
    palette = {material.name: material for material in create_materials().values()}
    gray = mannequin_material()
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 900, 1280
    poses, probes, centers = [], [], {}
    for name, clip, frame, camera_location, target in POSES:
        clear_scene()
        studio(112)
        matrices = world_matrices(player, frame, clips[clip])
        mannequin(matrices, gray)
        deviations = {}
        for piece, bones in USED_BONE_CONTRACT.items():
            surface_objects(models[piece], matrices, palette)
            deviations[piece] = fit_deviation(models[piece], matrices)
            centers.setdefault(piece, []).append(piece_center(models[piece], matrices))
        drifted = {piece: probe for piece, probe in deviations.items()
                   if probe['nearest'] > NEAR_ANCHOR or probe['farthest'] > FAR_ANCHOR}
        if drifted:
            raise ValueError(f'Piece vertices drifted from their rig bones: {drifted}')
        scene.camera.location = camera_location
        point_at(scene.camera, target)
        scene.camera.data.ortho_scale = ORTHO_SCALE
        scene.camera.data.clip_end = 2000
        scene.render.filepath = str(OUTPUT / 'renders' / f'Poseidon_ArmorSet_{name}.png')
        bpy.ops.render.render(write_still=True)
        poses.append(dict(render=f'Poseidon_ArmorSet_{name}.png', clip=clip,
                          action=clips[clip], frame=frame, view=name.split('_')[1]))
        probes.append(dict(clip=clip, frame=frame, vertex_anchor_distance=deviations))
        print('POSEIDON_EQUIPPED_POSE', json.dumps(poses[-1]), flush=True)
    walked = (centers['Boot'][1] - centers['Boot'][0]).length
    if walked < 3:
        raise ValueError(f'Player animation did not move the boots between poses: {walked}')
    report = dict(status='ATTACHMENT_MATH_PREVIEW_NOT_INGAME',
                  mannequin='PREVIEW_fit_mannequin generic tubes; never exported',
                  player_sha256=player['sha256'], player_bones=len(player['bones']),
                  anchor_gates=dict(nearest_below=NEAR_ANCHOR, farthest_below=FAR_ANCHOR,
                                    boot_pose_shift_min=3.0, boot_pose_shift=round(walked, 4)),
                  poses=poses, fit_probes=probes)
    (OUTPUT / 'equipped-review-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('POSEIDON_EQUIPPED_VERIFIED', json.dumps(report['fit_probes']), flush=True)


if __name__ == '__main__':
    main()
