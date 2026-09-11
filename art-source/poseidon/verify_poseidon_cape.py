"""Reopen the saved cape scene and independently decode the exported BMD payload."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
sys.path[:0] = [str(ROOT), str(ROOT.parent / 'celestial')]

from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from poseidon_cape_rig import (cape_bind, decoded_skinned_triangles, digest,
                               load_cape_rig, CAPE_BONE_CONTRACT)
from verify_prop_roundtrip import compare


def expected_triangles(objects, bind):
    grouped = triangle_groups(objects, Matrix.Identity(4), bind)
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'poseidon-cape.blend'
    rig = load_cape_rig()
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    collection = bpy.data.collections['Poseidon Cape']
    collection.hide_viewport = False
    bpy.context.view_layer.update()
    model = inspect(output / 'models' / 'Poseidon_Cape.bmd', True)
    bind = cape_bind(rig)
    expected = expected_triangles(collection.objects, bind)
    max_error = compare(expected, decoded_skinned_triangles(model))
    used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
    if tuple(used) != CAPE_BONE_CONTRACT:
        raise ValueError(f'cape used bones {used} violate the single-bone contract')
    if [bone.get('name') for bone in model['bones']] != \
           [bone.get('name') for bone in rig['native']['bones']]:
        raise ValueError('exported skeleton name diverges from the native cape')
    report = dict(saved_blend_sha256=digest(scene_file),
                  Cape=dict(max_error=max_error,
                            triangles=sum(map(len, expected.values())),
                            sha256=model['sha256']))
    (output / 'cape-saved-roundtrip-report.json').write_text(json.dumps(report, indent=2),
                                                             encoding='utf-8')
    print('POSEIDON_CAPE_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
