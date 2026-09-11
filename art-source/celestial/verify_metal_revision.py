"""Prove a finish-only revision leaves all model bytes and animation intact."""
import argparse
import hashlib
from io import BytesIO
import json
from pathlib import Path
import zipfile

from PIL import Image

ROOT = Path(__file__).resolve().parent
EXPECTED_PREVIOUS_BASE = '10141413669b3e061a7082c4933cec4902f03f7540cf73e1526fa65442e426ed'
CHANGED_TEXTURES = {f'Data/{folder}/Celestial_{name}.OZJ'
                    for folder in ('Player', 'Item') for name in ('Gold', 'Ivory')}


def current_assets():
    result = {}
    for folder in ('authored-armor', 'authored-props', 'authored-wings'):
        for path in (ROOT / folder / 'Data').rglob('*'):
            if not path.is_file():
                continue
            name = path.relative_to(ROOT / folder).as_posix()
            raw = path.read_bytes()
            if name in result and result[name] != raw:
                raise ValueError(f'Conflicting asset copies: {name}')
            result[name] = raw
    return result


def validate_texture(name, raw):
    if raw[:24] != bytes(24):
        raise ValueError(f'Unexpected OZJ wrapper: {name}')
    source = ROOT / 'textures' / Path(name).with_suffix('.jpg').name
    if raw[24:] != source.read_bytes():
        raise ValueError(f'Packaged texture differs from editable source: {name}')
    with Image.open(BytesIO(raw[24:])) as image:
        if image.size != (512, 512) or image.mode != 'RGB' or image.format != 'JPEG':
            raise ValueError(f'Texture format changed: {name}')
        image.load()


def verify(base):
    with base.open('rb') as stream:
        if hashlib.file_digest(stream, 'sha256').hexdigest() != EXPECTED_PREVIOUS_BASE:
            raise ValueError('Reference must be the previously deployed Golden base')
    assets = current_assets()
    if len(assets) != 17:
        raise ValueError('Expected exactly seventeen equipment assets')
    changes, models, preserved = [], {}, []
    with zipfile.ZipFile(base) as archive:
        entries = {entry.filename.replace('\\', '/').lower(): entry for entry in archive.infolist()}
        for name, raw in assets.items():
            previous = archive.read(entries[name.lower()])
            if raw != previous:
                changes.append(name)
            if name.endswith('.bmd'):
                if raw != previous:
                    raise ValueError(f'Geometry or animation changed: {name}')
                models[name] = hashlib.sha256(raw).hexdigest()
            elif name in CHANGED_TEXTURES:
                validate_texture(name, raw)
            elif raw == previous:
                preserved.append(name)
    if set(changes) != CHANGED_TEXTURES or len(models) != 10:
        raise ValueError('Revision must change only four gold/steel texture copies')
    return dict(passed=True, previous_base_sha256=EXPECTED_PREVIOUS_BASE, model_hashes=models,
                models_byte_identical=10, geometry_rig_uv_actions_unchanged=True,
                changed_textures=sorted(changes), preserved_textures=sorted(preserved),
                texture_dimensions=[512, 512], texture_format='RGB JPEG + 24-byte OZJ header',
                ingame_appearance_verified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base', type=Path)
    args = parser.parse_args()
    result = verify(args.base)
    (ROOT / 'research/metal-revision-integrity.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
