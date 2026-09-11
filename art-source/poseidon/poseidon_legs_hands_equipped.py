"""Pose the authored pants and gloves on the real player rig for fit inspection.

Attachment-math preview reconstructed in Blender, not an ingame capture: the
exported BMDs are decoded and placed with the player's own bone matrices from
the frozen base, next to a neutral mannequin built from generic primitives.
Run: blender --background --python-exit-code 1 --python poseidon_legs_hands_equipped.py
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
from poseidon_armor_equipped import fit_deviation, mannequin, mannequin_material, piece_center
from poseidon_armor_preview import point_at, studio
from poseidon_armor_rig import load_player_rig
from poseidon_legs_hands_rig import USED_BONE_CONTRACT
from poseidon_materials import create_materials

ENUM_PATH = ROOT.parents[1] / 'src' / 'source' / 'Core' / 'Globals' / '_enum.h'
OUTPUT = ROOT / 'prototype'
POSES = (('equipped_front', 'PLAYER_STOP_SWORD', 0, (0, -620, 158), (0, 0, 112)),
         ('equipped_side', 'PLAYER_WALK_SWORD', 3, (620, -40, 132), (0, 0, 100)))
ORTHO_SCALE = 430
NEAR_ANCHOR, FAR_ANCHOR = 20.0, 65.0
MIN_POSE_SHIFT = 3.0


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
        for piece in USED_BONE_CONTRACT:
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
        scene.render.filepath = str(OUTPUT / 'renders' / f'Poseidon_LegsHands_{name}.png')
        bpy.ops.render.render(write_still=True)
        poses.append(dict(render=f'Poseidon_LegsHands_{name}.png', clip=clip,
                          action=clips[clip], frame=frame, view=name.split('_')[1]))
        probes.append(dict(clip=clip, frame=frame, vertex_anchor_distance=deviations))
        print('POSEIDON_LEGS_HANDS_EQUIPPED_POSE', json.dumps(poses[-1]), flush=True)
    shifts = {piece: round((centers[piece][1] - centers[piece][0]).length, 4)
              for piece in centers}
    drifting = {piece: shift for piece, shift in shifts.items() if shift < MIN_POSE_SHIFT}
    if drifting:
        raise ValueError(f'Player animation did not move the worn pieces: {drifting}')
    report = dict(status='ATTACHMENT_MATH_PREVIEW_NOT_INGAME',
                  mannequin='PREVIEW_fit_mannequin generic tubes; never exported',
                  player_sha256=player['sha256'], player_bones=len(player['bones']),
                  anchor_gates=dict(nearest_below=NEAR_ANCHOR, farthest_below=FAR_ANCHOR,
                                    pose_shift_min=MIN_POSE_SHIFT, pose_shifts=shifts),
                  poses=poses, fit_probes=probes)
    (OUTPUT / 'legs-hands-equipped-review-report.json').write_text(
        json.dumps(report, indent=2), encoding='utf-8')
    print('POSEIDON_LEGS_HANDS_EQUIPPED_VERIFIED',
          json.dumps(dict(fit_probes=report['fit_probes'], pose_shifts=shifts)), flush=True)


if __name__ == '__main__':
    main()
