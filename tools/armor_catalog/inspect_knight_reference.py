"""Measure native Iron Knight references without editing archive contents."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from zipfile import ZipFile

from PIL import ImageDraw

from memory_asset import MemoryAsset
from textures import open_texture, palette

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'art-source/celestial'))
from inspect_bmd_rig import inspect

KNIGHTS = ('data/monster/monster150.bmd', 'data/monster/monster209.bmd')
MASKS = ('data/monster/icenightlight.ozj', 'data/monster/ex01icenightlight.ozj')
GEOMETRY_FIELDS = ('points', 'faces', 'normal_vectors', 'normal_indices', 'texcoords')


def digest(value):
    return hashlib.sha256(value).hexdigest()


def file_digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def structural_digest(value):
    return digest(json.dumps(value, separators=(',', ':')).encode('utf-8'))


def mesh_record(mesh):
    normal_sets = {index: set() for index in range(mesh['vertices'])}
    for face, indices in zip(mesh['faces'], mesh['normal_indices']):
        for vertex, normal in zip(face[0], indices):
            normal_sets[vertex].add(normal)
    return dict(vertices=mesh['vertices'], polygon_records=mesh['triangles'],
                triangles=sum(len(face[0]) - 2 for face in mesh['faces']),
                normals=len(mesh['normal_vectors']), texture=mesh['texture'],
                used_bones=mesh['used_bones'], uv_coordinates=len(mesh['texcoords']),
                vertices_with_multiple_normal_indices=sum(len(v) > 1 for v in normal_sets.values()),
                geometry_sha256=structural_digest({key: mesh[key] for key in GEOMETRY_FIELDS}))


def model_record(entry, model):
    meshes = [mesh_record(mesh) for mesh in model['meshes']]
    return dict(entry=entry, sha256=model['sha256'], meshes=meshes,
                vertices=sum(mesh['vertices'] for mesh in meshes),
                triangles=sum(mesh['triangles'] for mesh in meshes),
                bone_count=len(model['bones']),
                real_bone_count=sum(not bone.get('dummy') for bone in model['bones']),
                action_frames=model['action_frames'], action_locks=model['action_locks'],
                animated_bones=sum(any(c['moves'] or c['rotates'] for c in b.get('clips', []))
                                   for b in model['bones']),
                rig_sha256=structural_digest(model['bones']),
                geometry_sha256=structural_digest([m['geometry_sha256'] for m in meshes]))


def texture_stats(image):
    gray = image.convert('L')
    values = sorted(gray.get_flattened_data() if hasattr(gray, 'get_flattened_data') else gray.getdata())
    return dict(luminance_p05=values[int((len(values) - 1) * .05)],
                luminance_p50=values[int((len(values) - 1) * .50)],
                luminance_p95=values[int((len(values) - 1) * .95)],
                note='Valores de cinza PIL por pixel do atlas inteiro; não medem luz do renderer nem área visível por UV.')


def save_texture(entry, raw, output):
    image = open_texture(entry, raw)
    destination = output / (Path(entry).stem + '.png')
    image.save(destination)
    return dict(entry=entry, sha256=digest(raw), width=image.width, height=image.height,
                preview=destination.name, palette=palette(image), statistics=texture_stats(image))


def overlay_uv(image, mesh, output):
    drawing = ImageDraw.Draw(image)
    for _, indices in mesh['faces']:
        polygon = [(mesh['texcoords'][i][0] * image.width,
                    mesh['texcoords'][i][1] * image.height) for i in indices]
        drawing.line(polygon + polygon[:1], fill=(255, 70, 120, 170), width=1)
    image.save(output)


def collect_texture_entries(models, entries):
    found = set(MASKS)
    for name, model in models.items():
        for mesh in model['meshes']:
            suffix = '.ozt' if mesh['texture'].lower().endswith('.tga') else '.ozj'
            candidate = str(Path(name).parent / Path(mesh['texture']).with_suffix(suffix)).replace('\\', '/').lower()
            if candidate not in entries:
                raise ValueError(f'Textura ausente: {candidate}')
            found.add(candidate)
    return sorted(found)


def inspect_archive(archive_path, output):
    with ZipFile(archive_path) as archive:
        entries = {name.replace('\\', '/').lower(): name for name in archive.namelist()}
        names = list(KNIGHTS) + sorted(name for name in entries if name.endswith('.bmd')
                                     and name.startswith(('data/player/celestial_', 'data/item/celestial_')))
        models = {name: inspect(MemoryAsset(entries[name], archive.read(entries[name])), True) for name in names}
        records = [model_record(entries[name], model) for name, model in models.items()]
        textures = [save_texture(entries[name], archive.read(entries[name]), output)
                    for name in collect_texture_entries(models, entries)]
        for name in KNIGHTS:
            for index, mesh in enumerate(models[name]['meshes']):
                texture = f"data/monster/{Path(mesh['texture']).stem}.ozj".lower()
                image = open_texture(texture, archive.read(entries[texture]))
                overlay_uv(image, mesh, output / f'{Path(name).stem}-mesh{index}-uv.png')
        equality = [{key: left[key] == right[key] for key in GEOMETRY_FIELDS}
                    for left, right in zip(models[KNIGHTS[0]]['meshes'], models[KNIGHTS[1]]['meshes'])]
    return records, textures, equality


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    original_hash = file_digest(args.archive)
    records, textures, equality = inspect_archive(args.archive, args.output)
    celestial = [record for record in records if 'Celestial_' in record['entry']]
    native = records[0]['triangles']
    report = dict(archive=str(args.archive), archive_sha256=original_hash, models=records,
                  textures=textures, comparison=dict(native_triangles=native,
                  celestial_ten_files_triangles=sum(r['triangles'] for r in celestial),
                  celestial_armor_triangles=sum(r['triangles'] for r in celestial if '/Player/' in r['entry']),
                  native_geometry_identical=records[0]['geometry_sha256'] == records[1]['geometry_sha256'],
                  native_rig_identical=records[0]['rig_sha256'] == records[1]['rig_sha256'],
                  native_mesh_field_equality=equality),
                  validation=dict(archive_unchanged=original_hash == file_digest(args.archive),
                                  source_assets_written=False, textures_decoded=len(textures)))
    if not report['validation']['archive_unchanged']:
        raise ValueError('Arquivo de origem mudou durante a inspeção')
    (args.output / 'measurements.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(dict(models=len(records), comparison=report['comparison'], validation=report['validation']), indent=2))


if __name__ == '__main__':
    main()
