"""Verify the generated catalog's internal references and immutable input hash."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

from catalog_names import write_json


def verify(output):
    report = json.loads((output / 'catalog.json').read_text(encoding='utf-8'))
    families = json.loads((output / 'canonical-families.json').read_text(encoding='utf-8'))
    sources = {source['id']: source for source in report['sources']}
    canonical = {entry['path']: entry for entry in report['occurrences'] if entry['canonical']}
    assert sum(source['canonical'] for source in sources.values()) == 1
    assert sum(source['candidates'] for source in sources.values()) == len(report['occurrences'])
    for entry in report['occurrences']:
        assert entry['source_id'] in sources and entry['sha256'] in report['models']
        assert entry['mapping_ref'] in report['mappings']
        for binding in entry['textures']:
            if binding['status'] == 'ok':
                assert binding['sha256'] in report['textures']
    for family in families['families']:
        for pieces in family['pieces'].values():
            for piece in pieces:
                assert piece['path'] in canonical
                assert piece['item_number'] == family['item_number']
    for texture in report['textures'].values():
        if texture['status'] == 'ok':
            with Image.open(output / texture['thumbnail']) as image:
                image.verify()
            assert not texture['palette'] or abs(sum(color['share'] for color in texture['palette']) - 1) < .001
    with Path(report['canonical_archive']).open('rb') as archive:
        assert hashlib.file_digest(archive, 'sha256').hexdigest() == report['canonical_archive_sha256']
    assert hashlib.sha256(Path(report['parser']['path']).read_bytes()).hexdigest() == report['parser']['sha256']
    result = dict(passed=True, occurrences_checked=len(report['occurrences']), canonical_models=len(canonical),
                  families_checked=len(families['families']), thumbnails_checked=sum(t['status'] == 'ok' for t in report['textures'].values()),
                  canonical_archive_unchanged=True, shared_parser_unchanged=True, asset_writes=False)
    write_json(output / 'catalog-validation.json', result)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    verify(parser.parse_args().output)
