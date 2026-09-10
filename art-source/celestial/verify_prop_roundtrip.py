"""Verify saved Blender geometry against the independently decoded BMD payload."""
from collections import defaultdict
import json
import sys
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from export_prop_bmd import TRANSFORMS, triangle_groups
from inspect_bmd_rig import inspect

OUTPUT = ROOT / 'authored-props'
TOLERANCE = 0.00002


def decoded_triangles(model):
    result = defaultdict(list)
    for index, mesh in enumerate(model['meshes']):
        if mesh['texture_index'] != index:
            raise ValueError('Unexpected texture index')
        for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
            corners = []
            for vertex, normal, uv in zip(vertices, normals, uvs):
                corners.append((mesh['points'][vertex][1], mesh['normal_vectors'][normal][1:4],
                                mesh['texcoords'][uv]))
            result[mesh['texture']].append(corners)
    return result


def compare(expected, decoded):
    if set(expected) != set(decoded):
        raise ValueError('Materials changed during export')
    maximum = 0
    for texture, triangles in expected.items():
        if len(triangles) != len(decoded[texture]):
            raise ValueError('Triangle count changed during export')
        for source, actual in zip(triangles, decoded[texture]):
            for source_corner, actual_corner in zip(source, actual):
                for source_values, actual_values in zip(source_corner, actual_corner):
                    maximum = max(maximum, *(abs(a - b) for a, b in zip(source_values, actual_values)))
    if maximum > TOLERANCE:
        raise ValueError(f'Geometry, normal, or UV changed: {maximum}')
    return maximum


def main():
    bpy.ops.wm.open_mainfile(filepath=str(OUTPUT / 'celestial-authored-props.blend'))
    reports = {}
    for name in TRANSFORMS:
        bpy.data.collections['Celestial ' + name].hide_viewport = False
    bpy.context.view_layer.update()
    for name, transform in TRANSFORMS.items():
        collection = bpy.data.collections['Celestial ' + name]
        expected = triangle_groups(collection.objects, transform)
        decoded = decoded_triangles(inspect(OUTPUT / 'Data' / 'Item' / f'Celestial_{name}.bmd', True))
        print('VERIFYING', name, flush=True)
        reports[name] = dict(max_error=compare(expected, decoded), triangles=sum(map(len, expected.values())))
    (OUTPUT / 'roundtrip-report.json').write_text(json.dumps(reports, indent=2), encoding='utf-8')
    print('ROUNDTRIP_VERIFIED', json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
