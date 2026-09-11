"""Studio lighting/camera only; these are not screenshots from MU Online."""
import math

import bpy
from mathutils import Vector

FRAMING = {'Sword': dict(center=32, scale=280, detail=(52, 70), detail_target=(0, 0, 8)),
           'Staff': dict(center=47, scale=310, detail=(120, 120), detail_target=(0, 0, 148))}


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
    background.inputs[0].default_value = (.05, .06, .09, 1)
    background.inputs[1].default_value = .5
    for name, location, energy, color, size in (
            ('Key', (-95, -150, 125), 650000, (1, .9, .78), 110),
            ('Fill', (100, -80, 15), 360000, (.55, .75, 1), 100),
            ('Rim', (35, 75, 115), 900000, (.8, .88, 1), 100)):
        data = bpy.data.lights.new('Zeus ' + name, 'AREA')
        data.energy, data.color, data.size = energy, color, size
        obj = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(obj)
        obj.location = location
        point_at(obj, (0, 0, 30))
    camera_data = bpy.data.cameras.new('Zeus inspection camera')
    camera = bpy.data.objects.new(camera_data.name, camera_data)
    scene.collection.objects.link(camera)
    camera_data.type = 'ORTHO'
    camera_data.ortho_scale = 280
    scene.camera = camera


def render_weapon(collections, name, destination):
    scene = bpy.context.scene
    for key, collection in collections.items():
        collection.hide_render = key != name
    framing = FRAMING[name]
    for view, degrees in (('front', 0), ('side', 72)):
        angle = math.radians(degrees)
        scene.camera.location = (300 * math.sin(angle), -300 * math.cos(angle), framing['center'])
        point_at(scene.camera, (0, 0, framing['center']))
        scene.render.filepath = str(destination / f'Zeus_{name}_{view}.png')
        bpy.ops.render.render(write_still=True)


def render_head(collections, name, destination):
    scene = bpy.context.scene
    for key, collection in collections.items():
        collection.hide_render = key != name
    framing = FRAMING[name]
    scene.camera.data.ortho_scale = framing['detail'][0]
    scene.camera.location = (framing['detail'][0] * .52, -300, framing['detail_target'][2] + framing['detail'][1] * .3)
    point_at(scene.camera, framing['detail_target'])
    scene.render.filepath = str(destination / f'Zeus_{name}_detail.png')
    bpy.ops.render.render(write_still=True)
    scene.camera.data.ortho_scale = FRAMING['Sword']['scale']
