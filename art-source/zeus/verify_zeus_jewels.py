"""Reopen the saved Zeus jewels scene and independently decode the exported BMD payloads."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare
from zeus_armor_rig import decoded_skinned_triangles
from zeus_jewels_shapes import ANCHORS
from zeus_wings_rig import digest


def expected_triangles(objects):
    grouped = triangle_groups(objects, Matrix.Identity(4), [Matrix.Identity(4)])
    return {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                      for triangle in triangles] for texture, triangles in grouped.items()}


def main():
    output = ROOT / 'prototype'
    scene_file = output / 'zeus-jewels.blend'
    bpy.ops.wm.open_mainfile(filepath=str(scene_file))
    reports = {}
    for name in ('Pendant', 'Ring_Storm', 'Ring_Wisdom'):
        bpy.data.collections['Zeus ' + name].hide_viewport = False
    bpy.context.view_layer.update()
    for name in ('Pendant', 'Ring_Storm', 'Ring_Wisdom'):
        collection = bpy.data.collections['Zeus ' + name]
        model = inspect(output / 'models' / f'Zeus_{name}.bmd', True)
        reports[name] = dict(
            max_error=compare(expected_triangles(collection.objects),
                              decoded_skinned_triangles(model)),
            triangles=sum(mesh['triangles'] for mesh in model['meshes']),
            contract_bone=model['bones'][0]['name'],
            anchors=model_anchor_probe(model, ANCHORS[name]),
            sha256=model['sha256'])
    report = dict(saved_blend_sha256=digest(scene_file), models=reports)
    (output / 'jewels-saved-roundtrip-report.json').write_text(json.dumps(report, indent=2),
                                                               encoding='utf-8')
    print('ZEUS_JEWELS_SAVED_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


def model_anchor_probe(model, anchors):
    """Nearest exported vertex to each declared anchor, proving placement."""
    from mathutils import Vector
    points = [Vector(point) for mesh in model['meshes'] for _, point in mesh['points']]
    probe = {}
    for label, position in anchors.items():
        target = Vector(position)
        probe[label] = round(min((point - target).length for point in points), 4)
    return probe


if __name__ == '__main__':
    main()
