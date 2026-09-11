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
from poseidon_export import HAND_TRANSFORM, export_weapon
from poseidon_materials import atlas_dependencies, create_materials
from poseidon_preview import render_head, render_weapon, studio
from poseidon_shapes import scepter, trident


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
    for name, builder in (('Trident', trident), ('Scepter', scepter)):
        item = collection('Poseidon ' + name, lambda: builder(materials))
        collections[name] = item
        bpy.context.view_layer.update()
        topology = topology_report(item.objects)
        target = output / 'models' / f'Poseidon_{name}.bmd'
        reports[name] = export_weapon(item, name, target)
        model = inspect(target, True)
        expected = triangle_groups(item.objects, HAND_TRANSFORM)
        reports[name].update(topology=topology, sha256=digest(target),
                             roundtrip_max_error=compare(expected, decoded_triangles(model)))
    return collections, reports


def write_reports(output, reports, shared_hashes):
    (output / 'texture-dependencies.json').write_text(
        json.dumps(atlas_dependencies(), indent=2), encoding='utf-8')
    report = dict(status='AUTHORING_PROTOTYPE_ONLY', renderer='Blender Cycles / AgX, not MU runtime',
                  reference='codex-clipboard-04794f6e-f53a-4555-a521-d0a673490353.png',
                  model_origin='Original Poseidon geometry; generic shared primitives/encoder only',
                  shared_source_sha256=shared_hashes,
                  blend_sha256=digest(output / 'poseidon-weapons.blend'), models=reports)
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
        item.hide_render = name != 'Trident'
        item.hide_viewport = name != 'Trident'
    bpy.context.scene.camera.location = (0, -300, 25)
    bpy.context.scene.camera.rotation_euler = (1.57079632679, 0, 0)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'poseidon-weapons.blend'))
    if before != {path.name: digest(path) for path in shared_files}:
        raise ValueError('Shared source changed during generation; verify concurrent edits')
    write_reports(output, reports, before)
    print('POSEIDON_PROTOTYPES_VERIFIED', json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
