"""Keep the MU flap intact and add a dedicated sun-wheel effect anchor."""
import copy

import bpy
from mathutils import Vector

from armor_animation import set_pose, verify_pose
from import_native_reference import world_matrices

HALO_CENTER = (0, 19, 45)
HALO_BONE = 47
FRAME_STRIDE = 4


def with_halo_anchor(native):
    model = copy.deepcopy(native)
    if len(model['bones']) != HALO_BONE or len(model['action_frames']) != 1:
        raise ValueError('Unexpected wing animation contract')
    root = world_matrices(model, 0)[0]
    position = tuple(root.inverted() @ Vector(HALO_CENTER))
    count = model['action_frames'][0]
    model['bones'].append(dict(name='Celestial_Halo', parent=0,
                              clips=[dict(positions=[position] * count, rotations=[(0, 0, 0)] * count)]))
    return model


def vertices(objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    return [vertex.co.copy() for obj in objects
            for vertex in obj.evaluated_get(depsgraph).data.vertices]


def animate(rig, model, objects):
    maximum, checked, poses = 0, 0, []
    for native_frame in range(model['action_frames'][0]):
        timeline = native_frame * FRAME_STRIDE + 1
        matrices = world_matrices(model, native_frame)
        set_pose(rig, model, matrices, timeline)
        error, count = verify_pose(rig, matrices, objects)
        maximum, checked = max(maximum, error), checked + count
        poses.append(vertices(objects))
    for curve in rig.animation_data.action.fcurves:
        for point in curve.keyframe_points:
            point.interpolation = 'LINEAR'
    bpy.context.scene.frame_end = timeline
    bpy.context.scene.frame_set(1)
    middle = poses[len(poses) // 2]
    moved = sum((a - b).length > .05 for a, b in zip(poses[0], middle))
    seam = max((a - b).length for a, b in zip(poses[0], poses[-1]))
    if moved < len(poses[0]) // 3:
        raise ValueError('Too few wing vertices move in the flap')
    return dict(checked_vertices=checked, maximum_error=maximum, moving_vertices=moved,
                vertices_per_pose=len(poses[0]), loop_seam_distance=seam)
