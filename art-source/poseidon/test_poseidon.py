"""Pure-Python gates for generated prototype assets; no client or Blender startup."""
import hashlib
import json
import math
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / 'celestial'))
from inspect_bmd_rig import inspect


class PoseidonPrototypeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.models = {name: inspect(cls.output / 'models' / f'Poseidon_{name}.bmd', True)
                      for name in ('Trident', 'Scepter')}
        cls.build = json.loads((cls.output / 'build-report.json').read_text())

    def test_original_names_and_declared_atlases(self):
        textures = {'Poseidon_Black.jpg', 'Poseidon_Blue.jpg', 'Poseidon_Gold.jpg'}
        for name, model in self.models.items():
            self.assertEqual(model['name'], 'Poseidon_' + name)
            self.assertEqual({mesh['texture'] for mesh in model['meshes']}, textures)
            self.assertNotIn('Celestial', str(model['meshes']))

    def test_runtime_mesh_limits(self):
        for model in self.models.values():
            self.assertEqual(Path(model['file']).read_bytes()[:4], b'BMD\x0a')
            self.assertLessEqual(len(model['meshes']), 50)
            for index, mesh in enumerate(model['meshes']):
                self.assertEqual(mesh['texture_index'], index)
                self.assertLessEqual(mesh['triangles'], 2200)
                self.assertLessEqual(max(mesh['vertices'], len(mesh['normal_vectors']),
                                         len(mesh['texcoords'])), 10000)

    def test_single_rigid_owner_and_static_action(self):
        for model in self.models.values():
            self.assertEqual(model['action_frames'], [1])
            self.assertEqual(model['action_locks'], [False])
            self.assertEqual([bone['parent'] for bone in model['bones']], [-1, 0, 0])
            for mesh in model['meshes']:
                self.assertEqual(mesh['used_bones'], [0])

    def test_anchors_preserve_grip_core_and_tip(self):
        for name, model in self.models.items():
            bones = model['bones']
            self.assertEqual([b['name'] for b in bones],
                             [f'Poseidon_{name}_{label}' for label in ('Grip', 'Core', 'Tip')])
            self.assertEqual(bones[0]['clips'][0]['first_position'], (0, 0, 0))
            self.assertAlmostEqual(bones[1]['clips'][0]['first_position'][0], 2.2, places=5)
            self.assertEqual(bones[1]['clips'][0]['first_position'][1:], (-70, 0))
            self.assertEqual(bones[2]['clips'][0]['first_position'],
                             (0, -136 if name == 'Trident' else -110, 0))

    def test_finite_vertices_and_unit_normals(self):
        for model in self.models.values():
            for mesh in model['meshes']:
                for _, point in mesh['points']:
                    self.assertTrue(all(math.isfinite(value) for value in point))
                for _, nx, ny, nz, _ in mesh['normal_vectors']:
                    self.assertAlmostEqual(math.sqrt(nx * nx + ny * ny + nz * nz), 1, places=5)
                self.assertTrue(all(math.isfinite(v) and -1e-5 <= v <= 1.00001
                                    for uv in mesh['texcoords'] for v in uv))

    def test_triangle_indices_are_in_range(self):
        for model in self.models.values():
            for mesh in model['meshes']:
                for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
                    self.assertEqual(len(vertices), 3)
                    for indices, limit in ((vertices, mesh['vertices']), (uvs, len(mesh['texcoords'])),
                                           (normals, len(mesh['normal_vectors']))):
                        self.assertTrue(all(0 <= index < limit for index in indices))

    def test_volumetric_distinct_silhouettes(self):
        heights = {}
        for name, model in self.models.items():
            points = [point for mesh in model['meshes'] for _, point in mesh['points']]
            extents = [max(p[i] for p in points) - min(p[i] for p in points) for i in range(3)]
            self.assertGreater(extents[0], 5)
            self.assertGreater(extents[2], 40)
            self.assertGreater(extents[1], 190)
            heights[name] = extents[1]
            self.assertGreater(sum(m['triangles'] for m in model['meshes']), 5000)
        self.assertGreater(heights['Trident'], heights['Scepter'] + 20)

    def test_generation_and_saved_scene_roundtrip(self):
        saved = json.loads((self.output / 'saved-roundtrip-report.json').read_text())
        blend_hash = hashlib.sha256((self.output / 'poseidon-weapons.blend').read_bytes()).hexdigest()
        self.assertEqual(saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        for name, model in self.models.items():
            self.assertEqual(self.build['models'][name]['sha256'], model['sha256'])
            self.assertEqual(saved['models'][name]['sha256'], model['sha256'])
            self.assertLessEqual(saved['models'][name]['max_error'], 2e-5)
            self.assertEqual(self.build['models'][name]['topology']['nonmanifold_components'], [])

    def test_cannot_be_mistaken_for_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORING_PROTOTYPE_ONLY')
        dependencies = json.loads((self.output / 'texture-dependencies.json').read_text())
        self.assertEqual(len(dependencies), 4)  # Black, Gold, Blue + Pearl (cape ferragem)
        self.assertTrue(all(d['status'] == 'AUTHORED_ATLAS_REGENERABLE' for d in dependencies))
        self.assertFalse((self.output / 'Data').exists())

    def test_textured_inspection_renders_exist(self):
        renders = self.output / 'renders'
        for name in self.models:
            for view in ('front', 'side', 'back'):
                self.assertTrue((renders / f'Poseidon_{name}_{view}.png').exists(),
                                f'Poseidon_{name}_{view}.png')

    def test_native_rig_evidence_keeps_pet_and_owner_separate(self):
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        models = audit['models']
        self.assertEqual(len(models['owner_rig']['bones']), 60)
        self.assertEqual(len(models['owner_rig']['action_frames']), 284)
        self.assertEqual((len(models['dark_horse']['bones']), len(models['dark_horse']['action_frames'])), (60, 7))
        self.assertEqual((len(models['dark_spirit']['bones']), len(models['dark_spirit']['action_frames'])), (77, 4))
        self.assertEqual(len(models['emperor_cape']['bones']), 1)


if __name__ == '__main__':
    unittest.main()
