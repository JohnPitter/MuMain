"""Pure-Python gates for the authored Zeus jewels; no client or Blender startup."""
import hashlib
import json
import math
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / 'celestial'))
from inspect_bmd_rig import inspect

JEWELS = ('Pendant', 'Ring_Storm', 'Ring_Wisdom')
CONTRACT_BONES = {'Pendant': 'Tube04', 'Ring_Storm': 'Tube01', 'Ring_Wisdom': 'Tube01'}
NATIVE_KEYS = {'Pendant': 'native_pendant_reference', 'Ring_Storm': 'native_ring_reference',
               'Ring_Wisdom': 'native_ring_reference'}
ATLASES = {'Zeus_Blue.jpg', 'Zeus_Platina.jpg', 'Zeus_Emissive.jpg'}
TRIANGLE_BUDGETS = {'Pendant': (1700, 3200), 'Ring_Storm': (1600, 2800),
                    'Ring_Wisdom': (1600, 2800)}
ROUNDTRIP_GATE = 1e-6


class ZeusJewelsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.models = {name: inspect(cls.output / 'models' / f'Zeus_{name}.bmd', True)
                      for name in JEWELS}
        cls.build = json.loads((cls.output / 'jewels-build-report.json').read_text())
        cls.saved = json.loads((cls.output / 'jewels-saved-roundtrip-report.json').read_text())
        cls.audit = json.loads((ROOT / 'native-reference-audit.json').read_text())

    def test_jewel_contracts_preserve_native_names_and_static_action(self):
        for name, model in self.models.items():
            native = self.audit['models'][NATIVE_KEYS[name]]
            self.assertEqual(model['name'], 'Zeus_' + name)
            self.assertEqual(len(model['bones']), 1)
            self.assertEqual(model['bones'][0]['name'], CONTRACT_BONES[name])
            self.assertEqual(model['bones'][0]['name'], native['bones'][0]['name'])
            self.assertEqual(model['bones'][0]['parent'], -1)
            self.assertEqual(model['bones'][0]['clips'][0]['first_position'], (0.0, 0.0, 0.0))
            self.assertEqual(model['action_frames'], [1])
            self.assertEqual(model['action_locks'], [False])
            self.assertNotEqual(model['sha256'], native['sha256'])  # new geometry

    def test_single_rigid_root_binding(self):
        for name, model in self.models.items():
            for mesh in model['meshes']:
                self.assertEqual(mesh['used_bones'], [0])
                for bone, _ in mesh['points']:
                    self.assertEqual(bone, 0)

    def test_client_mesh_limits_and_finite_data(self):
        for name, model in self.models.items():
            self.assertEqual(Path(model['file']).read_bytes()[:4], b'BMD\x0a')
            self.assertLessEqual(len(model['meshes']), 50)
            for index, mesh in enumerate(model['meshes']):
                self.assertEqual(mesh['texture_index'], index)
                self.assertIn(mesh['texture'], ATLASES)
                self.assertLessEqual(mesh['triangles'], 2200)
                self.assertLessEqual(max(mesh['vertices'], len(mesh['normal_vectors']),
                                         len(mesh['texcoords'])), 10000)
                for _, point in mesh['points']:
                    self.assertTrue(all(math.isfinite(value) for value in point))
                for _, nx, ny, nz, _ in mesh['normal_vectors']:
                    self.assertAlmostEqual(math.sqrt(nx * nx + ny * ny + nz * nz), 1,
                                           places=5)
                for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
                    self.assertEqual(len(vertices), 3)
                    for indices, limit in ((vertices, mesh['vertices']),
                                           (uvs, len(mesh['texcoords'])),
                                           (normals, len(mesh['normal_vectors']))):
                        self.assertTrue(all(0 <= value < limit for value in indices))
                for uv in mesh['texcoords']:
                    self.assertTrue(all(math.isfinite(value) for value in uv))

    def test_triangle_budgets_hold_and_silhouettes_are_distinct(self):
        self.assertEqual(self.build['triangle_budgets'],
                         {name: list(budget) for name, budget in TRIANGLE_BUDGETS.items()})
        self.assertIn('+/-20%', self.build['budget_justification'])
        for name, model in self.models.items():
            triangles = sum(mesh['triangles'] for mesh in model['meshes'])
            low, high = TRIANGLE_BUDGETS[name]
            self.assertGreaterEqual(triangles, low, name)
            self.assertLessEqual(triangles, high, name)
            self.assertEqual(triangles, self.build['jewels'][name]['triangles'])
            self.assertEqual(self.build['jewels'][name]['topology']
                             ['nonmanifold_components'], [])
            self.assertGreater(self.build['jewels'][name]['minimum_triangle_area'], 1e-8)
        storm = self.build['jewels']['Ring_Storm']['extents']
        wisdom = self.build['jewels']['Ring_Wisdom']['extents']
        pendant = self.build['jewels']['Pendant']['extents']
        # Storm = tall bolt spike, Wisdom = wide crown halo, pendant hangs tall.
        self.assertGreater(wisdom[0], storm[0] + 4)
        self.assertGreater(storm[2], wisdom[2] + 1)
        self.assertGreater(pendant[2], pendant[0])

    def test_declared_anchors_sit_on_the_exported_surface(self):
        for name in JEWELS:
            anchors = self.saved['models'][name]['anchors']
            self.assertEqual(set(anchors), set(self.build['jewels'][name]['anchors']))
            for label, distance in anchors.items():
                gate = 2.5 if label in ('Band', 'ChainTop', 'Frame') else 6.0
                self.assertLess(distance, gate, f'{name}/{label}')

    def test_generation_and_saved_scene_roundtrip(self):
        blend_hash = hashlib.sha256(
            (self.output / 'zeus-jewels.blend').read_bytes()).hexdigest()
        self.assertEqual(self.saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        for name, model in self.models.items():
            self.assertEqual(self.build['jewels'][name]['sha256'], model['sha256'])
            self.assertEqual(self.saved['models'][name]['sha256'], model['sha256'])
            self.assertLessEqual(self.build['jewels'][name]['roundtrip_max_error'],
                                 ROUNDTRIP_GATE)
            self.assertLessEqual(self.saved['models'][name]['max_error'], ROUNDTRIP_GATE)

    def test_preview_cannot_be_mistaken_for_an_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORED_ZEUS_JEWELS_PROTOTYPE_PREVIEW_ONLY')
        audit_sha = hashlib.sha256((ROOT / 'native-reference-audit.json').read_bytes()).hexdigest()
        self.assertEqual(self.build['native_contract']['audit_sha256'], audit_sha)
        dependencies = json.loads((self.output / 'texture-dependencies.json').read_text())
        self.assertEqual(len(dependencies), 3)
        self.assertTrue(all(item['status'] == 'AUTHORED_ATLAS_REGENERABLE'
                            for item in dependencies))
        self.assertFalse((self.output / 'Data').exists())
        renders = self.output / 'renders'
        for name in JEWELS:
            for view in ('front', 'side', 'detail'):
                self.assertTrue((renders / f'Zeus_{name}_{view}.png').exists(),
                                f'Zeus_{name}_{view}.png')


if __name__ == '__main__':
    unittest.main()
