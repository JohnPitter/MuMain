"""Build canonical item identities and family links before the heavier mesh scan."""
import hashlib
import argparse
import json
import re
from pathlib import Path

from mappings import PARTS, client_names, enrich_mapping, filename_mapping, is_candidate, runtime_primary_paths, server_names


def read_names(source, client_root, server_root):
    result = {'server': server_names(server_root), 'client': {}, 'client_sources': [], 'errors': []}
    for relative in sorted(source.entries):
        if not relative.startswith('data/local/') or not relative.endswith('/item.bmd'):
            continue
        locale = Path(relative).parent.name
        locale = 'default' if locale == 'local' else locale.title()
        try:
            raw = source.read(relative)
            result['client'][locale] = client_names(raw, client_root)
            result['client_sources'].append(dict(path=source.display(relative), locale=locale,
                                                  sha256=hashlib.sha256(raw).hexdigest()))
        except (ValueError, StopIteration) as error:
            result['errors'].append(dict(path=relative, error=str(error)))
    return result


def canonical_families(source, names):
    families, auxiliary = {}, []
    loaded = runtime_primary_paths()
    for relative in sorted(source.entries):
        if not relative.startswith('data/player/') or not is_candidate(relative):
            continue
        mapping = enrich_mapping(filename_mapping(relative), names)
        if mapping.get('variant') == 'primary' and relative not in loaded:
            mapping['variant'] = 'present_but_not_primary_in_client_loader'
        if mapping['category'] != 'equipment':
            auxiliary.append(dict(path=relative, **mapping))
            continue
        number = mapping['item_number']
        family = families.setdefault(number, {'item_number': number, 'pieces': {}, 'variants': []})
        entry = {'path': relative, 'archive_entry': source.entries[relative], **mapping}
        if mapping['variant'] == 'primary':
            family['pieces'].setdefault(mapping['part'], []).append(entry)
        else:
            family['variants'].append(entry)
    for family in families.values():
        pieces = [piece for group in family['pieces'].values() for piece in group]
        first = next((piece for piece in pieces if piece['part'] == 'Armor'), pieces[0] if pieces else {})
        family['name'] = re.sub(r' (Helm(?:et)?|Mask|Armor|Pants|Gloves|Boots)$', '', first.get('name', 'Name unresolved'))
        family['name_reliability'] = first.get('name_reliability', 'unresolved_or_lossy_decode')
        family['present_parts'] = sorted(family['pieces'], key=lambda part: PARTS[part])
        family['missing_parts'] = [part for part in PARTS if part not in family['pieces']]
        family['server_defined_parts'] = [part for part in PARTS if f"{PARTS[part]}:{family['item_number']}" in names['server']]
        family['expected_missing_parts'] = [part for part in family['server_defined_parts'] if part not in family['pieces']]
    return dict(source=source.info(), families=list(sorted(families.values(), key=lambda family: family['item_number'])),
                auxiliary=auxiliary, warnings=names['errors'])


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    from asset_source import archive_sources

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--canonical', type=Path, required=True)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--server', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    arguments = parser.parse_args()
    canonical = archive_sources(arguments.canonical, arguments.canonical)[0]
    try:
        names = read_names(canonical, arguments.client, arguments.server)
        write_json(arguments.output / 'canonical-families.json', canonical_families(canonical, names))
        write_json(arguments.output / 'item-names.json', names)
    finally:
        canonical.close()
