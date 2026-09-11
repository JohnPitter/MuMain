"""Build original weapon prototypes, verify BMD roundtrip, then render inspection views."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import bmesh
import bpy

ROOT = Path(__file__).resolve().parent
SHARED = ROOT.parent / 'celestial'
sys.path[:0] = [str(ROOT), str(SHARED)]

from celestial_geometry import collection
from export_prop_bmd import triangle_groups
from inspect_bmd_rig import inspect
from verify_prop_roundtrip import compare, decoded_triangles
from zeus_export import HAND_TRANSFORM, export_weapon
from zeus_materials import PALETTE, create_materials
from zeus_preview import render_head, render_weapon, studio
from zeus_shapes import sword, staff


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def topology_report(objects):
    count, nonmanifold = 0, []
    for obj in objects:
        topology = bmesh.new()
        topology.from_mesh(obj.data)
        count += len(topology.faces)
        if any(not edge.is_manifold for edge in topology.edges):
            nonmanifold.append(obj.name)
        topology.free()
    if nonmanifold:
        raise ValueError(f'Open component meshes: {nonmanifold}')
    return dict(objects=len(objects), polygons=count, nonmanifold_components=nonmanifold)


def build_models(materials, output):
    collections, reports = {}, {}
    for name, builder in (('Sword', sword), ('Staff', staff)):
        item = collection('Zeus ' + name, lambda: builder(materials))
        collections[name] = item
        bpy.context.view_layer.update()
        topology = topology_report(item.objects)
        target = output / 'models' / f'Zeus_{name}.bmd'
        reports[name] = export_weapon(item, name, target)
        model = inspect(target, True)
        expected = triangle_groups(item.objects, HAND_TRANSFORM)
        reports[name].update(topology=topology, sha256=digest(target),
                             roundtrip_max_error=compare(expected, decoded_triangles(model)))
    return collections, reports


def write_reports(output, reports, shared_hashes):
    dependencies = [dict(texture=f'Zeus_{role}.jpg', material_role=role,
                         status='MISSING_ATLAS_NOT_FOR_CLIENT', linear_rgb=list(values[0]))
                    for role, values in PALETTE.items()]
    (output / 'texture-dependencies.json').write_text(json.dumps(dependencies, indent=2), encoding='utf-8')
    report = dict(status='AUTHORING_PROTOTYPE_ONLY', renderer='Blender Cycles / AgX, not MU runtime',
                  reference='concept-herdeiro-de-zeus-mg-20260911.png (sha256 7d5bd6e0370f792f06faa4048987433e163916ff3992e94762399c76b249d573)',
                  model_origin='Original Zeus geometry; generic shared primitives/encoder only',
                  shared_source_sha256=shared_hashes,
                  blend_sha256=digest(output / 'zeus-weapons.blend'), models=reports)
    (output / 'build-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skip-render', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    output = ROOT / 'prototype'
    for name in ('models', 'renders'):
        (output / name).mkdir(parents=True, exist_ok=True)
    shared_files = [SHARED / f for f in ('celestial_geometry.py', 'export_prop_bmd.py',
                                         'inspect_bmd_rig.py', 'verify_prop_roundtrip.py')]
    before = {path.name: digest(path) for path in shared_files}
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    collections, reports = build_models(create_materials(), output)
    studio()
    if not args.skip_render:
        for name in collections:
            render_weapon(collections, name, output / 'renders')
            render_head(collections, name, output / 'renders')
    for name, item in collections.items():
        item.hide_render = name != 'Sword'
        item.hide_viewport = name != 'Sword'
    bpy.context.scene.camera.location = (0, -300, 32)
    bpy.context.scene.camera.rotation_euler = (1.57079632679, 0, 0)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'zeus-weapons.blend'))
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, reports, before)
    print('ZEUS_PROTOTYPES_VERIFIED', json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
