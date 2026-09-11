"""Pure-Python gates for the authored Zeus wings; no client or Blender startup."""
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
WING_MEMBER = 'Data/Item/Wing44.bmd'
USED_BONES = (0, 1, 2, 3, 4, 5, 24, 25, 26, 27, 28)
ATLASES = {'Zeus_Blue.jpg', 'Zeus_Platina.jpg', 'Zeus_Emissive.jpg'}
TRIANGLE_BUDGET = (12000, 20000)
ROUNDTRIP_GATE = 1e-6
POSE_GATE = 2e-4


class ZeusWingsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.model = inspect(cls.output / 'models' / 'Zeus_Wings.bmd', True)
        cls.build = json.loads((cls.output / 'wings-build-report.json').read_text())
        cls.audit = json.loads((ROOT / 'native-reference-audit.json').read_text())

    def test_exported_skeleton_preserves_the_frozen_wing44_contract(self):
        contract = self.audit['models']['wing_skeleton_reference']
        self.assertEqual(self.model['name'], 'Zeus_Wings')
        self.assertEqual(len(self.model['bones']), 47)
        self.assertEqual(self.model['action_frames'], [9])
        self.assertEqual(self.model['action_locks'], [True])
        self.assertEqual(len(self.model['action_positions'][0]), 9)
        self.assertNotEqual(self.model['sha256'], contract['sha256'])  # new geometry
        with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'Wing44.bmd'
            path.write_bytes(archive.read(WING_MEMBER))
            native = inspect(path, True)
        for index, (bone, source) in enumerate(zip(self.model['bones'], native['bones'])):
            self.assertEqual(bone.get('dummy', False), source.get('dummy', False), index)
            if bone.get('dummy'):
                continue
            self.assertEqual(bone['name'], source['name'], index)
            self.assertEqual(bone['parent'], source['parent'], index)
            self.assertEqual(len(bone['clips'][0]['positions']), 9, index)
            self.assertEqual(bone['clips'][0]['positions'], source['clips'][0]['positions'], index)
            self.assertEqual(bone['clips'][0]['rotations'], source['clips'][0]['rotations'], index)
            for channel in ('positions', 'rotations'):
                for frame in bone['clips'][0][channel]:
                    self.assertTrue(all(math.isfinite(value) for value in frame))

    def test_used_bones_follow_the_authored_arm_chain_contract(self):
        used = sorted({bone for mesh in self.model['meshes'] for bone in mesh['used_bones']})
        self.assertEqual(tuple(used), USED_BONES)
        self.assertEqual(self.build['wings']['used_bones'], list(USED_BONES))
        for mesh in self.model['meshes']:
            for bone, _ in mesh['points']:
                self.assertFalse(self.model['bones'][bone].get('dummy', False))

    def test_client_mesh_limits(self):
        self.assertEqual(Path(self.model['file']).read_bytes()[:4], b'BMD\x0a')
        self.assertLessEqual(len(self.model['meshes']), 50)
        for index, mesh in enumerate(self.model['meshes']):
            self.assertEqual(mesh['texture_index'], index)
            self.assertIn(mesh['texture'], ATLASES)
            self.assertLessEqual(mesh['triangles'], 2200)
            self.assertLessEqual(max(mesh['vertices'], len(mesh['normal_vectors']),
                                     len(mesh['texcoords'])), 10000)
            for _, point in mesh['points']:
                self.assertTrue(all(math.isfinite(value) for value in point))
            for _, nx, ny, nz, bound in mesh['normal_vectors']:
                self.assertAlmostEqual(math.sqrt(nx * nx + ny * ny + nz * nz), 1, places=5)
                self.assertTrue(all(math.isfinite(value) for value in (nx, ny, nz)))
            for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
                self.assertEqual(len(vertices), 3)
                for indices, limit in ((vertices, mesh['vertices']),
                                       (uvs, len(mesh['texcoords'])),
                                       (normals, len(mesh['normal_vectors']))):
                    self.assertTrue(all(0 <= index_ < limit for index_ in indices))

    def test_triangle_budget_holds_and_matches_the_report(self):
        self.assertEqual(self.build['triangle_budget'], list(TRIANGLE_BUDGET))
        self.assertIn('Celestial', self.build['budget_justification'])
        triangles = sum(mesh['triangles'] for mesh in self.model['meshes'])
        self.assertGreaterEqual(triangles, TRIANGLE_BUDGET[0])
        self.assertLessEqual(triangles, TRIANGLE_BUDGET[1])
        self.assertEqual(triangles, self.build['wings']['triangles'])
        self.assertEqual(self.build['wings']['topology']['nonmanifold_components'], [])
        self.assertGreater(self.build['wings']['minimum_triangle_area'], 1e-8)

    def test_flap_motion_is_real_multi_pose_and_loops(self):
        motion = self.build['wings']['motion']
        self.assertEqual(motion['proof_frames'], [0, 4, 8])
        self.assertGreater(len(motion['proof_frames']), 1)
        self.assertLess(motion['maximum_pose_error'], POSE_GATE)
        self.assertGreater(motion['moving_vertices_pose4'],
                           motion['vertices_per_pose'] * 0.3)
        self.assertLess(motion['loop_seam_distance'], POSE_GATE)
        self.assertEqual(motion['checked_vertices'],
                         motion['vertices_per_pose'] * 9)

    def test_generation_and_saved_scene_roundtrip(self):
        saved = json.loads((self.output / 'wings-saved-roundtrip-report.json').read_text())
        blend_hash = hashlib.sha256((self.output / 'zeus-wings.blend').read_bytes()).hexdigest()
        self.assertEqual(saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        self.assertEqual(saved['sha256'], self.model['sha256'])
        self.assertEqual(self.build['wings']['sha256'], self.model['sha256'])
        self.assertLessEqual(saved['max_geometry_error'], ROUNDTRIP_GATE)
        self.assertLessEqual(self.build['wings']['roundtrip_max_error'], ROUNDTRIP_GATE)
        self.assertLessEqual(saved['max_pose_error'], POSE_GATE)
        self.assertEqual(saved['keyframes'], 9)
        self.assertEqual(saved['triangles'], self.build['wings']['triangles'])

    def test_equipped_preview_and_render_evidence(self):
        equipped = self.build['wings']['equipped']
        self.assertEqual(equipped['status'], 'ATTACHMENT_MATH_PREVIEW_NOT_INGAME')
        self.assertEqual(equipped['player_sha256'],
                         self.audit['models']['owner_rig']['sha256'])
        self.assertEqual(equipped['attach_bone'], 47)
        self.assertEqual(equipped['attach_local_offset'], [0, 0, 15])
        renders = self.output / 'renders'
        expected = ['Zeus_Wings_front.png', 'Zeus_Wings_side.png', 'Zeus_Wings_back.png',
                    'Zeus_Wings_flap_rest.png', 'Zeus_Wings_flap_beat.png',
                    'Zeus_Wings_equipped_front.png']
        for render in expected:
            self.assertTrue((renders / render).exists(), render)

    def test_preview_cannot_be_mistaken_for_an_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORED_ZEUS_WINGS_PROTOTYPE_PREVIEW_ONLY')
        self.assertEqual(self.build['renderer'].startswith('Blender Cycles'), True)
        self.assertEqual(self.build['native_rig']['archive_sha256'],
                         self.audit['archive_sha256'])
        self.assertEqual(self.build['native_rig']['wing44_sha256'],
                         self.audit['models']['wing_skeleton_reference']['sha256'])
        dependencies = json.loads((self.output / 'texture-dependencies.json').read_text())
        self.assertEqual(len(dependencies), 3)
        self.assertTrue(all(item['status'] == 'AUTHORED_ATLAS_REGENERABLE'
                            for item in dependencies))
        self.assertFalse((self.output / 'Data').exists())


if __name__ == '__main__':
    unittest.main()
