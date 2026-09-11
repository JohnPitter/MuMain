"""Render canonical armor BMDs for visual classification, never launch the game."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import zipfile

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'art-source/celestial'))
from import_native_reference import create_mesh, world_matrices
from inspect_bmd_rig import inspect

OUTPUT = ROOT / 'docs/art/armor-reference'
REPORT_BATCH_SIZE = 10
RENDER_SEMANTICS_VERSION = 2
LOADER_SOURCE = 'src/source/Data/DataHandler/LoadData.cpp:72-116'
HIDDEN_RENDER_SOURCE = 'src/source/Render/Models/ZzzBMD.cpp:1317'
VERIFIED_TEXTURE_FALLBACKS = {
    ('data/player/luckyitem/70', 'head helmet luck 40.jpg'):
        'data/player/luckyitem/65/head helmet luck 40.ozj',
}


class ArchiveAssets:
    def __init__(self, archive, temporary):
        self.archive = archive
        self.directory = Path(temporary)
        self.entries = {e.filename.replace('\\', '/').lower(): e for e in archive.infolist()}
        self.materials = {}
        self.reset_audit()

    def reset_audit(self):
        self.missing = set()
        self.fallbacks = {}

    def model(self, entry):
        raw = self.archive.read(entry)
        path = self.directory / (hashlib.sha256(raw).hexdigest() + '.bmd')
        path.write_bytes(raw)
        return inspect(path, True)

    def resolve_texture(self, filename, directory):
        stem = directory.lower() + '/' + filename.replace('\\', '/').lower()
        suffix = Path(stem).suffix.lower()
        encoded = '.ozj' if suffix == '.jpg' else '.ozt' if suffix == '.tga' else suffix
        key = str(Path(stem).with_suffix(encoded)).replace('\\', '/')
        entry = self.entries.get(key) or self.entries.get(stem)
        if entry is not None:
            return entry
        fallback = VERIFIED_TEXTURE_FALLBACKS.get((directory.lower(), filename.lower()))
        entry = self.entries.get(fallback) if fallback else None
        if entry is None:
            self.missing.add(filename)
            return None
        self.fallbacks[stem] = dict(requested=stem, archive_entry=entry.filename,
                                   sha256=hashlib.sha256(self.archive.read(entry)).hexdigest(),
                                   resolution='verified_preloaded_name_fallback', source=LOADER_SOURCE)
        return entry

    def texture(self, filename, directory):
        entry = self.resolve_texture(filename, directory)
        if entry is None:
            return None
        suffix = Path(filename).suffix.lower()
        raw = self.archive.read(entry)
        if entry.filename.lower().endswith('.ozj'):
            raw = raw[24:]
        elif entry.filename.lower().endswith('.ozt'):
            raw = raw[4:]
        path = self.directory / (hashlib.sha256(raw).hexdigest() + suffix)
        path.write_bytes(raw)
        return bpy.data.images.load(str(path), check_existing=True)

    def material(self, filename, directory):
        key = (directory, filename)
        if key in self.materials:
            return self.materials[key]
        material = bpy.data.materials.new(filename)
        material.use_nodes = True
        shader = next(node for node in material.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
        shader.inputs['Roughness'].default_value = .75
        image = self.texture(filename, directory)
        if image is None:
            shader.inputs['Base Color'].default_value = (.8, .02, .8, 1)
        else:
            node = material.node_tree.nodes.new('ShaderNodeTexImage')
            node.image = image
            material.node_tree.links.new(node.outputs['Color'], shader.inputs['Base Color'])
            material.node_tree.links.new(node.outputs['Alpha'], shader.inputs['Alpha'])
        self.materials[key] = material
        return material


def setup_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 8
    scene.cycles.use_denoising = True
    scene.cycles.max_bounces = 3
    scene.render.resolution_x, scene.render.resolution_y = 320, 384
    scene.render.resolution_percentage = 100
    if scene.world is None:
        scene.world = bpy.data.worlds.new('Catalog world')
    scene.world.color = (.18, .18, .18)
    scene.render.image_settings.file_format = 'PNG'
    scene.view_settings.view_transform = 'Standard'
    for name, location in (('Front', (100, -280, 240)), ('Rear', (-100, 280, 180))):
        light = bpy.data.lights.new(name, 'AREA')
        light.energy, light.size = 500000, 230
        obj = bpy.data.objects.new(name, light)
        scene.collection.objects.link(obj)
        obj.location = location
        obj.rotation_euler = (Vector((0, 0, 100)) - obj.location).to_track_quat('-Z', 'Y').to_euler()
    camera = bpy.data.cameras.new('Catalog camera')
    camera.type = 'ORTHO'
    camera.clip_end = 2000
    scene.camera = bpy.data.objects.new('Catalog camera', camera)
    scene.collection.objects.link(scene.camera)
    return scene


def add_family(family, assets):
    objects, reports, hidden = [], [], []
    for part, entries in family['pieces'].items():
        for entry in entries:
            model = assets.model(entry['archive_entry'])
            bind = world_matrices(model, 0)
            for index, mesh in enumerate(model['meshes']):
                if mesh['texture'].startswith('hid'):
                    hidden.append(dict(archive_entry=entry['archive_entry'], mesh=index,
                                       texture=mesh['texture'], source=HIDDEN_RENDER_SOURCE))
                    continue
                obj = create_mesh(mesh, None, bind, index)
                directory = str(Path(entry['archive_entry']).parent).replace('\\', '/')
                obj.data.materials.append(assets.material(mesh['texture'], directory))
                objects.append(obj)
            reports.append(dict(part=part, archive_entry=entry['archive_entry'], sha256=model['sha256']))
    return objects, reports, hidden


def frame_family(scene, objects, view):
    points = [obj.matrix_world @ v.co for obj in objects for v in obj.data.vertices]
    lower = Vector(tuple(min(p[i] for p in points) for i in range(3)))
    upper = Vector(tuple(max(p[i] for p in points) for i in range(3)))
    center = (lower + upper) / 2
    direction = Vector((.12, -1, .08)) if view == 'front' else Vector((-.12, 1, .08))
    scene.camera.location = center + direction * 500
    scene.camera.rotation_euler = (-direction).to_track_quat('-Z', 'Y').to_euler()
    scene.camera.data.ortho_scale = max(upper.z - lower.z, (upper.x - lower.x) * 384 / 320) * 1.15


def render_family(scene, family, assets, output):
    assets.reset_audit()
    result = dict(item_number=family['item_number'], views=[],
                  render_semantics_version=RENDER_SEMANTICS_VERSION)
    try:
        objects, result['models'], result['hidden_meshes_skipped'] = add_family(family, assets)
        for view in ('front', 'rear'):
            frame_family(scene, objects, view)
            path = output / f"family-{family['item_number']:03d}-{view}.png"
            scene.render.filepath = str(path)
            bpy.ops.render.render(write_still=True)
            result['views'].append(path.name)
    except Exception as error:
        result['error'] = str(error)
    finally:
        for obj in [o for o in bpy.data.objects if o.type == 'MESH']:
            bpy.data.objects.remove(obj, do_unlink=True)
        for mesh in [m for m in bpy.data.meshes if m.users == 0]:
            bpy.data.meshes.remove(mesh)
    result['missing_textures'] = sorted(assets.missing)
    result['texture_fallbacks'] = list(assets.fallbacks.values())
    print('CATALOG_RENDER', json.dumps(result), flush=True)
    return result


def audit_existing_dependencies(record, assets):
    assets.reset_audit()
    for entry in record['models']:
        model = assets.model(entry['archive_entry'])
        if model['sha256'] != entry['sha256']:
            raise ValueError('Existing render model differs from the selected source')
        directory = str(Path(entry['archive_entry']).parent).replace('\\', '/')
        for mesh in model['meshes']:
            if not mesh['texture'].startswith('hid'):
                assets.resolve_texture(mesh['texture'], directory)
    record['missing_textures'] = sorted(assets.missing)
    record['texture_fallbacks'] = list(assets.fallbacks.values())
    record['dependency_audit'] = 'Source resolution audit only; existing image was not rerendered.'


def store_result(catalog, index, result, assets):
    start = index // REPORT_BATCH_SIZE * REPORT_BATCH_SIZE
    path = OUTPUT / f'native-render-report-{start:03d}.json'
    report = json.loads(path.read_text()) if path.exists() else dict(families=[])
    if report.get('source', catalog['source'])['id'] != catalog['source']['id']:
        raise ValueError('Cannot merge render reports from different sources')
    records = {record['item_number']: record for record in report['families']}
    records[result['item_number']] = result
    allowed = {family['item_number'] for family in catalog['families'][start:start + REPORT_BATCH_SIZE]}
    if not set(records).issubset(allowed):
        raise ValueError('Existing report contains families outside its canonical batch')
    for record in records.values():
        if 'missing_textures' not in record:
            audit_existing_dependencies(record, assets)
    report.update(source=catalog['source'], families=[records[key] for key in sorted(records)],
                  missing_textures=sorted({name for record in records.values() for name in record['missing_textures']}),
                  validation='Native BMD geometry and textures in bind pose; diagnostic Blender lighting, not ingame.')
    path.write_text(json.dumps(report, indent=2), encoding='utf-8')


def selected_families(catalog, args):
    indexed = list(enumerate(catalog['families']))
    if args.families is None:
        return indexed[args.start:][:args.limit]
    wanted = set(args.families)
    if args.start or args.limit is not None:
        raise ValueError('Use --families separately from --start/--limit')
    if not wanted.issubset({family['item_number'] for _, family in indexed}):
        raise ValueError('Requested family is not in the canonical catalog')
    return [(index, family) for index, family in indexed if family['item_number'] in wanted]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--catalog', type=Path, default=OUTPUT / 'canonical-families.json')
    parser.add_argument('--limit', type=int)
    parser.add_argument('--start', type=int, default=0)
    parser.add_argument('--families', type=int, nargs='+')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    catalog = json.loads(args.catalog.read_text(encoding='utf-8-sig'))
    output = OUTPUT / 'native-renders'
    output.mkdir(parents=True, exist_ok=True)
    results = []
    with zipfile.ZipFile(catalog['source']['location']) as archive, TemporaryDirectory(prefix='mu-armor-render-') as temp:
        assets = ArchiveAssets(archive, temp)
        for index, family in selected_families(catalog, args):
            bpy.ops.wm.read_factory_settings(use_empty=True)
            assets.materials.clear()
            scene = setup_scene()
            results.append(render_family(scene, family, assets, output))
            store_result(catalog, index, results[-1], assets)
        if any('error' in result for result in results):
            raise RuntimeError('Some families could not be rendered; see native-render-report.json')


if __name__ == '__main__':
    main()
