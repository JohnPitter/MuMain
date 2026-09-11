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

CONCEPT_SHA256 = '7d5bd6e0370f792f06faa4048987433e163916ff3992e94762399c76b249d573'
ROUNDTRIP_GATE = 1e-6


class ZeusPrototypeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.models = {name: inspect(cls.output / 'models' / f'Zeus_{name}.bmd', True)
                      for name in ('Sword', 'Staff')}
        cls.build = json.loads((cls.output / 'build-report.json').read_text())

    def test_original_names_and_declared_atlases(self):
        textures = {'Zeus_Blue.jpg', 'Zeus_Platina.jpg', 'Zeus_Emissive.jpg'}
        for name, model in self.models.items():
            self.assertEqual(model['name'], 'Zeus_' + name)
            self.assertEqual({mesh['texture'] for mesh in model['meshes']}, textures)
            for forbidden in ('Celestial', 'Poseidon', 'SkinClass'):
                self.assertNotIn(forbidden, str(model['meshes']))

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
        expected_cores = {'Sword': (64,), 'Staff': (152,)}
        expected_tips = {'Sword': (0, -152, 0), 'Staff': (0, -178, 0)}
        for name, model in self.models.items():
            bones = model['bones']
            self.assertEqual([b['name'] for b in bones],
                             [f'Zeus_{name}_{label}' for label in ('Grip', 'Core', 'Tip')])
            self.assertEqual(bones[0]['clips'][0]['first_position'], (0, 0, 0))
            # Core rides the hand-local -Y channel; x=2.2 arrives as float32.
            core = bones[1]['clips'][0]['first_position']
            self.assertAlmostEqual(core[0], 2.2, places=5)
            self.assertEqual(core[1:], (-expected_cores[name][0], 0))
            self.assertEqual(bones[2]['clips'][0]['first_position'], expected_tips[name])

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

    def test_volumetric_distinct_silhouettes_and_budgets(self):
        heights = {}
        for name, model in self.models.items():
            points = [point for mesh in model['meshes'] for _, point in mesh['points']]
            extents = [max(p[i] for p in points) - min(p[i] for p in points) for i in range(3)]
            self.assertGreater(extents[0], 5)
            self.assertGreater(extents[2], 40)
            self.assertGreater(extents[1], 190)
            heights[name] = extents[1]
            triangles = sum(m['triangles'] for m in model['meshes'])
            # Budget justified in design-spec: same isometric readability band as
            # the Poseidon weapons (6866/6938); client caps are tested separately.
            self.assertGreaterEqual(triangles, 6000)
            self.assertLessEqual(triangles, 8000)
        self.assertGreater(heights['Staff'], heights['Sword'] + 20)

    def test_generation_and_saved_scene_roundtrip(self):
        saved = json.loads((self.output / 'saved-roundtrip-report.json').read_text())
        blend_hash = hashlib.sha256((self.output / 'zeus-weapons.blend').read_bytes()).hexdigest()
        self.assertEqual(saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        for name, model in self.models.items():
            self.assertEqual(self.build['models'][name]['sha256'], model['sha256'])
            self.assertEqual(saved['models'][name]['sha256'], model['sha256'])
            self.assertLessEqual(self.build['models'][name]['roundtrip_max_error'], ROUNDTRIP_GATE)
            self.assertLessEqual(saved['models'][name]['max_error'], ROUNDTRIP_GATE)
            self.assertEqual(self.build['models'][name]['topology']['nonmanifold_components'], [])

    def test_cannot_be_mistaken_for_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORING_PROTOTYPE_ONLY')
        dependencies = json.loads((self.output / 'texture-dependencies.json').read_text())
        self.assertEqual({d['texture'] for d in dependencies},
                         {'Zeus_Blue.jpg', 'Zeus_Platina.jpg', 'Zeus_Emissive.jpg'})
        self.assertTrue(all(d['status'] == 'MISSING_ATLAS_NOT_FOR_CLIENT' for d in dependencies))
        self.assertFalse((self.output / 'Data').exists())

    def test_render_evidence_covers_every_view(self):
        renders = {path.name for path in (self.output / 'renders').glob('*.png')}
        for name in ('Sword', 'Staff'):
            for view in ('front', 'side', 'detail'):
                self.assertIn(f'Zeus_{name}_{view}.png', renders)

    def test_native_rig_evidence_backs_the_duel_master_contract(self):
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        self.assertEqual(audit['concept_sha256'], CONCEPT_SHA256)
        models = audit['models']
        self.assertEqual(len(models['owner_rig']['bones']), 60)
        self.assertEqual(len(models['owner_rig']['action_frames']), 284)
        for piece in ('helm', 'armor', 'pants', 'gloves', 'boots'):
            model = models[f'duel_master_{piece}']
            self.assertTrue(model['member'].endswith('Class304.bmd'))
            self.assertEqual(len(model['bones']), 51)
            self.assertEqual(model['action_frames'], [1])
            self.assertEqual(model['textures'], ['SkinClass304.jpg'])
        self.assertEqual(len(models['wing_skeleton_reference']['bones']), 47)
        self.assertLess(len(models['native_sword_orientation_only']['bones']), 4)
        self.assertLess(len(models['native_staff_orientation_only']['bones']), 5)
        self.assertEqual(len(models['native_pendant_reference']['bones']), 1)
        self.assertEqual(len(models['native_ring_reference']['bones']), 1)
        mapping = audit['sources']['src/source/Character/CharacterManager.cpp']['matches']
        self.assertTrue(any('return CLASS_DARK;' in hit['text'] for hits in
                            mapping.values() for hit in hits))


if __name__ == '__main__':
    unittest.main()
