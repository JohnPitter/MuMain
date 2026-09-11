"""Pure-Python gates for the authored Poseidon cape; no client or Blender startup."""
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
CAPE_MEMBER = 'Data/Item/DarkLordRobe02.bmd'
PLAYER_MEMBER = 'Data/Player/player.bmd'
AUDIT_KEY = 'emperor_cape'
LINK_BONE = 19
ATLASES = {'Poseidon_Black.jpg', 'Poseidon_Gold.jpg', 'Poseidon_Blue.jpg', 'Poseidon_Pearl.jpg'}
ROUNDTRIP_GATE = 1e-6
LINK_CONSTANTS = dict(m1_rotation=(0, 90, 0), m1_translation=(-47, -7, 0),
                      m2_rotation=(145, 0, 275), m2_translation=(0, 10, -30))


def matrix_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def angle_matrix(roll, pitch, yaw):
    """Engine AngleMatrix (ZzzMathLib.cpp): degrees, Rz(yaw)Ry(pitch)Rx(roll)."""
    sr, cr = math.sin(math.radians(roll)), math.cos(math.radians(roll))
    sp, cp = math.sin(math.radians(pitch)), math.cos(math.radians(pitch))
    sy, cy = math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
    return [[cp * cy, sr * sp * cy - cr * sy, cr * sp * cy + sr * sy, 0.0],
            [cp * sy, sr * sp * sy + cr * cy, cr * sp * sy - sr * cy, 0.0],
            [-sp, sr * cp, cr * cp, 0.0],
            [0.0, 0.0, 0.0, 1.0]]


def translation_matrix(matrix, tx, ty, tz):
    result = [row[:] for row in matrix]
    result[0][3], result[1][3], result[2][3] = tx, ty, tz
    return result


def euler_xyz_matrix(rotation):
    """mathutils-compatible Euler('XYZ') rotation: R = Rz @ Ry @ Rx."""
    rx, ry, rz = rotation
    cx, sx = math.cos(rx), math.sin(rx)
    cy, sy = math.cos(ry), math.sin(ry)
    cz, sz = math.cos(rz), math.sin(rz)

    def mul3(a, b):
        return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]

    rz_m = [[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]]
    ry_m = [[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]
    rx_m = [[1, 0, 0], [0, cx, -sx], [0, sx, cx]]
    return mul3(rz_m, mul3(ry_m, rx_m))


def local_matrix(clip_value, frame):
    rot = euler_xyz_matrix(clip_value['rotations'][frame])
    pos = clip_value['positions'][frame]
    return [[rot[i][j] if j < 3 else pos[i] for j in range(4)] for i in range(3)] + \
           [[0.0, 0.0, 0.0, 1.0]]


def world_bone_matrices(model):
    mats = []
    for index, bone in enumerate(model['bones']):
        if bone.get('dummy'):
            mats.append(None)
            continue
        local = local_matrix(bone['clips'][0], 0)
        parent = bone['parent']
        if parent is None or parent < 0 or mats[parent] is None:
            mats.append(local)
        else:
            mats.append(matrix_mul(mats[parent], local))
    return mats


def independent_cape_root(player, cape_bone_clip):
    """Recompute BoneTransform[19] . M1 . M2 . bone0 with no mathutils involved."""
    m1 = translation_matrix(angle_matrix(*LINK_CONSTANTS['m1_rotation']), *LINK_CONSTANTS['m1_translation'])
    m2 = translation_matrix(angle_matrix(*LINK_CONSTANTS['m2_rotation']), *LINK_CONSTANTS['m2_translation'])
    bone19 = world_bone_matrices(player)[LINK_BONE]
    return matrix_mul(matrix_mul(bone19, matrix_mul(m1, m2)), local_matrix(cape_bone_clip, 0))


class PoseidonCapeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.model = inspect(cls.output / 'models' / 'Poseidon_Cape.bmd', True)
        cls.build = json.loads((cls.output / 'cape-build-report.json').read_text())
        with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory() as temp:
            cape_path = Path(temp) / 'native.bmd'
            cape_path.write_bytes(archive.read(CAPE_MEMBER))
            cls.native = inspect(cape_path, True)
            player_path = Path(temp) / 'player.bmd'
            player_path.write_bytes(archive.read(PLAYER_MEMBER))
            cls.player = inspect(player_path, True)

    def test_preview_model_with_declared_atlases_only(self):
        self.assertEqual(self.model['name'], 'Poseidon_Cape')
        self.assertEqual({mesh['texture'] for mesh in self.model['meshes']} <= ATLASES, True)
        self.assertNotIn('Celestial', str(self.model['meshes']))
        self.assertNotIn('Kundun', str(self.model['meshes']))
        self.assertNotIn('redwings', str(self.model['meshes']))

    def test_exported_skeleton_preserves_the_frozen_native_contract(self):
        native = self.native
        self.assertEqual(native['bones'][0]['name'], 'collar')
        self.assertEqual(len(self.model['bones']), 1)
        self.assertEqual(self.model['action_frames'], [1])
        self.assertEqual(self.model['action_locks'], [False])
        self.assertNotEqual(self.model['sha256'], native['sha256'])  # new geometry, not a reexport
        bone = self.model['bones'][0]
        source = native['bones'][0]
        self.assertEqual(bone.get('dummy', False), False)
        self.assertEqual(bone['name'], source['name'])
        self.assertEqual(bone['parent'], source['parent'])
        self.assertEqual(bone['clips'][0]['positions'], source['clips'][0]['positions'])
        self.assertEqual(bone['clips'][0]['rotations'], source['clips'][0]['rotations'])

    def test_single_bone_binding_follows_the_cape_contract(self):
        used = sorted({bone for mesh in self.model['meshes'] for bone in mesh['used_bones']})
        self.assertEqual(used, [0])
        for mesh in self.model['meshes']:
            for bone, _ in mesh['points']:
                self.assertEqual(bone, 0)
                self.assertFalse(self.model['bones'][bone].get('dummy', False))

    def test_client_mesh_limits(self):
        self.assertEqual(Path(self.model['file']).read_bytes()[:4], b'BMD\x0a')
        self.assertLessEqual(len(self.model['meshes']), 50)
        for index, mesh in enumerate(self.model['meshes']):
            self.assertEqual(mesh['texture_index'], index)
            self.assertLessEqual(mesh['triangles'], 2200)
            self.assertLessEqual(max(mesh['vertices'], len(mesh['normal_vectors']),
                                     len(mesh['texcoords'])), 10000)

    def test_finite_vertices_unit_normals_and_indices_in_range(self):
        for mesh in self.model['meshes']:
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

    def test_triangle_budget_holds_and_matches_report(self):
        triangles = sum(mesh['triangles'] for mesh in self.model['meshes'])
        budget = self.build['triangle_budget']
        self.assertGreaterEqual(triangles, budget['low'])
        self.assertLessEqual(triangles, budget['high'])
        self.assertEqual(triangles, self.build['piece']['triangles'])
        self.assertEqual(self.build['piece']['used_bones'], [0])

    def test_generation_and_saved_scene_roundtrip(self):
        saved = json.loads((self.output / 'cape-saved-roundtrip-report.json').read_text())
        blend_hash = hashlib.sha256((self.output / 'poseidon-cape.blend').read_bytes()).hexdigest()
        self.assertEqual(saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        self.assertLessEqual(saved['Cape']['max_error'], ROUNDTRIP_GATE)
        self.assertEqual(saved['Cape']['sha256'], self.model['sha256'])
        self.assertEqual(self.build['piece']['sha256'], self.model['sha256'])
        self.assertEqual(self.build['piece']['topology']['nonmanifold_components'], [])
        self.assertGreater(self.build['piece']['minimum_triangle_area'], 1e-8)

    def test_preview_cannot_be_mistaken_for_an_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORED_CAPE_PROTOTYPE_PREVIEW_ONLY')
        self.assertEqual(self.build['renderer'].startswith('Blender Cycles'), True)
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        self.assertEqual(self.build['native_contract']['native_sha256'],
                         audit['models'][AUDIT_KEY]['sha256'])
        self.assertEqual(self.build['native_contract']['bone'], 'collar')
        link = self.build['link_math']
        for key, value in LINK_CONSTANTS.items():
            self.assertEqual(link[key], list(value), key)
        self.assertEqual(link['bone'], LINK_BONE)
        dependencies = self.build['texture_dependencies']
        self.assertEqual({item['texture'] for item in dependencies}, ATLASES)
        # The atlases exist now as authored, regenerable art with hashes; the
        # package stays preview-only, so the install gate below still holds.
        self.assertTrue(all(item['status'] == 'AUTHORED_ATLAS_REGENERABLE'
                            for item in dependencies))
        self.assertTrue(all(item.get('generator') == 'generate_poseidon_atlases.py'
                            for item in dependencies))
        self.assertTrue(all(item.get('master_sha256') and item.get('game_jpeg_sha256')
                            and item.get('ozj_sha256') for item in dependencies))
        self.assertFalse((self.output / 'Data').exists())
        self.assertFalse((self.output / 'Poseidon_Black.jpg').exists())

    def test_link_math_matches_an_independent_recomputation(self):
        recorded = self.build['cape_root_translation']
        recomputed = independent_cape_root(self.player, self.native['bones'][0]['clips'][0])
        for axis in range(3):
            self.assertAlmostEqual(recorded[axis], recomputed[axis][3], delta=1e-3)
        equipped = json.loads((self.output / 'cape-equipped-review-report.json').read_text())
        self.assertEqual(equipped['cape_root_translation'], recorded)
        self.assertEqual(equipped['link_bone'], LINK_BONE)

    def test_equipped_preview_follows_the_player_rig(self):
        equipped = json.loads((self.output / 'cape-equipped-review-report.json').read_text())
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        self.assertEqual(equipped['status'], 'ATTACHMENT_MATH_PREVIEW_NOT_INGAME')
        self.assertEqual(equipped['player_sha256'], audit['models']['owner_rig']['sha256'])
        self.assertEqual(equipped['native_cape_sha256'], audit['models'][AUDIT_KEY]['sha256'])
        self.assertEqual(len(equipped['poses']), 2)
        gates = equipped['anchor_gates']
        self.assertGreaterEqual(gates['root_follow'], gates['root_follow_min'])
        self.assertLessEqual(gates['rigidity_drift'], gates['rigidity_drift_max'])
        for probe in equipped['span_probes']:
            self.assertLess(probe['farthest'], gates['farthest_below'])
        renders = self.output / 'renders'
        expected = [f'Poseidon_Cape_{view}.png' for view in ('front', 'side', 'back')]
        expected += ['Poseidon_CapeSet_equipped_front.png', 'Poseidon_CapeSet_equipped_back.png']
        for render in expected:
            self.assertTrue((renders / render).exists(), render)

    def test_cloth_contract_study_documents_the_runtime_anchors(self):
        study = (ROOT / 'cape-cloth-contract.md').read_text(encoding='utf-8')
        for anchor in ('MODEL_CAPE_OF_EMPEROR', 'LinkBone = 19', 'CPhysicsCloth',
                       'dl_redwings02', 'BITMAP_ROBE', 'RenderLinkObject',
                       'RATE_SHORT_SHOULDER', 'PCT_CURVED', 'collar',
                       'Data/Item/DarkLordRobe02.bmd'):
            self.assertIn(anchor, study, anchor)


if __name__ == '__main__':
    unittest.main()
