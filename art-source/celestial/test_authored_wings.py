"""Structural and motion contracts, not artistic or ingame acceptance."""
import hashlib
import json
import math
from pathlib import Path
import unittest

from inspect_bmd_rig import inspect
from artifact_proof import validate_proof

ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / 'authored-wings'


class AuthoredWingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = inspect(OUTPUT / 'Data' / 'Item' / 'Celestial_Wings.bmd', True)
        cls.report = json.loads((OUTPUT / 'wing-report.json').read_text())

    def test_complete_animation_and_halo_anchor(self):
        self.assertEqual(self.model['name'], 'Celestial_Wings')
        self.assertEqual(self.model['action_frames'], [9])
        self.assertEqual(self.model['action_locks'], [True])
        self.assertEqual(len(self.model['action_positions'][0]), 9)
        self.assertEqual(len(self.model['bones']), 48)
        self.assertEqual(self.model['bones'][47]['name'], 'Celestial_Halo')
        self.assertEqual(self.model['bones'][47]['parent'], 0)
        for i, bone in enumerate(self.model['bones']):
            self.assertLess(bone['parent'], i)
            for clip in bone['clips']:
                for channel in ('positions', 'rotations'):
                    self.assertEqual(len(clip[channel]), 9)
                    self.assertTrue(all(math.isfinite(x) for p in clip[channel] for x in p))

    def test_valid_single_bone_surfaces_and_indices(self):
        self.assertLessEqual(len(self.model['meshes']), 50)
        for mesh in self.model['meshes']:
            self.assertTrue(0 < mesh['vertices'] <= 10000)
            self.assertLessEqual(len(mesh['normal_vectors']), 10000)
            for (indices, uv_ids), normals in zip(mesh['faces'], mesh['normal_indices']):
                for index, uv, normal in zip(indices, uv_ids, normals):
                    self.assertTrue(0 <= index < len(mesh['points']))
                    self.assertTrue(0 <= uv < len(mesh['texcoords']))
                    self.assertTrue(0 <= normal < len(mesh['normal_vectors']))
                    bone, *values, bound = mesh['normal_vectors'][normal]
                    self.assertEqual(bone, mesh['points'][index][0])
                    self.assertEqual(bound, index)
                    self.assertTrue(0 <= bone < 48)
                    self.assertAlmostEqual(sum(x*x for x in values), 1, places=4)

    def test_only_authored_textures_and_matching_manifest(self):
        allowed = {f'Celestial_{name}.jpg' for name in ('Gold', 'Ivory', 'Sapphire')}
        self.assertEqual({m['texture'] for m in self.model['meshes']}, allowed)
        for texture in allowed:
            packed = OUTPUT / 'Data' / 'Item' / Path(texture).with_suffix('.OZJ')
            self.assertEqual(packed.read_bytes()[24:], (ROOT / 'textures' / texture).read_bytes())
        manifest = json.loads((OUTPUT / 'sha256.json').read_text())
        self.assertEqual(len(manifest), 4)
        for path, digest in manifest.items():
            self.assertEqual(hashlib.sha256((OUTPUT / path).read_bytes()).hexdigest(), digest)

    def test_real_motion_and_continuous_loop(self):
        motion = self.report['motion']
        self.assertGreater(motion['moving_vertices'], motion['vertices_per_pose'] * .7)
        self.assertLess(motion['maximum_error'], .0002)
        self.assertLess(motion['loop_seam_distance'], .0002)

    def test_geometry_and_all_exported_keyframes_match_blender(self):
        report = json.loads((OUTPUT / 'roundtrip-report.json').read_text())
        validate_proof(report, OUTPUT / 'Data/Item/Celestial_Wings.bmd',
                       OUTPUT / 'celestial-authored-wings.blend')
        self.assertLess(report['max_geometry_error'], .000001)
        self.assertLess(report['max_pose_error'], .0002)
        self.assertGreater(report['minimum_triangle_area'], 1e-8)
        self.assertEqual(report['keyframes'], 9)
        self.assertEqual(report['unchanged_armor_files'], 5)
        self.assertGreater(report['checked_vertices'], 100000)


if __name__ == '__main__':
    unittest.main(verbosity=2)
