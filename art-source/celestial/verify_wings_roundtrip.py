"""Compare BMD skin, UVs, normals and all flap keyframes with the editable source."""
import json
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

import bpy
from mathutils import Matrix

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from artifact_proof import digest
from armor_animation import verify_pose
from export_prop_bmd import triangle_groups
from export_skinned_bmd import ExportSpec, export
from import_native_reference import world_matrices
from inspect_bmd_rig import inspect
from verify_armor_roundtrip import decoded, validate_area
from verify_prop_roundtrip import compare

OUTPUT = ROOT / 'authored-wings'


def compare_geometry(group, model):
    expected = triangle_groups(group.objects, Matrix.Identity(4), world_matrices(model, 0))
    expected = {texture: [[(*corner[:3], (corner[3],)) for corner in triangle]
                          for triangle in triangles] for texture, triangles in expected.items()}
    return compare(expected, decoded(model))


def verify_armor_regression():
    output = ROOT / 'authored-armor'
    bpy.ops.wm.open_mainfile(filepath=str(output / 'celestial-authored-armor.blend'))
    bpy.context.scene.frame_set(1)
    with TemporaryDirectory(prefix='celestial-bmd-regression-') as temporary:
        for name in ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots'):
            original = output / 'Data' / 'Player' / f'Celestial_{name}.bmd'
            model = inspect(original, True)
            path = Path(temporary) / original.name
            export(bpy.data.collections['Celestial ' + name], world_matrices(model, 0), ExportSpec(model, path))
            if original.read_bytes() != path.read_bytes():
                raise ValueError(f'Armor changed through shared exporter: {name}')


def main():
    verify_armor_regression()
    bpy.ops.wm.open_mainfile(filepath=str(OUTPUT / 'celestial-authored-wings.blend'))
    bpy.context.scene.frame_set(1)
    model = inspect(OUTPUT / 'Data' / 'Item' / 'Celestial_Wings.bmd', True)
    group = bpy.data.collections['Celestial Wings']
    report = dict(max_geometry_error=compare_geometry(group, model),
                  minimum_triangle_area=validate_area(group), unchanged_armor_files=5,
                  model_sha256=model['sha256'],
                  blend_sha256=digest(OUTPUT / 'celestial-authored-wings.blend'))
    maximum, checked = 0, 0
    for frame in range(model['action_frames'][0]):
        bpy.context.scene.frame_set(frame * 4 + 1)
        error, count = verify_pose(bpy.data.objects['CelestialWingRig'],
                                   world_matrices(model, frame), list(group.objects))
        maximum, checked = max(maximum, error), checked + count
    report.update(max_pose_error=maximum, checked_vertices=checked,
                  keyframes=model['action_frames'][0])
    (OUTPUT / 'roundtrip-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('WINGS_ROUNDTRIP_VERIFIED', json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
