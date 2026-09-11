"""Build original Celestial props, native-format assets and honest geometry previews.

Run: blender -b -P build_celestial_props.py [-- --no-render]
Outputs are staged in authored-props, never copied over a live client.
"""
import hashlib
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from celestial_geometry import collection, materials
from celestial_palette import construction_palette
from celestial_props import BUILDERS
from export_prop_bmd import export
from inspect_bmd_rig import inspect

OUTPUT = ROOT / 'authored-props'


def light(name, location, power, size, color):
    data = bpy.data.lights.new(name, 'AREA')
    data.energy, data.shape, data.size, data.color = power, 'DISK', size, color
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector((0, 0, 10)) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def setup_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 900, 1200
    scene.render.resolution_percentage = 100
    scene.world.color = (0.07, 0.07, 0.07)
    scene.view_settings.view_transform = 'AgX'
    light('Key softbox', (-100, -160, 170), 1200000, 130, (1, .87, .7))
    light('Cool fill', (110, -100, 60), 850000, 100, (.7, .83, 1))
    light('Golden rim', (0, 100, 150), 1800000, 80, (1, .72, .34))
    camera = bpy.data.cameras.new('Review camera')
    obj = bpy.data.objects.new(camera.name, camera)
    bpy.context.collection.objects.link(obj)
    scene.camera = obj
    camera.type = 'ORTHO'
    return scene


def frame_camera(scene, name, objects):
    points = [obj.matrix_world @ vertex.co for obj in objects for vertex in obj.data.vertices]
    lower = Vector(tuple(min(p[i] for p in points) for i in range(3)))
    upper = Vector(tuple(max(p[i] for p in points) for i in range(3)))
    center = (lower + upper) / 2
    direction = Vector((.27, -1, .15 if name != 'Ring' else .65)).normalized()
    scene.camera.location = center + direction * 300
    scene.camera.rotation_euler = (-direction).to_track_quat('-Z', 'Y').to_euler()
    dimensions = upper - lower
    aspect = scene.render.resolution_x / scene.render.resolution_y
    scene.camera.data.ortho_scale = max(dimensions.z, dimensions.x / aspect) * 1.17
    scene.camera.data.clip_end = 2000


def package_textures(reports):
    names = {texture for report in reports for texture in report['textures']}
    for name in sorted(names):
        source = ROOT / 'textures' / name
        raw = source.read_bytes()
        if not raw.startswith(b'\xff\xd8'):
            raise ValueError(f'Expected JPEG: {source}')
        (OUTPUT / 'Data' / 'Item' / Path(name).with_suffix('.OZJ')).write_bytes(bytes(24) + raw)


def verify_export(collection, name, path):
    model = inspect(path, full=True)
    if model['action_frames'] != [1]:
        raise ValueError(f'Unexpected rigid action in {path}')
    if any(mesh['texture'].startswith(('kundun', 'Shield', 'Ring', 'Necklace')) for mesh in model['meshes']):
        raise ValueError('Native appearance dependency')
    points = [point for mesh in model['meshes'] for _, point in mesh['points']]
    return dict(bounds=[(min(p[i] for p in points), max(p[i] for p in points)) for i in range(3)],
                sha256=model['sha256'], objects=len(collection.objects))


def main():
    (OUTPUT / 'Data' / 'Item').mkdir(parents=True, exist_ok=True)
    scene = setup_scene()
    palette = construction_palette(materials())
    groups, reports = {}, []
    for name, builder in BUILDERS.items():
        groups[name] = collection('Celestial ' + name, lambda b=builder: b(palette))
        bpy.context.view_layer.update()
        path = OUTPUT / 'Data' / 'Item' / f'Celestial_{name}.bmd'
        report = export(groups[name], name, path)
        report.update(verify_export(groups[name], name, path))
        reports.append(report)
        print('AUTHORED', json.dumps(report), flush=True)
    package_textures(reports)
    if '--no-render' not in sys.argv:
        for name, group in groups.items():
            for other_name, other in groups.items():
                other.hide_render = other_name != name
            frame_camera(scene, name, group.objects)
            scene.render.filepath = str(OUTPUT / f'Celestial_{name}_review.png')
            bpy.ops.render.render(write_still=True)
    for group in groups.values():
        group.hide_render = group.name != 'Celestial Staff'
        group.hide_viewport = group.name != 'Celestial Staff'
    frame_camera(scene, 'Staff', groups['Staff'].objects)
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / 'celestial-authored-props.blend'))
    receipt = dict(status='geometry review; no ingame verification; not deployed', items=reports)
    (OUTPUT / 'asset-report.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    manifest = {str(path.relative_to(OUTPUT)).replace('\\', '/'): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (OUTPUT / 'Data').rglob('*') if path.is_file()}
    (OUTPUT / 'sha256.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
