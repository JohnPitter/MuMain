"""Run inside Blender to preserve a native BMD rig as an editable reference."""
import sys
from pathlib import Path

import bpy
from mathutils import Euler, Matrix, Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from inspect_bmd_rig import inspect


def world_matrices(model, frame, action=0):
    matrices = []
    for index, bone in enumerate(model['bones']):
        if bone.get('dummy'):
            matrices.append(Matrix.Identity(4))
            continue
        clip = bone['clips'][action]
        local = Euler(clip['rotations'][frame], 'XYZ').to_matrix().to_4x4()
        local.translation = Vector(clip['positions'][frame])
        parent = bone['parent']
        if parent >= index:
            raise ValueError('Unsupported bone ordering')
        matrices.append(matrices[parent] @ local if parent >= 0 else local)
    return matrices


def create_rig(model, bind):
    data = bpy.data.armatures.new('NativeRig')
    rig = bpy.data.objects.new('NativeRig', data)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for index, source in enumerate(model['bones']):
        bone = data.edit_bones.new(f"mu_{index:03d}")
        bone.head = bind[index].translation
        bone.tail = bone.head + bind[index].to_3x3().col[1] * 5
        bone.align_roll(bind[index].to_3x3().col[2])
        parent = source.get('parent', -1)
        if parent >= 0:
            bone.parent = data.edit_bones[f"mu_{parent:03d}"]
    bpy.ops.object.mode_set(mode='OBJECT')
    for index, source in enumerate(model['bones']):
        data.bones[f"mu_{index:03d}"]['source_name'] = source.get('name', '')
    return rig


def create_mesh(source, rig, bind, index):
    points = [bind[node] @ Vector(point) for node, point in source['points']]
    data = bpy.data.meshes.new(f'NativeMesh{index}')
    data.from_pydata(points, [], [face[0] for face in source['faces']])
    data.update()
    obj = bpy.data.objects.new(data.name, data)
    bpy.context.collection.objects.link(obj)
    obj['source_texture'] = source['texture']
    layer = data.uv_layers.new(name='NativeUV')
    for polygon, (_, uv_indices) in zip(data.polygons, source['faces']):
        for loop, uv_index in zip(polygon.loop_indices, uv_indices):
            u, v = source['texcoords'][uv_index]
            layer.data[loop].uv = (u, 1 - v)
    for node in source['used_bones']:
        group = obj.vertex_groups.new(name=f'mu_{node:03d}')
        group.add([i for i, (bone, _) in enumerate(source['points']) if bone == node], 1, 'REPLACE')
    obj.modifiers.new('NativeSkin', 'ARMATURE').object = rig
    return obj


def animate(model, rig):
    for frame in range(model['action_frames'][0]):
        bpy.context.scene.frame_set(frame + 1)
        matrices = world_matrices(model, frame)
        for index, matrix in enumerate(matrices):
            bone = rig.pose.bones[f'mu_{index:03d}']
            bone.rotation_mode = 'QUATERNION'
            rest = bone.bone.matrix_local
            parent = model['bones'][index].get('parent', -1)
            if parent >= 0:
                parent_rest = rig.data.bones[f'mu_{parent:03d}'].matrix_local
                basis = rest.inverted() @ parent_rest @ matrices[parent].inverted() @ matrix
            else:
                basis = rest.inverted() @ matrix
            bone.location, bone.rotation_quaternion, bone.scale = basis.decompose()
            bone.keyframe_insert('location', frame=frame + 1)
            bone.keyframe_insert('rotation_quaternion', frame=frame + 1)
    bpy.context.scene.frame_end = model['action_frames'][0]
    rig.animation_data.action.name = 'Native_Action_0'


def main():
    args = sys.argv[sys.argv.index('--') + 1:]
    model = inspect(Path(args[0]), full=True)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bind = world_matrices(model, 0)
    rig = create_rig(model, bind)
    meshes = [create_mesh(mesh, rig, bind, i) for i, mesh in enumerate(model['meshes'])]
    animate(model, rig)
    bpy.context.scene.frame_set(1)
    bpy.context.view_layer.update()
    for index, matrix in enumerate(bind):
        actual = rig.data.bones[f'mu_{index:03d}'].matrix_local
        error = max(abs(actual[row][col] - matrix[row][col]) for row in range(4) for col in range(4))
        if error > 0.001:
            raise ValueError(f'Bind pose mismatch at bone {index}: {error}')
    first = [tuple(v.co) for obj in meshes for v in obj.evaluated_get(bpy.context.evaluated_depsgraph_get()).data.vertices]
    bpy.context.scene.frame_set(5)
    bpy.context.view_layer.update()
    last = [tuple(v.co) for obj in meshes for v in obj.evaluated_get(bpy.context.evaluated_depsgraph_get()).data.vertices]
    moved = sum((Vector(a) - Vector(b)).length > 0.001 for a, b in zip(first, last))
    if not moved:
        raise ValueError('Imported animation does not deform vertices')
    bpy.context.scene['verified_moving_vertices'] = moved
    bpy.context.scene['source_sha256'] = model['sha256']
    bpy.context.scene.frame_set(1)
    bpy.ops.wm.save_as_mainfile(filepath=str(Path(args[1]).resolve()))
    print(f'VERIFIED: {moved} / {len(first)} vertices move between frames 1 and 5')


if __name__ == '__main__':
    main()
