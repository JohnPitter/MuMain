"""Pure-Python gates for the authored Zeus armor; no client or Blender startup."""
import hashlib
import json
import math
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / 'celestial'))
from inspect_bmd_rig import inspect

ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/'
               'celestial-ultimate-20260911/ultimate-base-downloaded.zip')
PIECES = ('Armor', 'Pant', 'Glove', 'Boot')
NATIVE_MEMBERS = {'Armor': 'Data/Player/ArmorClass304.bmd', 'Pant': 'Data/Player/PantClass304.bmd',
                  'Glove': 'Data/Player/GloveClass304.bmd', 'Boot': 'Data/Player/BootClass304.bmd'}
USED_BONES = {'Armor': (2, 3, 10, 17, 18, 25, 26, 27, 34, 35, 36),
              'Pant': (0, 2, 3, 4, 10, 11, 17),
              'Glove': (27, 28, 29, 36, 37, 38),
              'Boot': (4, 5, 11, 12)}
ATLASES = {'Zeus_Blue.jpg', 'Zeus_Platina.jpg', 'Zeus_Emissive.jpg'}
ROUNDTRIP_GATE = 1e-6


class ZeusArmorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.models = {name: inspect(cls.output / 'models' / f'Zeus_{name}.bmd', True)
                      for name in PIECES}
        cls.build = json.loads((cls.output / 'armor-build-report.json').read_text())

    def test_preview_models_with_declared_atlases_only(self):
        for name, model in self.models.items():
            self.assertEqual(model['name'], 'Zeus_' + name)
            self.assertEqual({mesh['texture'] for mesh in model['meshes']}, ATLASES)
            for forbidden in ('Celestial', 'Poseidon', 'SkinClass'):
                self.assertNotIn(forbidden, str(model['meshes']))

    def test_exported_skeleton_preserves_the_frozen_native_contract(self):
        with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory() as temp:
            natives = {}
            for name, member in NATIVE_MEMBERS.items():
                path = Path(temp) / f'{name}.bmd'
                path.write_bytes(archive.read(member))
                natives[name] = inspect(path, True)
        for name, model in self.models.items():
            native = natives[name]
            self.assertEqual(len(model['bones']), 51)
            self.assertEqual(model['action_frames'], [1])
            self.assertNotEqual(model['sha256'], native['sha256'])  # new geometry, not a reexport
            for index, (bone, source) in enumerate(zip(model['bones'], native['bones'])):
                self.assertEqual(bone.get('dummy', False), source.get('dummy', False), index)
                if bone.get('dummy'):
                    continue
                self.assertEqual(bone['name'], source['name'], index)
                self.assertEqual(bone['parent'], source['parent'], index)
                self.assertEqual(bone['clips'][0]['positions'], source['clips'][0]['positions'], index)
                self.assertEqual(bone['clips'][0]['rotations'], source['clips'][0]['rotations'], index)

    def test_used_bones_follow_the_audited_native_distribution(self):
        for name, model in self.models.items():
            used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
            self.assertEqual(tuple(used), USED_BONES[name])
            for mesh in model['meshes']:
                for bone, _ in mesh['points']:
                    self.assertFalse(model['bones'][bone].get('dummy', False))

    def test_client_mesh_limits(self):
        for model in self.models.values():
            self.assertEqual(Path(model['file']).read_bytes()[:4], b'BMD\x0a')
            self.assertLessEqual(len(model['meshes']), 50)
            for index, mesh in enumerate(model['meshes']):
                self.assertEqual(mesh['texture_index'], index)
                self.assertLessEqual(mesh['triangles'], 2200)
                self.assertLessEqual(max(mesh['vertices'], len(mesh['normal_vectors']),
                                         len(mesh['texcoords'])), 10000)

    def test_finite_vertices_unit_normals_and_indices_in_range(self):
        for model in self.models.values():
            for mesh in model['meshes']:
                for _, point in mesh['points']:
                    self.assertTrue(all(math.isfinite(value) for value in point))
                for _, nx, ny, nz, bound in mesh['normal_vectors']:
                    self.assertAlmostEqual(math.sqrt(nx * nx + ny * ny + nz * nz), 1, places=5)
                    self.assertTrue(all(math.isfinite(value) for value in (nx, ny, nz)))
                for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
                    self.assertEqual(len(vertices), 3)
                    for indices, limit in ((vertices, mesh['vertices']), (uvs, len(mesh['texcoords'])),
                                           (normals, len(mesh['normal_vectors']))):
                        self.assertTrue(all(0 <= index < limit for index in indices))

    def test_triangle_budgets_hold_and_match_report(self):
        self.assertEqual(self.build['triangle_budgets'],
                         {'Armor': [6280, 9400], 'Pant': [4500, 6740],
                          'Glove': [2400, 3610], 'Boot': [3350, 5030]})
        self.assertIn('+/-20%', self.build['budget_justification'])
        for name, model in self.models.items():
            triangles = sum(mesh['triangles'] for mesh in model['meshes'])
            low, high = self.build['triangle_budgets'][name]
            self.assertGreaterEqual(triangles, low, name)
            self.assertLessEqual(triangles, high, name)
            self.assertEqual(triangles, self.build['pieces'][name]['triangles'])
            self.assertEqual(self.build['pieces'][name]['used_bones'], list(USED_BONES[name]))

    def test_generation_and_saved_scene_roundtrip(self):
        saved = json.loads((self.output / 'armor-saved-roundtrip-report.json').read_text())
        blend_hash = hashlib.sha256((self.output / 'zeus-armor.blend').read_bytes()).hexdigest()
        self.assertEqual(saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        for name, model in self.models.items():
            # Observed band: weapons measured 5.4e-7..6.9e-7; skinned pieces
            # 4.2e-7..8.9e-7 (bone-local float32 spacing adds up to half an ulp).
            self.assertLessEqual(saved['models'][name]['max_error'], ROUNDTRIP_GATE)
            self.assertEqual(saved['models'][name]['sha256'], model['sha256'])
            self.assertEqual(self.build['pieces'][name]['sha256'], model['sha256'])
            self.assertEqual(self.build['pieces'][name]['topology']['nonmanifold_components'], [])
            self.assertGreater(self.build['pieces'][name]['minimum_triangle_area'], 1e-8)

    def test_preview_cannot_be_mistaken_for_an_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORED_ZEUS_ARMOR_PROTOTYPE_PREVIEW_ONLY')
        self.assertEqual(self.build['renderer'].startswith('Blender Cycles'), True)
        self.assertEqual(self.build['native_rig']['archive_sha256'],
                         json.loads((ROOT / 'native-reference-audit.json').read_text())['archive_sha256'])
        self.assertEqual(self.build['native_rig']['used_bone_contract'],
                         {name: list(bones) for name, bones in USED_BONES.items()})
        dependencies = json.loads((self.output / 'texture-dependencies.json').read_text())
        self.assertEqual(len(dependencies), 3)
        self.assertTrue(all(item['status'] == 'AUTHORED_ATLAS_REGENERABLE' for item in dependencies))
        self.assertFalse((self.output / 'Data').exists())

    def test_equipped_preview_follows_the_player_rig(self):
        equipped = json.loads((self.output / 'armor-equipped-review-report.json').read_text())
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        self.assertEqual(equipped['status'], 'ATTACHMENT_MATH_PREVIEW_NOT_INGAME')
        self.assertEqual(equipped['player_sha256'], audit['models']['owner_rig']['sha256'])
        self.assertEqual(len(equipped['poses']), 2)
        for probe in equipped['fit_probes']:
            for piece, distances in probe['vertex_anchor_distance'].items():
                self.assertLess(distances['nearest'], equipped['anchor_gates']['nearest_below'], piece)
                self.assertLess(distances['farthest'], equipped['anchor_gates']['farthest_below'], piece)
        for piece, shift in equipped['anchor_gates']['pose_shifts'].items():
            self.assertGreaterEqual(shift, equipped['anchor_gates']['pose_shift_min'], piece)
        renders = self.output / 'renders'
        expected = [f'Zeus_{piece}_{view}.png' for piece in PIECES for view in ('front', 'side', 'back')]
        expected += ['Zeus_ArmorSet_equipped_front.png', 'Zeus_ArmorSet_equipped_side.png']
        for render in expected:
            self.assertTrue((renders / render).exists(), render)


if __name__ == '__main__':
    unittest.main()
