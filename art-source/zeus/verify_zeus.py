"""Reopen the saved Blender scene and independently decode prototype BMD files."""
import hashlib
import json
import sys
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent
sys.path[:0] = [str(ROOT), str(ROOT.parent / 'celestial')]

from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from zeus_export import ANCHORS, HAND_TRANSFORM
from verify_prop_roundtrip import compare, decoded_triangles


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'zeus-weapons.blend'
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    for name in ANCHORS:
        bpy.data.collections['Zeus ' + name].hide_viewport = False
    bpy.context.view_layer.update()
    reports = {}
    for name in ANCHORS:
        collection = bpy.data.collections['Zeus ' + name]
        expected = triangle_groups(collection.objects, HAND_TRANSFORM)
        model = inspect(output / 'models' / f'Zeus_{name}.bmd', True)
        reports[name] = dict(max_error=compare(expected, decoded_triangles(model)),
                             triangles=sum(map(len, expected.values())), sha256=model['sha256'])
    report = dict(saved_blend_sha256=hashlib.sha256(scene_file.read_bytes()).hexdigest(), models=reports)
    (output / 'saved-roundtrip-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('ZEUS_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
