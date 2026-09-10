"""Stage original wings with a lossless single-influence BMD animation."""
import hashlib
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from armor_surfaces import add_skin
from build_celestial_props import setup_scene
from celestial_geometry import collection, materials
from celestial_wing_geometry import wings
from export_skinned_bmd import ExportSpec, export
from import_native_reference import create_rig, world_matrices
from inspect_bmd_rig import inspect
from wing_animation import animate, with_halo_anchor

OUTPUT = ROOT / 'authored-wings'


def configure_review(scene):
    scene.camera.location = (0, -600, 78)
    scene.camera.rotation_euler = (Vector((0, 0, 55)) - scene.camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera.data.ortho_scale = 450
    scene.camera.data.clip_end = 1500
    scene.render.resolution_x, scene.render.resolution_y = 1200, 1000
    scene.cycles.samples = 24
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            obj.rotation_euler = (Vector((0, 0, 55)) - obj.location).to_track_quat('-Z', 'Y').to_euler()


def package(report, motion):
    for texture in report['textures']:
        data = (ROOT / 'textures' / texture).read_bytes()
        (OUTPUT / 'Data' / 'Item' / Path(texture).with_suffix('.OZJ')).write_bytes(bytes(24) + data)
    manifest = {str(p.relative_to(OUTPUT)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in (OUTPUT / 'Data').rglob('*') if p.is_file()}
    (OUTPUT / 'sha256.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    (OUTPUT / 'wing-report.json').write_text(json.dumps(dict(model=report, motion=motion,
        status='authored wing candidate; fidelity and ingame validation pending'), indent=2), encoding='utf-8')


def main():
    args = sys.argv[sys.argv.index('--') + 1:]
    native = inspect(Path(args[0]) / 'Wing44.bmd', True)
    model = with_halo_anchor(native)
    bind = world_matrices(model, 0)
    (OUTPUT / 'Data' / 'Item').mkdir(parents=True, exist_ok=True)
    scene = setup_scene()
    palette = materials()
    rig = create_rig(model, bind)
    rig.name = 'CelestialWingRig'
    group = collection('Celestial Wings', lambda: wings(palette, bind))
    bpy.context.view_layer.update()
    spec = ExportSpec(model, OUTPUT / 'Data' / 'Item' / 'Celestial_Wings.bmd', model['action_frames'][0])
    report = export(group, bind, spec)
    report.update(native_rig_sha256=native['sha256'], animation_frames=spec.frames)
    add_skin(group, rig)
    motion = animate(rig, model, list(group.objects))
    package(report, motion)
    configure_review(scene)
    if '--no-render' not in args:
        for frame in (1, 17, 33):
            scene.frame_set(frame)
            scene.render.filepath = str(OUTPUT / f'Celestial_Wings_{frame:02d}.png')
            bpy.ops.render.render(write_still=True)
    scene.frame_set(1)
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / 'celestial-authored-wings.blend'))
    print('WINGS_AUTHORED', json.dumps(dict(model=report, motion=motion)), flush=True)


if __name__ == '__main__':
    main()
