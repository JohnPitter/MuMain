"""Read native attachment measurements; never reuse the native surface geometry."""
import json
import sys
from pathlib import Path

from mathutils import Vector

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from import_native_reference import world_matrices
from inspect_bmd_rig import inspect


def main():
    directory = Path(sys.argv[sys.argv.index('--') + 1])
    model = inspect(directory / 'ArmorMale74.bmd', True)
    bind = world_matrices(model, 0)
    for index, (bone, matrix) in enumerate(zip(model['bones'], bind)):
        if not bone.get('dummy'):
            print(index, bone['name'], tuple(round(value, 2) for value in matrix.translation))
    for name in ('HelmMale74', 'ArmorMale74', 'PantMale74', 'GloveMale40', 'BootMale74'):
        item = inspect(directory / f'{name}.bmd', True)
        frames = world_matrices(item, 0)
        points = [frames[node] @ Vector(point) for mesh in item['meshes'] for node, point in mesh['points']]
        print(name, [(round(min(p[i] for p in points), 2), round(max(p[i] for p in points), 2)) for i in range(3)])


if __name__ == '__main__':
    main()
