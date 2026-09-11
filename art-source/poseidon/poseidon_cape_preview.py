"""Studio lighting and inspection cameras for the cape prototype; not MU screenshots."""
import math

import bpy
from mathutils import Vector

VIEWS = {'front': 0, 'side': 72, 'back': 180}
CAPE_TARGET_Z = 122
CAPE_ORTHO_SCALE = 320


def point_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def studio(target=CAPE_TARGET_Z):
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.resolution_x = 900
    scene.render.resolution_y = 1280
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = 'AgX'
    scene.world.use_nodes = True
    background = next(node for node in scene.world.node_tree.nodes if node.type == 'BACKGROUND')
    background.inputs[0].default_value = (.055, .065, .085, 1)
    background.inputs[1].default_value = .5
    for name, location, energy, color, size in (
            ('Key', (-95, -150, target + 25), 700000, (1, .88, .71), 120),
            ('Fill', (100, -80, target - 90), 380000, (.65, .8, 1), 110),
            ('Rim', (35, 95, target + 15), 950000, (1, .77, .4), 110)):
        data = bpy.data.lights.new('Poseidon ' + name, 'AREA')
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(obj)
        obj.location = location
        point_at(obj, (0, 0, target))
    camera_data = bpy.data.cameras.new('Poseidon cape inspection camera')
    camera = bpy.data.objects.new(camera_data.name, camera_data)
    scene.collection.objects.link(camera)
    camera_data.type = 'ORTHO'
    scene.camera = camera


def frame_cape(scene, view):
    angle = math.radians(VIEWS[view])
    scene.camera.location = (420 * math.sin(angle), -420 * math.cos(angle), CAPE_TARGET_Z)
    point_at(scene.camera, (0, 0, CAPE_TARGET_Z))
    scene.camera.data.ortho_scale = CAPE_ORTHO_SCALE
    scene.camera.data.clip_end = 2000


def render_cape(scene, destination):
    for view in VIEWS:
        frame_cape(scene, view)
        scene.render.filepath = str(destination / f'Poseidon_Cape_{view}.png')
        bpy.ops.render.render(write_still=True)
