"""Inventory MU equipment sources without changing or extracting game assets."""
import argparse
import hashlib
import importlib.util
import json
from collections import Counter
from pathlib import Path

from asset_source import discover, in_scope
from catalog_names import canonical_families, read_names, write_json
from catalog_report import texture_sheets, write_report
from detail_report import appearance_variants, write_details
from item_models import item_mapping, literal_item_loads
from mappings import enrich_mapping, filename_mapping, runtime_primary_paths
from memory_asset import MemoryAsset
from textures import elsewhere_candidates, resolve_texture, runtime_semantics, texture_record


def load_inspector(client_root):
    path = client_root / 'art-source/celestial/inspect_bmd_rig.py'
    spec = importlib.util.spec_from_file_location('inspect_bmd_rig', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.inspect, dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def texture_script(name):
    if '_' not in name:
        return {'enabled': False, 'flags': []}
    token = name.split('_', 1)[1].split('.', 1)[0][:4]
    flags = {'R': 'bright', 'H': 'hidden', 'S': 'stream', 'N': 'none_blend'}
    valid = bool(token) and all(letter in flags for letter in token)
    return {'enabled': valid, 'flags': [flags[letter] for letter in token] if valid else [],
            'panda_special_case_candidate': name == 'mu_rgb_lights.jpg'}


def summarize_model(raw, label, inspect):
    if raw[:3] != b'BMD' and Path(label).name.lower() in ('item.bmd', 'itemtest.bmd'):
        return dict(status='non_mesh', first_path=label, bytes=len(raw),
                    description='Legacy encrypted item table; .bmd extension does not identify a 3D mesh.')
    try:
        model = inspect(MemoryAsset(label, raw))
        actions = []
        for index, frames in enumerate(model['action_frames']):
            actions.append(dict(index=index, frames=frames, locked=model['action_locks'][index],
                moving_bones=[i for i, bone in enumerate(model['bones']) if not bone.get('dummy') and bone['clips'][index]['moves']],
                rotating_bones=[i for i, bone in enumerate(model['bones']) if not bone.get('dummy') and bone['clips'][index]['rotates']]))
        bones = [{key: value for key, value in bone.items() if key != 'clips'} for bone in model['bones']]
        for mesh in model['meshes']:
            mesh['texture_script'] = texture_script(mesh['texture'])
        return dict(status='ok', first_path=label, bytes=len(raw), version=raw[3], embedded_name=model['name'],
            meshes=model['meshes'], mesh_count=len(model['meshes']), bone_count=len(bones), bones=bones,
            vertices=sum(mesh['vertices'] for mesh in model['meshes']),
            polygon_records=sum(mesh['triangles'] for mesh in model['meshes']), actions=actions,
            animated_action_count=sum(bool(action['moving_bones'] or action['rotating_bones']) for action in actions),
            empty_geometry=not any(mesh['vertices'] for mesh in model['meshes']))
    except (ValueError, IndexError, OSError) as error:
        return dict(status='error', first_path=label, bytes=len(raw), version=raw[3] if len(raw) > 3 else None, error=str(error))


def model_mapping(relative, context):
    if relative.startswith('data/item/'):
        return item_mapping(relative, context['loads'], context['names'])
    result = enrich_mapping(filename_mapping(relative), context['names'])
    if result.get('variant') == 'primary' and relative not in context['primary_paths']:
        result['variant'] = 'present_but_not_primary_in_client_loader'
    if result['category'] == 'unmapped_equipment_candidate' and not result['part']:
        result = item_mapping(relative, context['loads'], context['names'])
        if result['category'] == 'unmapped_item_resource':
            result['category'] = 'player_auxiliary_resource'
    return result


def bind_textures(source, relative, model, context):
    result = []
    mapping = context['mappings'][relative]
    directories = mapping.get('texture_directories', [])
    model_texture_path = str(Path(directories[0]) / Path(relative).name).replace('\\', '/').lower() if len(directories) == 1 else relative
    for index, mesh in enumerate(model.get('meshes', [])):
        name = mesh['texture']
        path, resolution = resolve_texture(source, model_texture_path, name)
        if model_texture_path != relative:
            resolution = 'explicit_native_texture_directory' if path else 'unresolved_in_native_texture_directory'
        binding = dict(mesh_index=index, declared=name, resolution=resolution, status='missing')
        binding['runtime_semantics'] = runtime_semantics(name)
        if path is None:
            binding['elsewhere_same_source_candidates'] = elsewhere_candidates(source, name)
            if binding['runtime_semantics']['hidden_sentinel']:
                binding['status'] = 'not_required_hidden'
            result.append(binding)
            continue
        key = source.id, path
        if key not in context['texture_paths']:
            raw = source.read(path)
            digest = hashlib.sha256(raw).hexdigest()
            context['texture_paths'][key] = digest
            if digest not in context['textures']:
                try:
                    context['textures'][digest] = dict(status='ok', first_path=source.display(path), **texture_record(path, raw, context['output']))
                except (ValueError, OSError) as error:
                    context['textures'][digest] = dict(status='error', first_path=source.display(path), error=str(error))
        digest = context['texture_paths'][key]
        binding.update(path=path, sha256=digest, status=context['textures'][digest]['status'])
        result.append(binding)
    return result


def scan_source(source, context):
    records = []
    for relative in sorted(source.entries):
        if not in_scope(relative):
            continue
        raw = source.read(relative)
        digest = hashlib.sha256(raw).hexdigest()
        if digest not in context['models']:
            context['models'][digest] = summarize_model(raw, source.display(relative), context['inspect'])
        model = context['models'][digest]
        if relative not in context['mappings']:
            context['mappings'][relative] = model_mapping(relative, context)
        records.append(dict(source_id=source.id, canonical=source.canonical, path=relative,
            original_path=source.display(relative), sha256=digest, mapping_ref=relative,
            textures=bind_textures(source, relative, model, context)))
    return records


def summarize(report):
    occurrences = report['occurrences']
    canonical = [item for item in occurrences if item['canonical']]
    conflicts = {}
    for item in occurrences:
        conflicts.setdefault(item['path'], set()).add(item['sha256'])
    return dict(sources=len(report['sources']), bmd_occurrences=len(occurrences), unique_bmd_sha256=len(report['models']),
        duplicate_bmd_occurrences=len(occurrences) - len(report['models']), canonical_bmd=len(canonical),
        parsed_unique_bmd=sum(model['status'] == 'ok' for model in report['models'].values()),
        failed_unique_bmd=sum(model['status'] == 'error' for model in report['models'].values()),
        non_mesh_bmd_tables=sum(model['status'] == 'non_mesh' for model in report['models'].values()),
        unique_texture_sha256=len(report['textures']), failed_texture_decode=sum(texture['status'] != 'ok' for texture in report['textures'].values()),
        missing_texture_references=sum(binding['status'] == 'missing' for item in occurrences for binding in item['textures']),
        canonical_missing_texture_references=sum(binding['status'] == 'missing' for item in canonical for binding in item['textures']),
        conflicting_same_paths=sum(len(hashes) > 1 for hashes in conflicts.values()),
        appearance_variant_paths=len(report['appearance_variants']),
        canonical_categories=dict(Counter(report['mappings'][item['mapping_ref']]['category'] for item in canonical)))


def evidence_files(args):
    files = [args.client / relative for relative in (
        'src/source/Engine/Object/ZzzOpenData.cpp', 'src/source/Core/Globals/_enum.h',
        'src/source/Core/Globals/_define.h', 'src/source/Core/Globals/_crypt.h',
        'src/source/Data/GameData/ItemData/ItemFieldDefs.h', 'src/source/Data/GameData/ItemData/ItemStructs.h',
        'src/source/Data/DataHandler/ItemData/ItemDataLoader.cpp', 'src/source/Data/DataHandler/LoadData.cpp',
        'src/source/GameLogic/Social/MonkSystem.cpp', 'src/source/Render/Sprites/TextureScript.cpp')]
    files.extend(args.server / relative for relative in (
        'src/Persistence/Initialization/VersionSeasonSix/Items/Armors.cs',
        'src/Persistence/Initialization/Items/ArmorInitializerBase.cs',
        'src/DataModel/Configuration/Items/UltimateEquipment.cs'))
    return [dict(path=str(path), sha256=hashlib.sha256(path.read_bytes()).hexdigest()) for path in files]


def build(args):
    sources, audit = discover(args.roots, args.canonical)
    canonical = next(source for source in sources if source.canonical)
    names = read_names(canonical, args.client, args.server)
    families = canonical_families(canonical, names)
    write_json(args.output / 'canonical-families.json', families)
    write_json(args.output / 'item-names.json', names)
    inspect, parser_info = load_inspector(args.client)
    context = dict(models={}, textures={}, texture_paths={}, mappings={}, output=args.output,
                   loads=literal_item_loads(args.client), primary_paths=runtime_primary_paths(), names=names, inspect=inspect)
    report = dict(schema_version=1, canonical_archive=str(args.canonical), parser=parser_info, evidence_files=evidence_files(args), sources=[source.info() for source in sources],
                  discovery_audit=audit, occurrences=[], models=context['models'], textures=context['textures'], mappings=context['mappings'])
    with args.canonical.open('rb') as archive:
        report['canonical_archive_sha256'] = hashlib.file_digest(archive, 'sha256').hexdigest()
    try:
        for index, source in enumerate(sources, 1):
            print(f'[{index}/{len(sources)}] {source.id}: {source.info()["candidates"]} BMD', flush=True)
            report['occurrences'].extend(scan_source(source, context))
        report['appearance_variants'] = appearance_variants(report)
        report['summary'] = summarize(report)
        report['texture_sheets'] = texture_sheets(report, args.output)
        write_json(args.output / 'catalog.json', report)
        write_report(report, families, args.output)
        write_details(report, families, args.output)
        print(json.dumps(report['summary'], ensure_ascii=True, indent=2))
    finally:
        for source in sources:
            source.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--canonical', type=Path, required=True)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--server', type=Path, required=True)
    parser.add_argument('--roots', type=Path, nargs='+', required=True)
    parser.add_argument('--output', type=Path, required=True)
    build(parser.parse_args())
