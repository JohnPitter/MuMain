"""Studio framing for the pants and gloves prototypes; not MU screenshots."""
import math

import bpy

from poseidon_armor_preview import point_at

VIEWS = {'front': 0, 'side': 72, 'back': 180}
FRAMING = {'Pant': (92, 150), 'Glove': (100, 150)}


def frame_piece(scene, name, view):
    target_z, scale = FRAMING[name]
    angle = math.radians(VIEWS[view])
    scene.camera.location = (420 * math.sin(angle), -420 * math.cos(angle), target_z)
    point_at(scene.camera, (0, 0, target_z))
    scene.camera.data.ortho_scale = scale
    scene.camera.data.clip_end = 2000


def render_piece(collections, name, destination):
    scene = bpy.context.scene
    for key, collection in collections.items():
        collection.hide_render = key != name
    for view in VIEWS:
        frame_piece(scene, name, view)
        scene.render.filepath = str(destination / f'Poseidon_{name}_{view}.png')
        bpy.ops.render.render(write_still=True)
