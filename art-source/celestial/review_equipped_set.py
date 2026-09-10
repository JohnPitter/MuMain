"""Review decoded equipment together using the client's current link transforms.

This is a Blender reconstruction of attachment math, not an ingame capture.
Run with -- <client Data/Player> to render idle, casting and flight snapshots.
"""
import hashlib
import json
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from armor_head_reference import native_material
from build_celestial_props import setup_scene
from celestial_geometry import materials
from import_native_reference import create_mesh, world_matrices
from inspect_bmd_rig import inspect

OUTPUT = ROOT / 'equipped-review'
ARMOR_NAMES = ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots')
CLIENT_SOURCE = ROOT.parents[1] / 'src/source/Engine/Object/ZzzCharacter.cpp'


def import_surface(model, transforms, palette):
    objects = []
    for index, source in enumerate(model['meshes']):
        obj = create_mesh(source, None, transforms, index)
        obj.name = model['name'] + f' / decoded mesh {index}'
        obj.data.materials.append(palette[source['texture']])
        objects.append(obj)
    return objects


def attachments(player):
    wing_offset = player[47].to_3x3() @ Vector((0, 0, 15))
    return dict(Staff=player[33], Shield=player[42],
                Wings=Matrix.Translation(wing_offset) @ player[47])


def load_models():
    paths = {name: ROOT / 'authored-armor/Data/Player' / f'Celestial_{name}.bmd' for name in ARMOR_NAMES}
    paths.update({name: ROOT / 'authored-props/Data/Item' / f'Celestial_{name}.bmd'
                  for name in ('Staff', 'Shield')})
    paths['Wings'] = ROOT / 'authored-wings/Data/Item/Celestial_Wings.bmd'
    return {name: inspect(path, True) for name, path in paths.items()}


def configure_scene(scene):
    target = Vector((0, 0, 135))
    scene.camera.location = (120, -630, 180)
    scene.camera.rotation_euler = (target - scene.camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera.data.ortho_scale = 450
    scene.camera.data.clip_end = 1600
    scene.render.resolution_x, scene.render.resolution_y = 1200, 1200
    scene.cycles.samples = 24
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()


def render_pose(models, pose, palette, head):
    scene = setup_scene()
    for name in ARMOR_NAMES:
        import_surface(models[name], pose, palette)
    import_surface(head, pose, palette)
    for name, parent in attachments(pose).items():
        model = models[name]
        transforms = [parent @ transform for transform in world_matrices(model, 0)]
        import_surface(model, transforms, palette)
    configure_scene(scene)
    return scene


def main():
    directory = Path(sys.argv[sys.argv.index('--') + 1])
    player = inspect(directory / 'player.bmd', True)
    head = inspect(directory / 'HelmClass201.bmd', True)
    models = load_models()
    palette = {material.name: material for material in materials().values()}
    for mesh in head['meshes']:
        palette[mesh['texture']] = native_material(directory, mesh['texture'])
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name, action, frame in (('idle', 4, 0), ('cast', 146, 2), ('flight', 34, 1)):
        scene = render_pose(models, world_matrices(player, frame, action), palette, head)
        scene.render.filepath = str(OUTPUT / f'Celestial_Equipped_{name}.png')
        bpy.ops.render.render(write_still=True)
        bpy.ops.file.pack_all()
        bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / f'celestial-equipped-{name}.blend'))
    report = dict(status='attachment-math preview, not ingame or visual acceptance',
                  models={name: model['sha256'] for name, model in models.items()},
                  attachment_source_sha256=hashlib.sha256(CLIENT_SOURCE.read_bytes()).hexdigest(),
                  player_sha256=player['sha256'], head_sha256=head['sha256'],
                  jewelry='ring and pendant are inventory assets; not rendered on the body by the current client')
    (OUTPUT / 'review-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
