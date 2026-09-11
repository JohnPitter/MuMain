"""Pose the authored cape on the real player rig for fit inspection.

Attachment-math preview reconstructed in Blender, not an ingame capture: the
exported cape BMD is decoded and placed with the player's own bone matrices
from the frozen base, composed through the runtime link math of
poseidon_cape_rig (BoneTransform[19] . M1 . M2 . bone0), next to a neutral
mannequin built from generic primitives. Rigid ferragem follows bone 19 in
every pose; the fabric stays procedural cloth owned by the runtime.
Run: blender --background --python-exit-code 1 --python poseidon_cape_equipped.py
"""
import json
import sys
import tempfile
import zipfile
from pathlib import Path

import bpy
from mathutils import Euler, Vector

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from armor_animation import action_indices
from import_native_reference import create_mesh, world_matrices
from inspect_bmd_rig import inspect
from poseidon_armor_equipped import clear_scene, mannequin, mannequin_material
from poseidon_cape_materials import create_cape_materials
from poseidon_cape_preview import point_at, studio
from poseidon_cape_rig import (ARCHIVE, LINK_BONE, PLAYER_MEMBER, link_matrix,
                               load_cape_rig)

ENUM_PATH = ROOT.parents[1] / 'src' / 'source' / 'Core' / 'Globals' / '_enum.h'
OUTPUT = ROOT / 'prototype'
POSES = (('equipped_front', 'PLAYER_STOP_SWORD', 0, (0, -620, 150), (0, 0, 118)),
         ('equipped_back', 'PLAYER_WALK_SWORD', 3, (55, 620, 150), (0, 0, 112)))
ORTHO_SCALE = 430
SPAN_GATE = 135.0
RIGIDITY_GATE = 0.75
FOLLOW_GATE = 3.0


def cape_root_for(player_world, bone0):
    """Per-frame runtime link product: BoneTransform[19] . M1 . M2 . bone0."""
    return player_world[LINK_BONE] @ link_matrix() @ bone0


def bone0_matrix(model):
    clip = model['bones'][0]['clips'][0]
    local = Euler(clip['rotations'][0], 'XYZ').to_matrix().to_4x4()
    local.translation = Vector(clip['positions'][0])
    return local


def surface_objects(model, bind, palette):
    objects = []
    for index, source in enumerate(model['meshes']):
        obj = create_mesh(source, None, bind, index)
        material = palette.get(source['texture'])
        if material is None:
            raise ValueError(f'No preview material for {source["texture"]}')
        obj.data.materials.append(material)
        objects.append(obj)
    return objects


def vertex_span(model, bind):
    """Vertex distances to the cape root translation (rigid ferragem reach)."""
    root = bind[0].translation
    distances = [(root - (bind[bone] @ Vector(point)).xyz).length
                 for mesh in model['meshes'] for bone, point in mesh['points']]
    return dict(nearest=round(min(distances), 4), farthest=round(max(distances), 4))


def load_player():
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='poseidon-cape-eq-') as temp:
        path = Path(temp) / 'player.bmd'
        path.write_bytes(archive.read(PLAYER_MEMBER))
        return inspect(path, True)


def cloth_proxy(player_world, bone0):
    """Translucent stand-in for the runtime's procedural main cloth grid.

    Approximates CPhysicsCloth::Create + SetFixedVertices for the
    MODEL_CAPE_OF_EMPEROR main cape (bone 19 anchor (0, 8, 10), 10x10,
    180x180, SHORT_SHOULDER + CURVED): pinned top row 0.6 x 180 wide at the
    anchor, rows widening to full width, edges curved toward the body. The
    physics sag is not simulated; the fall is an envelope guess. Preview only;
    never exported and never part of the BMD.
    """
    rows, cols = 10, 10
    width, height = 180.0, 180.0
    rate = 0.6
    root = cape_root_for(player_world, bone0)
    anchor_x = 10.0  # bone-local fzPos of the pinned row (world z ~ 167)
    hem_x = -150.0   # envelope keeps the hem near the ground; physics not simulated
    unit = (anchor_x - hem_x) / (rows - 1)
    material = bpy.data.materials.new('PREVIEW_cloth_proxy')
    material.use_nodes = True
    shader = next(node for node in material.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    shader.inputs['Base Color'].default_value = (.05, .07, .12, 1)
    shader.inputs['Alpha'].default_value = 0.16
    shader.inputs['Roughness'].default_value = 0.9
    material['preview_only'] = True
    points, faces = [], []

    def pid(j, i):
        return j * cols + i

    for j in range(rows):
        row_width = width * (rate + (1 - rate) * j / (rows - 1))
        for i in range(cols):
            curve = 2.0 * abs(i / (cols - 1) - 0.5)
            across = row_width * i / (cols - 1) - 0.5 * row_width
            if j == 0:
                bone = Vector((anchor_x, -(8.0 - 10.0 * curve * curve), across))
            else:
                bone = Vector((anchor_x - unit * j, -(20.0 - 10.0 * curve * curve), across))
            points.append(root @ bone)
    for j in range(rows - 1):
        for i in range(cols - 1):
            faces.append((pid(j, i), pid(j, i + 1), pid(j + 1, i + 1), pid(j + 1, i)))
    data = bpy.data.meshes.new('PREVIEW_cloth_proxy')
    data.from_pydata(points, [], faces)
    data.update()
    obj = bpy.data.objects.new('PREVIEW cloth proxy', data)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    return obj


def main():
    rig = load_cape_rig()
    player = load_player()
    clips = action_indices(ENUM_PATH)
    model = inspect(OUTPUT / 'models' / 'Poseidon_Cape.bmd', True)
    bone0 = bone0_matrix(model)
    palette = {material.name: material for material in create_cape_materials().values()}
    gray = mannequin_material()
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = 900, 1280
    poses, centers, spans = [], [], []
    for name, clip, frame, camera_location, target in POSES:
        clear_scene()
        studio()
        matrices = world_matrices(player, frame, clips[clip])
        mannequin(matrices, gray)
        bind = {0: cape_root_for(matrices, bone0)}
        surface_objects(model, bind, palette)
        if name == 'equipped_front':
            # Ghost envelope of the reserved cloth zone, standing pose only: the
            # simulated cloth would not inherit the walk-cycle torso lean that
            # the rigid ferragem faithfully follows via bone 19.
            cloth_proxy(matrices, bone0)
        centers.append(bind[0].translation.copy())
        spans.append(vertex_span(model, bind))
        scene.camera.location = camera_location
        point_at(scene.camera, target)
        scene.camera.data.ortho_scale = ORTHO_SCALE
        scene.camera.data.clip_end = 2000
        scene.render.filepath = str(OUTPUT / 'renders' / f'Poseidon_CapeSet_{name}.png')
        bpy.ops.render.render(write_still=True)
        poses.append(dict(render=f'Poseidon_CapeSet_{name}.png', clip=clip,
                          action=clips[clip], frame=frame, view=name.split('_')[1]))
        print('POSEIDON_CAPE_EQUIPPED_POSE', json.dumps(poses[-1]), flush=True)
    shift = (centers[1] - centers[0]).length
    if shift < FOLLOW_GATE:
        raise ValueError(f'cape root did not follow the player animation: {shift}')
    drift = max(abs(spans[0][key] - spans[1][key]) for key in ('nearest', 'farthest'))
    if drift > RIGIDITY_GATE:
        raise ValueError(f'rigid ferragem deformed between poses: drift {drift}')
    for probe in spans:
        if probe['farthest'] > SPAN_GATE:
            raise ValueError(f'cape ferragem outside its link reach: {probe}')
    report = dict(status='ATTACHMENT_MATH_PREVIEW_NOT_INGAME',
                  mannequin='PREVIEW_fit_mannequin generic tubes; never exported',
                  cloth_proxy='PREVIEW_cloth_proxy translucent envelope of the runtime '
                              'procedural main grid; never exported',
                  player_sha256=player['sha256'], player_bones=len(player['bones']),
                  native_cape_sha256=rig['contract']['sha256'],
                  link_bone=LINK_BONE,
                  cape_root_translation=rig['cape_root_translation'],
                  anchor_gates=dict(farthest_below=SPAN_GATE, root_follow_min=FOLLOW_GATE,
                                    rigidity_drift_max=RIGIDITY_GATE,
                                    root_follow=round(shift, 4),
                                    rigidity_drift=round(drift, 4)),
                  poses=poses, span_probes=spans)
    (OUTPUT / 'cape-equipped-review-report.json').write_text(json.dumps(report, indent=2),
                                                             encoding='utf-8')
    print('POSEIDON_CAPE_EQUIPPED_VERIFIED', json.dumps(report['span_probes']), flush=True)


if __name__ == '__main__':
    main()
