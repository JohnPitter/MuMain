"""Prove that a palette revision preserved geometry, UVs, normals and animation."""
import argparse
from collections import Counter, defaultdict
import json
import math
from pathlib import Path
from tempfile import TemporaryDirectory
import zipfile

from inspect_bmd_rig import inspect

ROOT = Path(__file__).resolve().parent
TEXTURE_SWAP = {'Celestial_Gold.jpg': 'Celestial_Ivory.jpg',
                'Celestial_Ivory.jpg': 'Celestial_Gold.jpg'}
JEWELRY = ('Celestial_Ring.bmd', 'Celestial_Pendant.bmd')
# export_prop_bmd.intern groups UVs at six decimal places. A new material group
# can retain a different original float from that same bin; positions stay exact.
UV_INTERN_EPSILON = 0.000001
PIECES = {
    **{f'Data/Player/Celestial_{name}.bmd': 'authored-armor'
       for name in ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots')},
    **{f'Data/Item/Celestial_{name}.bmd': 'authored-props'
       for name in ('Staff', 'Shield', 'Pendant', 'Ring')},
    'Data/Item/Celestial_Wings.bmd': 'authored-wings',
}


def face_corners(mesh, vertices, uvs, normals):
    result = []
    for vertex, uv, normal in zip(vertices, uvs, normals):
        bone, position = mesh['points'][vertex]
        normal_bone, *vector, normal_vertex = mesh['normal_vectors'][normal]
        if normal_vertex != vertex:
            raise ValueError('Normal is bound to a different vertex')
        texcoord = mesh['texcoords'][uv]
        if not all(math.isfinite(v) for v in (*position, *vector, *texcoord)):
            raise ValueError('Non-finite geometry, normal or UV')
        result.append(((bone, *position, normal_bone, *vector), tuple(texcoord)))
    return result


def triangles(model):
    result = defaultdict(list)
    for mesh in model['meshes']:
        if len(mesh['faces']) != len(mesh['normal_indices']):
            raise ValueError('Face and normal-index counts differ')
        for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
            if len(vertices) != len(uvs) or len(vertices) != len(normals):
                raise ValueError('Face corner counts differ')
            corners = face_corners(mesh, vertices, uvs, normals)
            rotations = [corners[i:] + corners[:i] for i in range(len(corners))]
            ordered = min(rotations, key=lambda rotation: tuple(c[0] for c in rotation))
            geometry = tuple(c[0] for c in ordered)
            result[geometry].append((mesh['texture'], tuple(c[1] for c in ordered)))
    return result


def texture_allowed(before, after, jewelry):
    if jewelry and before == 'Celestial_Gold.jpg':
        return after in TEXTURE_SWAP
    return after == TEXTURE_SWAP.get(before, before)


def surface_error(before, after, jewelry):
    if not texture_allowed(before[0], after[0], jewelry):
        return math.inf
    return max(abs(a - b) for old, new in zip(before[1], after[1])
               for a, b in zip(old, new))


def match_surfaces(before, after, jewelry):
    distances = [[surface_error(old, new, jewelry) for new in after] for old in before]
    owners = {}

    def assign(old_index, visited):
        remapped_metal = jewelry and before[old_index][0] in TEXTURE_SWAP
        tolerance = UV_INTERN_EPSILON if remapped_metal else 0.0
        for new_index, error in enumerate(distances[old_index]):
            if error > tolerance or new_index in visited:
                continue
            visited.add(new_index)
            if new_index not in owners or assign(owners[new_index], visited):
                owners[new_index] = old_index
                return True
        return False

    for index in range(len(before)):
        if not assign(index, set()):
            raise ValueError('UVs or texture assignment changed beyond the palette contract')
    errors = [distances[old][new] for new, old in owners.items()]
    return max(errors, default=0), sum(error != 0 for error in errors)


def compare_models(old, new, name):
    for key in ('name', 'bones', 'action_frames', 'action_locks', 'action_positions'):
        if key not in old or key not in new or old[key] != new[key]:
            raise ValueError(f'{name}: animation/rig changed or missing: {key}')
    before, after = triangles(old), triangles(new)
    if Counter({k: len(v) for k, v in before.items()}) != Counter({k: len(v) for k, v in after.items()}):
        raise ValueError(f'{name}: geometry, winding, binding or normals changed')
    maximum, changed_uv_triangles = 0.0, 0
    for geometry, surfaces in before.items():
        error, changed = match_surfaces(surfaces, after[geometry], name in JEWELRY)
        maximum = max(maximum, error)
        changed_uv_triangles += changed
    return dict(triangles=sum(map(len, after.values())), old_sha256=old['sha256'],
                new_sha256=new['sha256'], geometry_uv_normals_preserved=True,
                rig_and_animation_preserved=True, geometry_normals_winding_exact=True,
                maximum_uv_error=maximum, changed_uv_triangles=changed_uv_triangles,
                uv_tolerance=UV_INTERN_EPSILON if name in JEWELRY else 0.0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--previous-base', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    result = {}
    with zipfile.ZipFile(args.previous_base) as archive, TemporaryDirectory(prefix='celestial-palette-') as temporary:
        entries = {entry.filename.replace('\\', '/').lower(): entry for entry in archive.infolist()}
        for relative, group in PIECES.items():
            path = Path(temporary) / Path(relative).name
            path.write_bytes(archive.read(entries[relative.lower()]))
            old = inspect(path, True)
            new = inspect(ROOT / group / relative, True)
            result[relative] = compare_models(old, new, path.name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(dict(models=len(result), triangles=sum(r['triangles'] for r in result.values()),
                         report=str(args.report), geometry_and_animations_preserved=True)))


if __name__ == '__main__':
    main()
