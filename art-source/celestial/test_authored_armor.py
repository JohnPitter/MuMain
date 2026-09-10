"""Validate native armor contracts; these tests cannot certify artistic fidelity."""
import hashlib
import json
import math
import unittest
from pathlib import Path

from inspect_bmd_rig import inspect
from artifact_proof import validate_proof

ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / 'authored-armor'
NAMES = ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots')
EXPECTED_BONES = {
    'Helm': {20}, 'Armor': {2, 17, 18, 26, 35},
    'Pants': {2, 3, 4, 10, 11, 44, 45, 46, 48, 49, 50},
    'Gloves': {27, 28, 29, 30, 31, 36, 37, 38, 39, 40}, 'Boots': {4, 5, 11, 12},
}


class AuthoredArmorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.models = {name: inspect(OUTPUT / 'Data' / 'Player' / f'Celestial_{name}.bmd', True)
                      for name in NAMES}

    def test_five_separate_models_with_native_joint_indices(self):
        for name, model in self.models.items():
            with self.subTest(name=name):
                self.assertEqual(model['name'], 'Celestial_' + name)
                self.assertEqual(len(model['bones']), 51)
                self.assertEqual(model['action_frames'], [1])
                actual = {bone for mesh in model['meshes'] for bone in mesh['used_bones']}
                self.assertEqual(actual, EXPECTED_BONES[name])

    def test_every_normal_and_face_points_to_valid_data(self):
        for model in self.models.values():
            self.assertLessEqual(len(model['meshes']), 50)
            for mesh in model['meshes']:
                self.assertTrue(0 < mesh['vertices'] <= 10000)
                for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
                    for vertex, uv, normal in zip(vertices, uvs, normals):
                        self.assertTrue(0 <= vertex < len(mesh['points']))
                        self.assertTrue(0 <= uv < len(mesh['texcoords']))
                        self.assertTrue(0 <= normal < len(mesh['normal_vectors']))
                        bone, *values, bound_vertex = mesh['normal_vectors'][normal]
                        self.assertEqual(bound_vertex, vertex)
                        self.assertEqual(bone, mesh['points'][vertex][0])
                        self.assertAlmostEqual(sum(x*x for x in values), 1, places=4)

    def test_no_dummy_bone_influence_or_non_finite_geometry(self):
        for model in self.models.values():
            for mesh in model['meshes']:
                for bone, point in mesh['points']:
                    self.assertTrue(0 <= bone < len(model['bones']))
                    self.assertFalse(model['bones'][bone].get('dummy', False))
                    self.assertTrue(all(math.isfinite(v) for v in point))

    def test_all_skin_skeletons_match(self):
        baseline = self.models['Helm']['bones']
        for model in self.models.values():
            self.assertEqual(baseline, model['bones'])
        self.assertEqual(baseline[20]['name'], 'Bip01 Head')
        self.assertEqual(baseline[28]['name'], 'Bip01 R Hand')
        self.assertEqual(baseline[37]['name'], 'Bip01 L Hand')

    def test_only_celestial_textures_resolve_in_player_folder(self):
        permitted = {f'Celestial_{color}.jpg' for color in ('Gold', 'Ivory', 'Sapphire')}
        for model in self.models.values():
            for mesh in model['meshes']:
                self.assertIn(mesh['texture'], permitted)
                path = OUTPUT / 'Data' / 'Player' / Path(mesh['texture']).with_suffix('.OZJ')
                self.assertEqual(path.read_bytes()[24:], (ROOT / 'textures' / mesh['texture']).read_bytes())

    def test_all_required_player_clips_were_measured(self):
        report = json.loads((OUTPUT / 'armor-report.json').read_text())
        expected = {'PLAYER_STOP_SWORD': 4, 'PLAYER_WALK_SWORD': 17, 'PLAYER_RUN_SWORD': 26,
                    'PLAYER_SKILL_HAND1': 146, 'PLAYER_DIE1': 237, 'PLAYER_STOP_FLY': 11,
                    'PLAYER_FLY': 34, 'PLAYER_ATTACK_SWORD_RIGHT1': 39}
        self.assertEqual({clip['name']: clip['action'] for clip in report['clips']}, expected)
        for clip in report['clips']:
            self.assertGreater(clip['checked_vertices'], 50000)
            self.assertLess(clip['maximum_error'], .0002)

    def test_roundtrip_and_manifest(self):
        manifest = json.loads((OUTPUT / 'sha256.json').read_text())
        self.assertEqual(len(manifest), 8)
        for path, digest in manifest.items():
            self.assertEqual(hashlib.sha256((OUTPUT / path).read_bytes()).hexdigest(), digest)
        report = json.loads((OUTPUT / 'roundtrip-report.json').read_text())
        self.assertEqual(set(report), set(NAMES))
        self.assertEqual(report['Boots']['closed_sabatons'], 2)
        for name, value in report.items():
            validate_proof(value, OUTPUT / 'Data/Player' / f'Celestial_{name}.bmd',
                           OUTPUT / 'celestial-authored-armor.blend')
            self.assertLess(value['max_error'], .00002)
            self.assertGreater(value['minimum_triangle_area'], 1e-8)


if __name__ == '__main__':
    unittest.main(verbosity=2)
