"""Studio lighting and inspection cameras for the armor prototypes; not MU screenshots."""
import math

import bpy
from mathutils import Vector

VIEWS = {'front': 0, 'side': 72, 'back': 180}
FRAMING = {'Helm': (178, 80), 'Armor': (136, 132), 'Boot': (30, 96)}


def point_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def studio(target=110):
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
            ('Key', (-95, -150, target + 15), 650000, (1, .88, .71), 110),
            ('Fill', (100, -80, target - 95), 360000, (.65, .8, 1), 100),
            ('Rim', (35, 75, target + 5), 900000, (1, .77, .4), 100)):
        data = bpy.data.lights.new('Poseidon ' + name, 'AREA')
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(obj)
        obj.location = location
        point_at(obj, (0, 0, target))
    camera_data = bpy.data.cameras.new('Poseidon armor inspection camera')
    camera = bpy.data.objects.new(camera_data.name, camera_data)
    scene.collection.objects.link(camera)
    camera_data.type = 'ORTHO'
    scene.camera = camera


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
