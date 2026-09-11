"""Studio lighting/camera only; these are not screenshots from MU Online."""
import math

import bpy
from mathutils import Vector


def point_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def studio():
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
            ('Key', (-95, -150, 125), 650000, (1, .88, .71), 110),
            ('Fill', (100, -80, 15), 360000, (.65, .8, 1), 100),
            ('Rim', (35, 75, 115), 900000, (1, .77, .4), 100)):
        data = bpy.data.lights.new('Poseidon ' + name, 'AREA')
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(obj)
        obj.location = location
        point_at(obj, (0, 0, 30))
    camera_data = bpy.data.cameras.new('Poseidon inspection camera')
    camera = bpy.data.objects.new(camera_data.name, camera_data)
    scene.collection.objects.link(camera)
    camera_data.type = 'ORTHO'
    camera_data.ortho_scale = 250
    scene.camera = camera


def render_weapon(collections, name, destination):
    scene = bpy.context.scene
    for key, collection in collections.items():
        collection.hide_render = key != name
    for view, degrees in (('front', 0), ('side', 72), ('back', 180)):
        angle = math.radians(degrees)
        scene.camera.location = (300 * math.sin(angle), -300 * math.cos(angle), 25)
        point_at(scene.camera, (0, 0, 25))
        scene.render.filepath = str(destination / f'Poseidon_{name}_{view}.png')
        bpy.ops.render.render(write_still=True)


def render_head(collections, name, destination):
    scene = bpy.context.scene
    for key, collection in collections.items():
        collection.hide_render = key != name
    scene.camera.data.ortho_scale = 104
    scene.camera.location = (52, -300, 102)
    point_at(scene.camera, (0, 0, 94 if name == 'Trident' else 80))
    scene.render.filepath = str(destination / f'Poseidon_{name}_detail.png')
    bpy.ops.render.render(write_still=True)
    scene.camera.data.ortho_scale = 250
