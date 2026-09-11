"""Assemble diagnostic native renders with source family labels and coverage."""
import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2] / 'docs/art/armor-reference'
TILE_WIDTH, TILE_HEIGHT = 320, 422
COLUMNS, ROWS = 5, 2


def font(size):
    path = Path('C:/Windows/Fonts/arial.ttf')
    return ImageFont.truetype(str(path), size) if path.exists() else ImageFont.load_default()


def label(family):
    entries = family['pieces'].get('Armor', [])
    if not entries:
        entries = next(iter(family['pieces'].values()))
    server = entries[0].get('server') or {}
    name = server.get('name') or f"Family {family['item_number']}"
    return f"{family['item_number']:02d} | {name}"


def compose(families, page, view, directory):
    image = Image.new('RGB', (TILE_WIDTH * COLUMNS, TILE_HEIGHT * ROWS + 40), '#202023')
    draw = ImageDraw.Draw(image)
    draw.text((12, 8), f'BMD nativo | {view} | pose-base | sem efeitos de runtime', font=font(20), fill='white')
    for index, family in enumerate(families):
        x = index % COLUMNS * TILE_WIDTH
        y = index // COLUMNS * TILE_HEIGHT + 40
        path = ROOT / 'native-renders' / f"family-{family['item_number']:03d}-{view}.png"
        with Image.open(path) as preview:
            image.paste(preview.convert('RGB'), (x, y))
        draw.text((x + 6, y + 390), label(family)[:42], font=font(16), fill='white')
    target = directory / f'{view}-{page + 1:02d}.png'
    image.save(target)
    return target.name


def load_records(catalog):
    records, missing = {}, set()
    for start in range(0, len(catalog['families']), 10):
        report = json.loads((ROOT / f'native-render-report-{start:03d}.json').read_text())
        missing.update(report['missing_textures'])
        for record in report['families']:
            if 'error' in record or len(record['views']) != 2:
                raise ValueError('Incomplete family render: ' + str(record['item_number']))
            if record['item_number'] in records:
                raise ValueError('Duplicate family render: ' + str(record['item_number']))
            records[record['item_number']] = record
    if set(records) != {f['item_number'] for f in catalog['families']}:
        raise ValueError('Rendered families do not cover the canonical catalog')
    return records, missing


def compose_pages(catalog, selected):
    directory = ROOT / 'native-sheets'
    directory.mkdir(exist_ok=True)
    sheets = []
    for start in range(0, len(catalog['families']), COLUMNS * ROWS):
        families = catalog['families'][start:start + COLUMNS * ROWS]
        for view in ('front', 'rear'):
            page = start // (COLUMNS * ROWS)
            name = f'{view}-{page + 1:02d}.png'
            if selected is None or selected.intersection(f['item_number'] for f in families):
                compose(families, page, view, directory)
            if not (directory / name).is_file():
                raise ValueError('Missing untouched sheet: ' + name)
            sheets.append(name)
    return sheets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--families', type=int, nargs='+')
    args = parser.parse_args()
    catalog = json.loads((ROOT / 'canonical-families.json').read_text(encoding='utf-8-sig'))
    records, missing = load_records(catalog)
    selected = set(args.families) if args.families is not None else None
    if selected is not None and not selected.issubset(records):
        raise ValueError('Requested family is not in the canonical catalog')
    sheets = compose_pages(catalog, selected)
    result = dict(families=len(records), views=len(records) * 2, sheets=sheets,
                  missing_textures=sorted(missing), source=catalog['source'],
                  hidden_meshes_skipped=sum(len(r.get('hidden_meshes_skipped', [])) for r in records.values()),
                  families_with_hidden_meshes_skipped=[key for key, r in records.items() if r.get('hidden_meshes_skipped')],
                  resolved_texture_fallbacks=[dict(item_number=key, **fallback) for key, r in records.items()
                                             for fallback in r.get('texture_fallbacks', [])],
                  validation='Diagnostic Blender renders, not screenshots or full runtime effects.')
    (ROOT / 'native-sheets-report.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    merged = dict(result, families=[records[f['item_number']] for f in catalog['families']])
    (ROOT / 'native-render-report.json').write_text(json.dumps(merged, indent=2), encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'source'}))


if __name__ == '__main__':
    main()
