"""Compare finished material statistics with a frozen diagnostic baseline; write JSON only."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image

from inspect_knight_reference import texture_stats
from textures import palette


def read_current(path):
    before = hashlib.sha256(path.read_bytes()).hexdigest()
    with Image.open(path) as image:
        record = dict(path=path.as_posix(), sha256=before, bytes=path.stat().st_size,
                      format=image.format, mode=image.mode, width=image.width, height=image.height,
                      palette=palette(image.convert('RGBA')), statistics=texture_stats(image))
    record['unchanged_after_read'] = before == hashlib.sha256(path.read_bytes()).hexdigest()
    return record


def compare(baseline, current):
    old, new = baseline['statistics'], current['statistics']
    old_span = old['luminance_p95'] - old['luminance_p05']
    new_span = new['luminance_p95'] - new['luminance_p05']
    return dict(baseline=baseline, current=current,
                comparison=dict(baseline_luminance_p95_p05=old_span, current_luminance_p95_p05=new_span,
                                contrast_span_ratio=round(new_span / old_span, 4) if old_span else None))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--textures', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    baseline = json.loads(args.baseline.read_text(encoding='utf-8'))
    records = []
    for name in ('Celestial_Gold', 'Celestial_Ivory'):
        old = next(t for t in baseline['textures'] if t['entry'] == f'Data/Item/{name}.OZJ')
        records.append(compare(old, read_current((args.textures / f'{name}.jpg').resolve())))
    valid = all(r['current']['unchanged_after_read'] and r['current']['format'] == 'JPEG'
                and r['current']['mode'] == 'RGB' and r['current']['width'] == r['current']['height'] == 512
                for r in records)
    if not valid:
        raise ValueError('Falha de integridade ou formato das texturas')
    report = dict(baseline_archive=baseline['archive'], baseline_archive_sha256=baseline['archive_sha256'],
                  materials=records, validation=dict(passed=valid, image_writes=False, assets_modified=False),
                  interpretation='A nova pintura amplia o contraste do bitmap. Não comprova alinhamento UV, densidade das gravuras, brilho ou desempenho no cliente real.')
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(dict(passed=valid, comparison=[r['comparison'] for r in records]), indent=2))


if __name__ == '__main__':
    main()
