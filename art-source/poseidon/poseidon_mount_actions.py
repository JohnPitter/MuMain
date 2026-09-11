"""Prove the authored mount deforms correctly under the native DarkHorse clips.

Action-pose preview reconstructed in Blender, not an ingame capture: the saved
scene is attached to an armature built from the exported skeleton, posed with
every frame of the proven actions (stand, gallop, earthshake skill), verified
against the bone matrices and rendered at a representative frame of each.
Run: blender --background --python-exit-code 1 --python poseidon_mount_actions.py
"""
import json
import sys
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from armor_animation import set_pose, verify_pose
from armor_surfaces import add_skin
from import_native_reference import create_rig, world_matrices
from inspect_bmd_rig import inspect
from poseidon_mount_rig import ACTION_NAMES, MODEL_NAME, USED_BONE_CONTRACT
from poseidon_pets_preview import frame_target

COLLECTION = 'Poseidon Black Mount'
OUTPUT = ROOT / 'prototype'
POSE_TOLERANCE = 2e-4
# (report name, native action index, render frame) - GOBoid.cpp mount branch.
ACTION_PROOFS = (('stand', 0, 3), ('gallop', 1, 3), ('earthshake', 3, 6))
VIEWS = (('side', 72, (0, -20, 150), 380), ('front', 0, (0, 6, 150), 320))


def main():
    model = inspect(OUTPUT / 'models' / f'{MODEL_NAME}.bmd', True)
    if tuple(model['action_frames']) not in ((7, 7, 6, 10, 41, 51, 31),):
        raise ValueError('Mount action contract changed')
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 900, 1280
    proofs = []
    for name, action, render_frame in ACTION_PROOFS:
        # The saved build scene already carries the Cycles studio and camera.
        bpy.ops.wm.open_mainfile(filepath=str(OUTPUT / 'poseidon-mount.blend'))
        scene = bpy.context.scene
        collection = bpy.data.collections[COLLECTION]
        collection.hide_viewport = False
        bpy.context.view_layer.update()
        objects = list(collection.objects)
        bind = world_matrices(model, 0)
        rig = create_rig(model, bind)
        add_skin(collection, rig)
        maximum, checked = 0, 0
        for frame in range(model['action_frames'][action]):
            matrices = world_matrices(model, frame, action)
            set_pose(rig, model, matrices, frame + 1)
            error, count = verify_pose(rig, matrices, objects)
            maximum, checked = max(maximum, error), checked + count
        if maximum > POSE_TOLERANCE:
            raise ValueError(f'{name}: authored mount deformation mismatch {maximum}')
        matrices = world_matrices(model, render_frame, action)
        set_pose(rig, model, matrices, model['action_frames'][action] + 2)
        scene.frame_set(model['action_frames'][action] + 2)
        for view, azimuth, center, scale in VIEWS:
            frame_target(scene, center, azimuth, scale=scale)
            scene.render.filepath = str(
                OUTPUT / 'renders' / f'{MODEL_NAME}_action_{name}_{view}.png')
            bpy.ops.render.render(write_still=True)
        proofs.append(dict(action=action, name=ACTION_NAMES[action], proof=name,
                           frames=model['action_frames'][action],
                           render_frame=render_frame, checked_vertices=checked,
                           maximum_error=maximum,
                           renders=[f'{MODEL_NAME}_action_{name}_{view}.png'
                                    for view, _, _, _ in VIEWS]))
        print('POSEIDON_MOUNT_ACTION_POSE', json.dumps(proofs[-1]), flush=True)
    report = dict(status='ACTION_MATH_PREVIEW_NOT_INGAME',
                  model_sha256=model['sha256'], pose_tolerance=POSE_TOLERANCE,
                  action_evidence=ACTION_NAMES,
                  used_bone_contract=USED_BONE_CONTRACT, proofs=proofs)
    (OUTPUT / 'mount-actions-report.json').write_text(json.dumps(report, indent=2),
                                                      encoding='utf-8')
    print('POSEIDON_MOUNT_ACTIONS_VERIFIED', json.dumps(
        {proof['proof']: proof['maximum_error'] for proof in proofs}), flush=True)


if __name__ == '__main__':
    main()
