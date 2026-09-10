"""Structural regression checks for the staged Celestial assets, not visual approval."""
import hashlib
import json
import math
import unittest
from pathlib import Path

from inspect_bmd_rig import inspect, payload

ROOT = Path(__file__).resolve().parent
STAGED = ROOT / 'authored-props'
ITEMS = STAGED / 'Data' / 'Item'
NAMES = ('Staff', 'Shield', 'Pendant', 'Ring')
PERMITTED_TEXTURES = {f'Celestial_{name}.jpg' for name in ('Gold', 'Ivory', 'Sapphire', 'Emissive')}


class AuthoredPropsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.models = {name: inspect(ITEMS / f'Celestial_{name}.bmd', full=True) for name in NAMES}

    def test_complete_prop_package_without_native_appearance(self):
        for name, model in self.models.items():
            with self.subTest(name=name):
                self.assertEqual(model['name'], 'Celestial_' + name)
                self.assertTrue(model['meshes'])
                self.assertLessEqual(len(model['meshes']), 50)
                self.assertEqual(model['action_frames'], [1])
                for mesh in model['meshes']:
                    self.assertIn(mesh['texture'], PERMITTED_TEXTURES)
                    self.assertTrue(all(0 < mesh[key] <= 10000 for key in
                                        ('vertices', 'triangles')))

    def test_face_indices_and_normal_bindings(self):
        for name, model in self.models.items():
            for mesh in model['meshes']:
                with self.subTest(name=name, texture=mesh['texture']):
                    for (vertices, uvs), normals in zip(mesh['faces'], mesh['normal_indices']):
                        self.assertEqual(len(vertices), 3)
                        for vertex, normal, uv in zip(vertices, normals, uvs):
                            self.assertTrue(0 <= vertex < len(mesh['points']))
                            self.assertTrue(0 <= normal < len(mesh['normal_vectors']))
                            self.assertTrue(0 <= uv < len(mesh['texcoords']))
                            self.assertEqual(mesh['normal_vectors'][normal][-1], vertex)

    def test_finite_geometry_and_unit_normals(self):
        for model in self.models.values():
            for mesh in model['meshes']:
                for bone, point in mesh['points']:
                    self.assertEqual(bone, 0)
                    self.assertTrue(all(math.isfinite(value) for value in point))
                for bone, x, y, z, _ in mesh['normal_vectors']:
                    self.assertEqual(bone, 0)
                    self.assertAlmostEqual(math.sqrt(x*x + y*y + z*z), 1, places=4)
                for uv in mesh['texcoords']:
                    self.assertTrue(all(-.001 <= value <= 1.001 for value in uv))

    def test_triangles_are_not_degenerate(self):
        for name, model in self.models.items():
            for mesh in model['meshes']:
                for indices, _ in mesh['faces']:
                    a, b, c = [mesh['points'][index][1] for index in indices]
                    u, v = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
                    cross = (u[1]*v[2] - u[2]*v[1], u[2]*v[0] - u[0]*v[2], u[0]*v[1] - u[1]*v[0])
                    self.assertGreater(sum(value*value for value in cross), 1e-14, name)

    def test_staff_anchor_is_in_head_not_grip(self):
        model = self.models['Staff']
        self.assertEqual(len(model['bones']), 3)
        self.assertEqual(model['bones'][1]['parent'], 0)
        anchor = model['bones'][1]['clips'][0]['first_position']
        self.assertAlmostEqual(anchor[1], -86)
        self.assertAlmostEqual(anchor[0], 3)
        self.assertAlmostEqual(anchor[2], 0)
        points = [p for mesh in model['meshes'] for _, p in mesh['points']]
        length = max(p[1] for p in points) - min(p[1] for p in points)
        self.assertTrue(210 < length < 230)
        width = max(p[2] for p in points) - min(p[2] for p in points)
        depth = max(p[0] for p in points) - min(p[0] for p in points)
        self.assertGreater(width, depth * 3)

    def test_ring_setting_is_above_band(self):
        model = self.models['Ring']
        gems = [point for mesh in model['meshes'] if mesh['texture'] == 'Celestial_Sapphire.jpg'
                for _, point in mesh['points']]
        self.assertTrue(gems)
        self.assertGreater(min(point[2] for point in gems), 5)

    def test_every_texture_resolves_to_staged_ozj(self):
        for model in self.models.values():
            for mesh in model['meshes']:
                original = (ROOT / 'textures' / mesh['texture']).read_bytes()
                packed = (ITEMS / Path(mesh['texture']).with_suffix('.OZJ')).read_bytes()
                self.assertEqual(packed[24:], original)
                self.assertEqual(packed[24:26], b'\xff\xd8')

    def test_manifest_matches_every_payload_file(self):
        manifest = json.loads((STAGED / 'sha256.json').read_text())
        files = {str(path.relative_to(STAGED)).replace('\\', '/') for path in ITEMS.iterdir()}
        self.assertEqual(set(manifest), files)
        for name, digest in manifest.items():
            self.assertEqual(hashlib.sha256((STAGED / name).read_bytes()).hexdigest(), digest)

    def test_bad_signature_rejected(self):
        with self.assertRaises(ValueError):
            payload(b'NOT\x0a' + bytes(40))


if __name__ == '__main__':
    unittest.main(verbosity=2)
