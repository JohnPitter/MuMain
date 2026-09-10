"""Generate and stage all five armor meshes; never modify an installed game.

Run Blender -b -P build_celestial_armor.py -- <client Data/Player> [--no-render].
"""
import hashlib
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from armor_animation import action_indices, animate
from armor_head_reference import add_head
from armor_surfaces import add_skin
from build_celestial_props import frame_camera, setup_scene
from celestial_cuirass import armor, helmet
from celestial_geometry import collection, materials
from celestial_limb_armor import BUILDERS
from export_armor_bmd import export
from import_native_reference import create_rig, world_matrices
from inspect_bmd_rig import inspect

OUTPUT = ROOT / 'authored-armor'
ENUM_PATH = ROOT.parents[1] / 'src' / 'source' / 'Core' / 'Globals' / '_enum.h'


def package_textures(reports):
    for texture in {name for report in reports for name in report['textures']}:
        data = (ROOT / 'textures' / texture).read_bytes()
        (OUTPUT / 'Data' / 'Player' / Path(texture).with_suffix('.OZJ')).write_bytes(bytes(24) + data)


def configure_review(scene, objects):
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            obj.rotation_euler = (Vector((0, 0, 115)) - obj.location).to_track_quat('-Z', 'Y').to_euler()
    frame_camera(scene, 'Armor', objects)
    scene.camera.location = (80, -340, 148)
    scene.camera.rotation_euler = (Vector((0, 0, 110)) - scene.camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera.data.ortho_scale = 248
    scene.cycles.samples = 24
    scene.render.resolution_x, scene.render.resolution_y = 1000, 1280


def render_reviews(scene, clips):
    views = [('front', 1), ('idle', clips[0]['start']),
             ('walk', clips[1]['start'] + 8), ('cast', clips[3]['start'] + 8)]
    for name, frame in views:
        scene.frame_set(frame)
        scene.render.filepath = str(OUTPUT / f'Celestial_Armor_{name}.png')
        bpy.ops.render.render(write_still=True)
    scene.frame_set(1)


def write_receipt(reports, clips, rig_source, player_source):
    receipt = dict(status='first authored armor; visual revision pending; not deployed',
                   rig_sha256=rig_source['sha256'], player_sha256=player_source['sha256'],
                   pieces=reports, clips=clips)
    (OUTPUT / 'armor-report.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    manifest = {str(path.relative_to(OUTPUT)).replace('\\', '/'): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (OUTPUT / 'Data').rglob('*') if path.is_file()}
    (OUTPUT / 'sha256.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')


def main():
    args = sys.argv[sys.argv.index('--') + 1:]
    source = Path(args[0])
    (OUTPUT / 'Data' / 'Player').mkdir(parents=True, exist_ok=True)
    scene = setup_scene()
    palette = materials()
    model, player = inspect(source / 'ArmorMale74.bmd', True), inspect(source / 'player.bmd', True)
    bind = world_matrices(model, 0)
    rig = create_rig(model, bind)
    rig.name = 'CelestialBodyRig'
    groups, reports = {}, []
    for name, builder in dict(Helm=helmet, Armor=armor, **BUILDERS).items():
        group = collection('Celestial ' + name, lambda b=builder: b(palette))
        bpy.context.view_layer.update()
        report = export(group, bind, model, OUTPUT / 'Data' / 'Player' / f'Celestial_{name}.bmd')
        add_skin(group, rig)
        groups[name] = group
        reports.append(report)
        print('ARMOR_AUTHORED', json.dumps(report), flush=True)
    objects = [obj for group in groups.values() for obj in group.objects]
    objects.extend(add_head(source, rig, bind))
    clips = animate(rig, model, player, objects, action_indices(ENUM_PATH))
    package_textures(reports)
    write_receipt(reports, clips, model, player)
    configure_review(scene, objects)
    if '--no-render' not in args:
        render_reviews(scene, clips)
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / 'celestial-authored-armor.blend'))
    print('ARMOR_CLIPS_VERIFIED', json.dumps(clips), flush=True)


if __name__ == '__main__':
    main()
