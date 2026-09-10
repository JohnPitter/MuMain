"""Build an internally consistent test package; never publish or install it."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile

from inspect_bmd_rig import inspect
from artifact_proof import digest, validate_proof

ROOT = Path(__file__).resolve().parent
SOURCES = ('authored-props', 'authored-armor', 'authored-wings')
MODEL_PATHS = {f'Data/Item/Celestial_{name}.bmd' for name in ('Staff', 'Shield', 'Ring', 'Pendant', 'Wings')}
MODEL_PATHS |= {f'Data/Player/Celestial_{name}.bmd' for name in ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots')}


def validate_path(relative):
    path = Path(relative)
    if not path.parts or path.is_absolute() or '..' in path.parts or path.parts[0] != 'Data' or ':' in relative:
        raise ValueError(f'Unsafe package path: {relative}')


def add_asset(payload, relative, source):
    validate_path(relative)
    if relative in payload and digest(payload[relative]) != digest(source):
        raise ValueError(f'Conflicting shared texture: {relative}')
    payload[relative] = source


def collect_payload():
    payload = {}
    for name in SOURCES:
        directory = ROOT / name
        manifest = json.loads((directory / 'sha256.json').read_text())
        proofs = json.loads((directory / 'roundtrip-report.json').read_text())
        blend = directory / f'celestial-{name}.blend'
        for relative, expected in manifest.items():
            validate_path(relative)
            source = directory / relative
            if digest(source) != expected:
                raise ValueError(f'Generated file differs from its manifest: {source}')
            if source.suffix.lower() == '.bmd':
                proof = proofs if name == 'authored-wings' else proofs[source.stem.removeprefix('Celestial_')]
                validate_proof(proof, source, blend)
            add_asset(payload, relative, source)
    return payload


def validate_assets(payload):
    actual = {path for path in payload if path.lower().endswith('.bmd')}
    if actual != MODEL_PATHS:
        raise ValueError(f'Incomplete or unexpected equipment models: {actual ^ MODEL_PATHS}')
    for relative in sorted(actual):
        model = inspect(payload[relative], True)
        for mesh in model['meshes']:
            texture = str(Path(relative).parent / Path(mesh['texture']).with_suffix('.OZJ')).replace('\\', '/')
            if texture not in payload:
                raise ValueError(f'Missing texture for {relative}: {texture}')
            if payload[texture].read_bytes()[24:26] != b'\xff\xd8':
                raise ValueError(f'Invalid OZJ JPEG payload: {texture}')
    return {relative: digest(source) for relative, source in sorted(payload.items())}


def validate_executable(path):
    data = path.read_bytes()
    if data[:2] != b'MZ':
        raise ValueError('Expected a Windows client executable')
    offset, = struct.unpack_from('<I', data, 0x3c)
    if data[offset:offset + 4] != b'PE\0\0' or struct.unpack_from('<H', data, offset + 4)[0] != 0x8664:
        raise ValueError('Expected the x64 client build')
    for model in MODEL_PATHS:
        if Path(model).stem.encode('utf-16-le') not in data:
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
    hashes = validate_assets(payload)
    validate_executable(executable)
    payload['Main.exe'] = executable
    hashes['Main.exe'] = digest(executable)
    data = archive(payload)
    fingerprint = hashlib.sha256(data).hexdigest()
    destination.mkdir(parents=True, exist_ok=True)
    target = destination / f'celestial-candidate-{fingerprint[:16]}.zip'
    target.write_bytes(data)
    report = dict(status='candidate only; artistic fidelity, performance and ingame acceptance pending',
                  archive=target.name, sha256=fingerprint, assets=len(payload) - 1, files=hashes,
                  deployment='not published; preserve the current connection DLL and all unrelated client files')
    target.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'packaged-review')
    args = parser.parse_args()
    print(json.dumps(build(args.executable, args.output), indent=2))
