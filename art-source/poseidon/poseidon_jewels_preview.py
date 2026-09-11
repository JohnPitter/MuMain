"""Studio framing for the pendant and rings prototypes; not MU screenshots."""
import math

import bpy

from poseidon_armor_preview import point_at

VIEWS = ('front', 'side', 'detail')
# Per piece: front/side (72 degrees oblique) full views and a crest close-up,
# given as (camera target height, ortho scale).
FRAMING = {
    'Pendant': {'front': (3, 64), 'side': (3, 64), 'detail': (6, 24)},
    'RingTide': {'front': (7, 40), 'side': (7, 40), 'detail': (9.6, 17)},
    'RingEmperor': {'front': (7.5, 42), 'side': (7.5, 42), 'detail': (10.5, 18)},
}
DISTANCE = 420


def frame_piece(scene, name, view):
    target_z, scale = FRAMING[name][view]
    angle = math.radians(0 if view == 'front' else 72)
    scene.camera.location = (DISTANCE * math.sin(angle),
                             -DISTANCE * math.cos(angle), target_z + 12)
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
