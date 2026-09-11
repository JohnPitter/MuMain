"""Reopen the saved Zeus wings scene and re-prove geometry, skin and flap keyframes."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from armor_animation import verify_pose
from export_prop_bmd import triangle_groups
from import_native_reference import world_matrices
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare
from zeus_armor_rig import decoded_skinned_triangles
from zeus_wings_rig import FLAP_FRAMES, digest, load_wing_rig


def minimum_area(objects):
    minimum = float('inf')
    for obj in objects:
        obj.data.calc_loop_triangles()
        for triangle in obj.data.loop_triangles:
            a, b, c = [obj.matrix_world @ obj.data.vertices[i].co for i in triangle.vertices]
            minimum = min(minimum, (b - a).cross(c - a).length / 2)
    return minimum


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'zeus-wings.blend'
    rig = load_wing_rig()
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    group = bpy.data.collections['Zeus Wings']
    group.hide_viewport = False
    bpy.context.view_layer.update()
    objects = list(group.objects)
    model = inspect(output / 'models' / 'Zeus_Wings.bmd', True)
    expected = triangle_groups(objects, Matrix.Identity(4), world_matrices(rig['model'], 0))
    expected = {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                          for triangle in triangles]
                for texture, triangles in expected.items()}
    maximum, checked = 0, 0
    for frame in range(FLAP_FRAMES):
        bpy.context.scene.frame_set(frame * 4 + 1)
        error, count = verify_pose(bpy.data.objects['ZeusWingRig'],
                                   world_matrices(rig['model'], frame), objects)
        maximum, checked = max(maximum, error), checked + count
    used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
    if tuple(used) != rig['used_bones']:
        raise ValueError(f'Saved scene used bones {used} violate the contract')
    report = dict(saved_blend_sha256=digest(scene_file),
                  max_geometry_error=compare(expected, decoded_skinned_triangles(model)),
                  max_pose_error=maximum, checked_vertices=checked,
                  keyframes=FLAP_FRAMES, minimum_triangle_area=minimum_area(objects),
                  triangles=sum(mesh['triangles'] for mesh in model['meshes']),
                  sha256=model['sha256'])
    bpy.context.scene.frame_set(1)
    (output / 'wings-saved-roundtrip-report.json').write_text(json.dumps(report, indent=2),
                                                              encoding='utf-8')
    print('ZEUS_WINGS_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
