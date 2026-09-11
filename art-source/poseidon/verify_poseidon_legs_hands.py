"""Reopen the saved legs/hands scene and independently decode the exported BMDs."""
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
from poseidon_legs_hands_rig import load_native_rig, USED_BONE_CONTRACT
from verify_prop_roundtrip import compare


def expected_triangles(objects, bind):
    grouped = triangle_groups(objects, Matrix.Identity(4), bind)
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'poseidon-legs-hands.blend'
    rig = load_native_rig()
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    for name in USED_BONE_CONTRACT:
        bpy.data.collections['Poseidon ' + name].hide_viewport = False
    bpy.context.view_layer.update()
    reports = {}
    for name in USED_BONE_CONTRACT:
        collection = bpy.data.collections['Poseidon ' + name]
        model = inspect(output / 'models' / f'Poseidon_{name}.bmd', True)
        expected = expected_triangles(collection.objects, rig['binds'][name])
        reports[name] = dict(max_error=compare(expected, decoded_skinned_triangles(model)),
                             triangles=sum(map(len, expected.values())), sha256=model['sha256'])
        used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
        if tuple(used) != USED_BONE_CONTRACT[name]:
            raise ValueError(f'{name}: used bones {used} violate the native contract')
        if [bone.get('name') for bone in model['bones']] != \
                [bone.get('name') for bone in rig['models'][name]['bones']]:
            raise ValueError(f'{name}: exported skeleton names diverge from the native rig')
    report = dict(saved_blend_sha256=digest(scene_file), models=reports)
    (output / 'legs-hands-saved-roundtrip-report.json').write_text(json.dumps(report, indent=2),
                                                                   encoding='utf-8')
    print('POSEIDON_LEGS_HANDS_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
