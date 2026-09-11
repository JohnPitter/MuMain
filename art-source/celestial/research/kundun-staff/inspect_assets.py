"""Read Staff of Kundun and original material maps; write diagnostic copies only."""
import argparse
import json
from pathlib import Path
import sys
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[4]
sys.path[:0] = [str(ROOT / 'tools/armor_catalog'), str(ROOT / 'art-source/celestial')]
from inspect_bmd_rig import inspect
from inspect_knight_reference import file_digest, model_record, save_texture
from memory_asset import MemoryAsset

MODEL = 'data/item/staff12.bmd'
TEXTURES = ('data/item/kundunstic_r.ozj', 'data/item/kundunstic.ozj', 'data/item/kundunstic2.ozj',
            'data/effect/chrome01.ozj', 'data/effect/chrome02.ozj', 'data/effect/shiny01.ozj',
            'data/item/celestial_ivory.ozj')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    args = parser.parse_args()
    output = Path(__file__).resolve().parent
    before = file_digest(args.archive)
    with ZipFile(args.archive) as archive:
        entries = {name.replace('\\', '/').lower(): name for name in archive.namelist()}
        model = inspect(MemoryAsset(entries[MODEL], archive.read(entries[MODEL])), True)
        textures = [save_texture(entries[name], archive.read(entries[name]), output) for name in TEXTURES]
    record = model_record(entries[MODEL], model)
    record['bones'] = [{key: bone[key] for key in ('name', 'parent', 'dummy') if key in bone} for bone in model['bones']]
    report = dict(archive=str(args.archive.resolve()), archive_sha256=before, item_group=5, item_number=11,
                  model_symbol='MODEL_STAFF_OF_KUNDUN', model=record, textures=textures,
                  validation=dict(archive_unchanged=before == file_digest(args.archive),
                                  original_assets_modified=False, decoded_copies_only=True))
    if not report['validation']['archive_unchanged']:
        raise ValueError('Archive changed during read')
    (output / 'measurements.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(dict(archive_sha256=before, triangles=record['triangles'], vertices=record['vertices'],
                         bones=record['bone_count'], actions=record['action_frames'], validation=report['validation']), indent=2))


if __name__ == '__main__':
    main()
