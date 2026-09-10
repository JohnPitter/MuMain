"""Sample actual player clips onto authored armor without changing the skeleton contract."""
import re

import bpy
from mathutils import Vector

from import_native_reference import world_matrices

CLIPS = ('PLAYER_STOP_SWORD', 'PLAYER_WALK_SWORD', 'PLAYER_RUN_SWORD',
         'PLAYER_SKILL_HAND1', 'PLAYER_DIE1', 'PLAYER_STOP_FLY', 'PLAYER_FLY',
         'PLAYER_ATTACK_SWORD_RIGHT1')
FRAME_STRIDE = 4
POSE_TOLERANCE = .0002


def action_indices(path):
    source = path.read_text(encoding='utf-8-sig')
    source = source[source.index('    PLAYER_SET,'):source.index('    MAX_PLAYER_ACTION,')]
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    values, current = {}, 0
    for token in source.split(','):
        token = token.strip()
        if not token:
            continue
        fields = [field.strip() for field in token.split('=')]
        if len(fields) == 2:
            current = values[fields[1]] if fields[1] in values else int(fields[1])
        values[fields[0]] = current
        current += 1
    return {name: values[name] for name in CLIPS}


def set_pose(rig, bind_model, matrices, frame):
    bpy.context.scene.frame_set(frame)
    for index, source in enumerate(bind_model['bones']):
        bone = rig.pose.bones[f'mu_{index:03d}']
        rest = bone.bone.matrix_local
        parent = source.get('parent', -1)
        if parent < 0:
            basis = rest.inverted() @ matrices[index]
        else:
            parent_rest = rig.data.bones[f'mu_{parent:03d}'].matrix_local
            basis = rest.inverted() @ parent_rest @ matrices[parent].inverted() @ matrices[index]
        bone.rotation_mode = 'QUATERNION'
        bone.location, bone.rotation_quaternion, bone.scale = basis.decompose()
        bone.keyframe_insert('location', frame=frame)
        bone.keyframe_insert('rotation_quaternion', frame=frame)
    bpy.context.view_layer.update()


def verify_pose(rig, matrices, objects):
    maximum, checked = 0, 0
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for obj in objects:
        evaluated = obj.evaluated_get(depsgraph).data
        for vertex, actual in zip(obj.data.vertices, evaluated.vertices):
            group = obj.vertex_groups[vertex.groups[0].group].name
            bone = int(group.removeprefix('mu_'))
            inverse = rig.data.bones[group].matrix_local.inverted()
            expected = matrices[bone] @ inverse @ vertex.co
            maximum = max(maximum, (expected - actual.co).length)
            checked += 1
    if maximum > POSE_TOLERANCE:
        raise ValueError(f'Authored armor deformation mismatch: {maximum}')
    return maximum, checked


def animate(rig, bind_model, player, objects, indices):
    reports, timeline = [], 1
    set_pose(rig, bind_model, world_matrices(bind_model, 0), timeline)
    for name, action in indices.items():
        timeline += FRAME_STRIDE
        start, maximum, checked = timeline, 0, 0
        bpy.context.scene.timeline_markers.new(name, frame=timeline)
        for native_frame in range(player['action_frames'][action]):
            matrices = world_matrices(player, native_frame, action)
            set_pose(rig, bind_model, matrices, timeline)
            error, count = verify_pose(rig, matrices, objects)
            maximum, checked = max(maximum, error), checked + count
            timeline += FRAME_STRIDE
        reports.append(dict(name=name, action=action, start=start, end=timeline - FRAME_STRIDE,
                            checked_vertices=checked, maximum_error=maximum))
    for curve in rig.animation_data.action.fcurves:
        for point in curve.keyframe_points:
            point.interpolation = 'LINEAR'
    bpy.context.scene.frame_end = timeline - FRAME_STRIDE
    bpy.context.scene.frame_set(1)
    return reports
