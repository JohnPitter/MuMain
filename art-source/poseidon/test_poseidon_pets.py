"""Pure-Python gates for the authored Poseidon mount and eagle; no Blender."""
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
# pet key -> (model name, audit key, frozen member, action frames, report stem)
PETS = {
    'Mount': ('Poseidon_Black_Mount', 'dark_horse', 'Data/Skill/DarkHorse.bmd',
              (7, 7, 6, 10, 41, 51, 31), 'mount'),
    'Eagle': ('Poseidon_Imperial_Eagle', 'dark_spirit', 'Data/Skill/darkspirit.bmd',
              (6, 4, 4, 4), 'eagle'),
}
# Measured contracts (see poseidon_mount_rig.py / poseidon_eagle_rig.py).
USED_BONES = {
    'Mount': (17, 18, 19, 20, 21, 22, 23, 28, 31, 32, 33, 34, 37, 38, 39, 40,
              42, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58),
    'Eagle': (2, 3, 4, 5, 6, 10, 11, 12, 14, 15, 18, 19, 22, 26, 30, 34, 35,
              36, 37, 38, 39, 41, 42, 45, 46, 49, 53, 57, 61, 62, 63, 64, 65,
              66, 67, 69, 70, 71, 72, 74, 75),
}
ATLASES = {'Poseidon_Black.jpg', 'Poseidon_Gold.jpg', 'Poseidon_Blue.jpg'}
ROUNDTRIP_GATE = 1e-6


class PoseidonPetsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.models = {pet: inspect(cls.output / 'models' / f'{name}.bmd', True)
                      for pet, (name, *_ ) in PETS.items()}
        cls.builds = {pet: json.loads((cls.output / f'{stem}-build-report.json').read_text())
                      for pet, (*_, stem) in PETS.items()}

    def test_preview_models_with_declared_atlases_only(self):
        for pet, (name, *_ ) in PETS.items():
            model = self.models[pet]
            self.assertEqual(model['name'], name)
            self.assertEqual({mesh['texture'] for mesh in model['meshes']}, ATLASES)
            self.assertNotIn('Celestial', str(model['meshes']))
            self.assertNotIn('dkhorse', str(model['meshes']))
            self.assertNotIn('dkonebody', str(model['meshes']))

    def test_exported_skeleton_preserves_the_full_native_multiaction_contract(self):
        """Bones, clips of every action, locks and roots match the frozen ZIP."""
        with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory() as temp:
            natives = {}
            for pet, (_, _, member, *_ ) in PETS.items():
                path = Path(temp) / f'{pet}.bmd'
                path.write_bytes(archive.read(member))
                natives[pet] = inspect(path, True)
        for pet, (name, _, _, frames, *_ ) in PETS.items():
            model, native = self.models[pet], natives[pet]
            self.assertEqual(len(model['bones']), 60 if pet == 'Mount' else 77)
            self.assertEqual(tuple(model['action_frames']), frames)
            self.assertNotEqual(model['sha256'], native['sha256'])  # new geometry, not a reexport
            self.assertEqual(model['bones'], native['bones'])       # bit-for-bit skeleton
            self.assertEqual(model['action_frames'], native['action_frames'])
            self.assertEqual(model['action_locks'], native['action_locks'])
            self.assertEqual(model['action_positions'], native['action_positions'])

    def test_used_bones_follow_the_declared_native_contract(self):
        for pet, model in self.models.items():
            used = sorted({bone for mesh in model['meshes'] for bone in mesh['used_bones']})
            self.assertEqual(tuple(used), USED_BONES[pet])
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
        budgets = {'Mount': (15000, 30000), 'Eagle': (3000, 6000)}
        for pet, (name, *_ ) in PETS.items():
            build = self.builds[pet]
            model = self.models[pet]
            triangles = sum(mesh['triangles'] for mesh in model['meshes'])
            low, high = budgets[pet]
            self.assertGreaterEqual(triangles, low, pet)
            self.assertLessEqual(triangles, high, pet)
            self.assertEqual(triangles, build['pieces'][name]['triangles'])
            self.assertIn('budget_justification', build)
            reported = build['pieces'][name]['part_triangles']
            self.assertEqual(sum(reported.values()), triangles)
            self.assertEqual(build['pieces'][name]['used_bones'], list(USED_BONES[pet]))

    def test_generation_and_saved_scene_roundtrip(self):
        for pet, (name, *_ ) in PETS.items():
            build = self.builds[pet]
            saved = json.loads(
                (self.output / ('mount-saved-roundtrip-report.json' if pet == 'Mount'
                                else 'eagle-saved-roundtrip-report.json')).read_text())
            blend = self.output / ('poseidon-mount.blend' if pet == 'Mount'
                                   else 'poseidon-eagle.blend')
            blend_hash = hashlib.sha256(blend.read_bytes()).hexdigest()
            self.assertEqual(saved['saved_blend_sha256'], blend_hash)
            self.assertEqual(build['blend_sha256'], blend_hash)
            record = saved['models'][name]
            # Observed band: equipment lots 5.9e-7..8.9e-7; pets measure
            # 6.5e-7..7.8e-7 - bone-local float32 spacing again.
            self.assertLessEqual(record['max_error'], ROUNDTRIP_GATE)
            self.assertEqual(record['sha256'], self.models[pet]['sha256'])
            self.assertEqual(build['pieces'][name]['sha256'], self.models[pet]['sha256'])
            self.assertEqual(build['pieces'][name]['topology']['nonmanifold_components'], [])
            self.assertGreater(build['pieces'][name]['minimum_triangle_area'], 1e-8)

    def test_preview_cannot_be_mistaken_for_an_installable_package(self):
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        member_keys = {'Mount': 'dark_horse', 'Eagle': 'dark_spirit'}
        for pet, build in self.builds.items():
            self.assertEqual(build['status'], f'AUTHORED_{pet.upper()}_PROTOTYPE_PREVIEW_ONLY')
            self.assertTrue(build['renderer'].startswith('Blender Cycles'))
            self.assertEqual(build['native_rig']['archive_sha256'], audit['archive_sha256'])
            self.assertEqual(build['native_rig']['model_sha256'],
                             audit['models'][member_keys[pet]]['sha256'])
        self.assertFalse((self.output / 'Data').exists())

    def test_action_proofs_cover_native_clips_and_render(self):
        expected_proofs = {'Mount': (('stand', 0), ('gallop', 1), ('earthshake', 3)),
                           'Eagle': (('fly', 0), ('flying', 1), ('escape', 3))}
        for pet, (name, *_ ) in PETS.items():
            report = json.loads((self.output / ('mount-actions-report.json' if pet == 'Mount'
                                                else 'eagle-actions-report.json')).read_text())
            self.assertEqual(report['status'], 'ACTION_MATH_PREVIEW_NOT_INGAME')
            self.assertEqual(report['model_sha256'], self.models[pet]['sha256'])
            self.assertGreaterEqual(len(report['proofs']), 2)
            proven = {(proof['proof'], proof['action']) for proof in report['proofs']}
            for proof_name, action in expected_proofs[pet]:
                self.assertIn((proof_name, action), proven)
            for proof in report['proofs']:
                self.assertLessEqual(proof['maximum_error'], report['pose_tolerance'])
                self.assertGreater(proof['checked_vertices'], 0)
            for render in (render for proof in report['proofs'] for render in proof['renders']):
                self.assertTrue((self.output / 'renders' / render).exists(), render)

    def test_design_decisions_are_documented(self):
        mount, eagle = self.builds['Mount'], self.builds['Eagle']
        self.assertIn('mounted_lance', mount['design_decisions'])
        self.assertIn('not modeled', mount['design_decisions']['mounted_lance'])
        self.assertIn('DarkSpirit', eagle['design_decisions']['contract'])
        self.assertIn('effect-bone', eagle['design_decisions']['effect_bones'])
        # The mount action table is the GOBoid branch, the eagle one the pet system.
        self.assertIn('GOBoid.cpp', mount['native_rig']['action_evidence'])
        self.assertIn('CSPetSystem.cpp', eagle['native_rig']['action_evidence'])

    def test_inspection_renders_exist(self):
        renders = self.output / 'renders'
        for pet, (name, *_ ) in PETS.items():
            views = ('front', 'side', 'back', 'detail_head')
            views += ('detail_saddle',) if pet == 'Mount' else ('detail_wing',)
            for view in views:
                self.assertTrue((renders / f'{name}_{view}.png').exists(), f'{name}_{view}')


if __name__ == '__main__':
    unittest.main()
