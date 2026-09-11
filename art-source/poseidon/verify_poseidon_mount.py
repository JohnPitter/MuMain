"""Reopen the saved mount scene and independently decode the exported BMD."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
sys.path[:0] = [str(ROOT), str(ROOT.parent / 'celestial')]

from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from poseidon_armor_rig import decoded_skinned_triangles, digest
from poseidon_mount_rig import MODEL_NAME, USED_BONE_CONTRACT, load_native_rig
from verify_prop_roundtrip import compare

COLLECTION = 'Poseidon Black Mount'


def expected_triangles(objects, bind):
    grouped = triangle_groups(objects, Matrix.Identity(4), bind)
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'poseidon-mount.blend'
    rig = load_native_rig()
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    collection = bpy.data.collections[COLLECTION]
    collection.hide_viewport = False
    bpy.context.view_layer.update()
    model = inspect(output / 'models' / f'{MODEL_NAME}.bmd', True)
    expected = expected_triangles(collection.objects, rig['binds'])
    native = rig['model']
    if model['bones'] != native['bones'] or \
            model['action_frames'] != native['action_frames'] or \
            model['action_locks'] != native['action_locks'] or \
            model['action_positions'] != native['action_positions']:
        raise ValueError('Saved-scene mount skeleton diverges from the audited DarkHorse contract')
    used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
    if tuple(used) != USED_BONE_CONTRACT:
        raise ValueError(f'Mount used bones {used} violate the native contract')
    report = dict(saved_blend_sha256=digest(scene_file),
                  models={MODEL_NAME: dict(
                      max_error=compare(expected, decoded_skinned_triangles(model)),
                      triangles=sum(len(triangles) for triangles in expected.values()),
                      sha256=model['sha256'], actions=len(model['action_frames']),
                      action_frames=list(model['action_frames']),
                      used_bones=used)}) 
    (output / 'mount-saved-roundtrip-report.json').write_text(json.dumps(report, indent=2),
                                                              encoding='utf-8')
    print('POSEIDON_MOUNT_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
