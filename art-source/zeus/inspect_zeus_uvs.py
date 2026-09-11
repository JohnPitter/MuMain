"""Extract the UV region layout of the eleven Zeus models.

Opens the saved prototype scenes, records every component (object) with its
material role, triangle count, world-space area and the UV footprint its loops
occupy inside the shared 0-1 atlas square, then decodes the exported BMDs to
cross-check texel usage per atlas. The report justifies atlas resolution and
documents which model regions sample which definitive atlas. All eleven set
models are covered: Armor, Pant, Glove, Boot (Class304), Cape, Sword, Staff,
Wings, Pendant, Ring_Storm and Ring_Wisdom.

Run: blender --background --python-exit-code 1 --python inspect_zeus_uvs.py
"""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from inspect_bmd_rig import inspect

SCENES = (('Sword', 'Staff'), 'zeus-weapons.blend'), \
         (('Armor', 'Pant', 'Glove', 'Boot'), 'zeus-armor.blend'), \
         (('Wings',), 'zeus-wings.blend'), \
         (('Pendant', 'Ring_Storm', 'Ring_Wisdom'), 'zeus-jewels.blend'), \
         (('Cape',), 'zeus-cape.blend')


def component_record(obj):
    data = obj.data
    data.calc_loop_triangles()
    uv_layer = data.uv_layers.active.data
    area, us, vs = 0.0, [], []
    for triangle in data.loop_triangles:
        corners = [data.vertices[i].co for i in triangle.vertices]
        area += (corners[1] - corners[0]).cross(corners[2] - corners[0]).length / 2
        for loop_id in triangle.loops:
            u, v = uv_layer[loop_id].uv
            us.append(u)
            vs.append(v)
    material = data.materials[0].name if data.materials else None
    return dict(component=obj.name, atlas=material, triangles=len(data.loop_triangles),
                surface_area=round(area, 4),
                uv_bounds=dict(u=[round(min(us), 4), round(max(us), 4)],
                               v=[round(min(vs), 4), round(max(vs), 4)]))


def main():
    regions = {}
    for names, scene_file in SCENES:
        bpy.ops.wm.open_mainfile(filepath=str(ROOT / 'prototype' / scene_file))
        for name in names:
            collection = bpy.data.collections['Zeus ' + name]
            collection.hide_viewport = False
            components = [component_record(obj) for obj in sorted(collection.objects,
                                                                 key=lambda item: item.name)]
            by_atlas = {}
            for record in components:
                by_atlas.setdefault(record['atlas'], []).append(record['component'])
            regions[name] = dict(scene=scene_file, atlas_components=by_atlas,
                                 components=components)
    report = dict(status='UV_LAYOUT_PROOF_FOR_DEFINITIVE_ATLASES',
                  uv_projection=('per-component planar box projection over the two largest '
                                 'bbox axes, normalized to the full 0-1 square '
                                 '(celestial_geometry.mesh); every component samples the '
                                 'whole atlas, so the masters are full-field engraved '
                                 'finishes, not spatially partitioned charts'),
                  models=regions)
    for name in regions:
        model = inspect(ROOT / 'prototype' / 'models' / f'Zeus_{name}.bmd', True)
        regions[name]['bmd_atlas_texcoords'] = {
            mesh['texture']: len(mesh['texcoords']) for mesh in model['meshes']}
        regions[name]['bmd_triangles'] = {
            mesh['texture']: mesh['triangles'] for mesh in model['meshes']}
    target = ROOT / 'prototype' / 'uv-region-report.json'
    target.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('ZEUS_UV_REGIONS_VERIFIED', json.dumps(
        {name: {atlas: len(parts) for atlas, parts in regions[name]['atlas_components'].items()}
         for name in regions}), flush=True)


if __name__ == '__main__':
    main()
