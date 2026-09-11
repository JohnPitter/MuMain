"""Validate cross-file documentation, render coverage and indexed source hashes."""
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / 'docs/art/armor-reference'


def read_json(name):
    return json.loads((DOCS / name).read_text(encoding='utf-8-sig'))


def check_links():
    checked = 0
    for document in DOCS.glob('*.md'):
        for target in re.findall(r'\]\(([^)]+)\)', document.read_text(encoding='utf-8-sig')):
            target = unquote(target.strip('<>')).split('#', 1)[0]
            if not target or target.startswith(('https://', 'http://')):
                continue
            target = re.sub(r':\d+(?:-\d+)?$', '', target)
            path = Path(target)
            if not path.is_absolute():
                path = document.parent / path
            if not path.exists():
                raise ValueError(f'Broken link in {document.name}: {target}')
            checked += 1
    return checked


def check_sources(index):
    for source in index['files']:
        path = ROOT / source['path']
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != source['sha256']:
            raise ValueError(f'Indexed source changed: {source["path"]}')
    for field in ('byModelSymbol', 'byResourceSymbol'):
        for entries in index[field].values():
            for file_number, event_number in entries:
                index['files'][file_number]['events'][event_number]


def check_previews(families, sheets):
    if sheets['missing_textures'] or sheets['families'] != len(families):
        raise ValueError('Render coverage incomplete')
    paths = [DOCS / 'native-sheets' / name for name in sheets['sheets']]
    for family in families:
        number = family['item_number']
        paths.extend(DOCS / 'native-renders' / f'family-{number:03d}-{view}.png'
                     for view in ('front', 'rear'))
    for path in paths:
        with Image.open(path) as preview:
            preview.verify()
    return len(paths)


def main():
    families = read_json('canonical-families.json')['families']
    sheets = read_json('native-sheets-report.json')
    index = read_json('equipment-effects-index.json')
    catalog = read_json('catalog.json')
    check_sources(index)
    links = check_links()
    previews = check_previews(families, sheets)
    notes = {}
    for part in ('early', 'late'):
        notes.update(read_json(f'visual-style-notes-{part}.json'))
    if set(notes) != {str(f['item_number']) for f in families}:
        raise ValueError('Visual notes do not cover the canonical families')
    result = dict(passed=True, local_links_checked=links, previews_checked=previews,
                  armor_families=len(families), source_hashes_checked=len(index['files']),
                  catalog_summary=catalog['summary'], ingame_validation=False)
    (DOCS / 'reference-validation.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
