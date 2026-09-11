"""Build the Poseidon client candidate package; never publish or install it.

Adapted from the Celestial ``package_candidate.py`` for the larger Poseidon set
(13 BMDs + the four authored atlases installed in both asset folders). Produces
a zip plus a JSON report with per-file SHA-256, BMD structure proofs and the
executable reference check. The connection DLL and every unrelated client file
stay untouched: only Main.exe and the ``Data/`` payload below are shipped.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'celestial'))
from inspect_bmd_rig import inspect  # noqa: E402

ROOT = Path(__file__).resolve().parent
MODELS = ROOT / 'prototype' / 'models'
ATLASES = ROOT / 'textures' / 'ozj'
ATLAS_NAMES = ('Poseidon_Black.OZJ', 'Poseidon_Gold.OZJ', 'Poseidon_Blue.OZJ', 'Poseidon_Pearl.OZJ')
PLAYER_MODELS = ('Poseidon_Helm', 'Poseidon_Armor', 'Poseidon_Pant', 'Poseidon_Glove', 'Poseidon_Boot')
ITEM_MODELS = ('Poseidon_Trident', 'Poseidon_Scepter', 'Poseidon_Cape', 'Poseidon_Pendant',
               'Poseidon_RingTide', 'Poseidon_RingEmperor', 'Poseidon_Black_Mount',
               'Poseidon_Imperial_Eagle')
MODEL_PATHS = {f'Data/Player/{name}.bmd' for name in PLAYER_MODELS}
MODEL_PATHS |= {f'Data/Item/{name}.bmd' for name in ITEM_MODELS}


def digest(source):
    return hashlib.sha256(source.read_bytes()).hexdigest()


def validate_path(relative):
    path = Path(relative)
    if not path.parts or path.is_absolute() or '..' in path.parts or path.parts[0] != 'Data' or ':' in relative:
        raise ValueError(f'Unsafe package path: {relative}')


def collect_payload():
    payload = {}
    for name in PLAYER_MODELS:
        payload[f'Data/Player/{name}.bmd'] = MODELS / f'{name}.bmd'
    for name in ITEM_MODELS:
        payload[f'Data/Item/{name}.bmd'] = MODELS / f'{name}.bmd'
    for atlas in sorted(ATLASES.iterdir()):
        if atlas.name not in ATLAS_NAMES:
            raise ValueError(f'Unexpected file in atlas folder: {atlas.name}')
        payload[f'Data/Player/{atlas.name}'] = atlas
        payload[f'Data/Item/{atlas.name}'] = atlas
    return payload


def validate_assets(payload):
    """Every BMD must be structurally sound and every mesh texture packaged."""
    actual = {path for path in payload if path.lower().endswith('.bmd')}
    if actual != MODEL_PATHS:
        raise ValueError(f'Incomplete or unexpected equipment models: {actual ^ MODEL_PATHS}')
    atlases = {Path(path).name for path in payload if path.lower().endswith('.ozj')}
    if atlases != set(ATLAS_NAMES):
        raise ValueError(f'Unexpected atlas set: {atlases}')
    structure = {}
    for relative in sorted(actual):
        model = inspect(payload[relative], True)
        structure[relative] = {key: model[key] for key in ('bones', 'actions') if key in model}
        structure[relative]['meshes'] = len(model['meshes'])
        for mesh in model['meshes']:
            texture = str(Path(relative).parent / Path(mesh['texture']).with_suffix('.OZJ')).replace('\\', '/')
            if texture not in payload:
                raise ValueError(f'Missing texture for {relative}: {texture}')
            if payload[texture].read_bytes()[24:26] != b'\xff\xd8':
                raise ValueError(f'Invalid OZJ JPEG payload: {texture}')
    return {relative: digest(source) for relative, source in sorted(payload.items())}, structure


def validate_executable(path):
    data = path.read_bytes()
    if data[:2] != b'MZ':
        raise ValueError('Expected a Windows client executable')
    offset, = struct.unpack_from('<I', data, 0x3c)
    if data[offset:offset + 4] != b'PE\0\0' or struct.unpack_from('<H', data, offset + 4)[0] != 0x8664:
        raise ValueError('Expected the x64 client build')
    for model in MODEL_PATHS:
        stem = Path(model).stem
        if stem.encode('utf-16-le') not in data:
            raise ValueError(f'Client does not reference the authored model: {model}')


def archive(payload):
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, 'w', compression=zipfile.ZIP_DEFLATED) as output:
        for relative, source in sorted(payload.items()):
            info = zipfile.ZipInfo(relative, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(info, source.read_bytes())
    return buffer.getvalue()


def build(executable, destination):
    payload = collect_payload()
    hashes, structure = validate_assets(payload)
    validate_executable(executable)
    payload['Main.exe'] = executable
    hashes['Main.exe'] = digest(executable)
    data = archive(payload)
    fingerprint = hashlib.sha256(data).hexdigest()
    destination.mkdir(parents=True, exist_ok=True)
    target = destination / f'poseidon-candidate-{fingerprint[:16]}.zip'
    target.write_bytes(data)
    report = dict(status='candidate only; artistic fidelity, performance and ingame acceptance pending',
                  archive=target.name, sha256=fingerprint, assets=len(payload) - 1,
                  models=sorted(MODEL_PATHS), model_structure=structure, files=hashes,
                  main_sha256=hashes['Main.exe'],
                  deployment='not published; preserve the current connection DLL and all unrelated client files')
    target.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--output', type=Path,
                        default=Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/poseidon-integration-20260911'))
    args = parser.parse_args()
    print(json.dumps(build(args.executable, args.output), indent=2))
