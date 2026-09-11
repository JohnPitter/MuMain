"""Render decoded armor without wings concealing the rear surfaces; no ingame claim."""
import json
from pathlib import Path
import sys

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from armor_head_reference import HEAD_MODELS, load_head, native_material
from build_celestial_props import setup_scene
from celestial_geometry import materials
from import_native_reference import world_matrices
from inspect_bmd_rig import inspect
from review_equipped_set import ARMOR_NAMES, import_surface

OUTPUT = ROOT / 'armor-details'
VIEWS = (('front', False, (.12, -1, .04), 'soul-master'),
         ('rear', False, (-.12, 1, .04), 'soul-master'),
         ('side', False, (1, .2, .04), 'soul-master'),
         ('helm-front', True, (.12, -1, .1), 'soul-master'),
         ('helm-rear', True, (-.12, 1, .1), 'soul-master'),
         ('grand-master-front', False, (.12, -1, .04), 'grand-master'),
         ('grand-master-helm', True, (.12, -1, .1), 'grand-master'))


def frame(scene, objects, direction):
    points = [obj.matrix_world @ vertex.co for obj in objects for vertex in obj.data.vertices]
    lower = Vector(tuple(min(p[index] for p in points) for index in range(3)))
    upper = Vector(tuple(max(p[index] for p in points) for index in range(3)))
    center, dimensions = (lower + upper) / 2, upper - lower
    scene.camera.location = center + Vector(direction).normalized() * 360
    scene.camera.rotation_euler = (center - scene.camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera.data.ortho_scale = max(dimensions.z, dimensions.x / .75, dimensions.y / .75) * 1.18
    scene.camera.data.clip_end = 1200
    scene.render.resolution_x, scene.render.resolution_y = 900, 1200
    scene.cycles.samples = 20
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            obj.rotation_euler = (center - obj.location).to_track_quat('-Z', 'Y').to_euler()


def main():
    directory = Path(sys.argv[sys.argv.index('--') + 1])
    player = inspect(directory / 'player.bmd', True)
    heads = {name: load_head(directory, name) for name in HEAD_MODELS}
    models = {name: inspect(ROOT / 'authored-armor/Data/Player' / f'Celestial_{name}.bmd', True)
              for name in ARMOR_NAMES}
    palette = {material.name: material for material in materials().values()}
    for head in heads.values():
        for surface in head['meshes']:
            palette[surface['texture']] = native_material(directory, surface['texture'])
    pose = world_matrices(player, 0, 4)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name, helmet_only, direction, character_class in VIEWS:
        scene = setup_scene()
        objects = import_surface(heads[character_class], pose, palette)
        for part in ('Helm',) if helmet_only else ARMOR_NAMES:
            objects.extend(import_surface(models[part], pose, palette))
        frame(scene, objects, direction)
        scene.render.filepath = str(OUTPUT / f'Celestial_Armor_{name}.png')
        bpy.ops.render.render(write_still=True)
    report = dict(status='decoded armor review; not ingame or artistic acceptance',
                  models={name: model['sha256'] for name, model in models.items()},
                  player_sha256=player['sha256'], action=4, frame=0,
                  heads={name: dict(file=HEAD_MODELS[name], sha256=head['sha256']) for name, head in heads.items()},
                  views=[dict(name=name, character_class=character_class) for name, _, _, character_class in VIEWS])
    (OUTPUT / 'review-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
