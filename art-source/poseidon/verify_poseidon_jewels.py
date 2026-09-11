"""Reopen the saved jewels scene and independently decode the exported BMDs."""
import hashlib
import json
import sys
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent
sys.path[:0] = [str(ROOT), str(ROOT.parent / 'celestial')]

from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from poseidon_jewels_export import ITEM_TRANSFORMS
from poseidon_jewels_shapes import BUILDERS
from verify_prop_roundtrip import compare, decoded_triangles


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'poseidon-jewels.blend'
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    reports = {}
    for name in BUILDERS:
        bpy.data.collections['Poseidon ' + name].hide_viewport = False
    bpy.context.view_layer.update()
    for name in BUILDERS:
        collection = bpy.data.collections['Poseidon ' + name]
        expected = triangle_groups(collection.objects, ITEM_TRANSFORMS[name])
        model = inspect(output / 'models' / f'Poseidon_{name}.bmd', True)
        reports[name] = dict(max_error=compare(expected, decoded_triangles(model)),
                             triangles=sum(map(len, expected.values())),
                             sha256=model['sha256'])
    report = dict(saved_blend_sha256=hashlib.sha256(scene_file.read_bytes()).hexdigest(),
                  models=reports)
    (output / 'jewels-saved-roundtrip-report.json').write_text(json.dumps(report, indent=2),
                                                               encoding='utf-8')
    print('POSEIDON_JEWELS_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
