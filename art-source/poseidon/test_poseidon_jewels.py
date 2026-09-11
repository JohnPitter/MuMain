"""Pure-Python gates for the authored Poseidon pendant and rings; no Blender startup."""
import hashlib
import json
import math
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / 'celestial'))
from inspect_bmd_rig import inspect

PIECES = ('Pendant', 'RingTide', 'RingEmperor')
RINGS = ('RingTide', 'RingEmperor')
ANCHOR_LABELS = {'Pendant': ('Seat', 'Bail', 'Drop'),
                 'RingTide': ('Band', 'Gem'), 'RingEmperor': ('Band', 'Gem')}
# Native precedent equipped one ring model twice; these are two authored models.
ATLASES = {'Poseidon_Black.jpg', 'Poseidon_Gold.jpg', 'Poseidon_Blue.jpg'}
ROUNDTRIP_GATE = 1e-6
# Jewelry band agreed for this batch: light items, below the smallest worn
# piece (helm, 3484 triangles) and above the native item floor (saint.bmd, 313).
JEWELRY_LOW, JEWELRY_HIGH = 1500, 3500
HELM_FLOOR = 3484


class PoseidonJewelsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.output = ROOT / 'prototype'
        cls.models = {name: inspect(cls.output / 'models' / f'Poseidon_{name}.bmd', True)
                      for name in PIECES}
        cls.build = json.loads((cls.output / 'jewels-build-report.json').read_text())

    def test_original_names_and_declared_atlases(self):
        for name, model in self.models.items():
            self.assertEqual(model['name'], 'Poseidon_' + name)
            self.assertEqual({mesh['texture'] for mesh in model['meshes']}, ATLASES)
            self.assertNotIn('Celestial', str(model['meshes']))
            self.assertNotIn('Kundun', str(model['meshes']))

    def test_rigid_contract_follows_the_audited_native_item(self):
        audit = json.loads((ROOT / 'native-reference-audit.json').read_text())
        native_item = audit['models']['native_scepter_orientation_only']
        contract = self.build['native_item_contract']
        self.assertEqual(contract['sha256'], native_item['sha256'])
        self.assertEqual(contract['audit_sha256'],
                         hashlib.sha256((ROOT / 'native-reference-audit.json')
                                        .read_bytes()).hexdigest())
        self.assertEqual(contract['archive_sha256'], audit['archive_sha256'])
        for name, model in self.models.items():
            self.assertEqual(model['action_frames'], [1])
            self.assertEqual(model['action_locks'], [False])
            expected_parents = [-1] + [0] * (len(ANCHOR_LABELS[name]) - 1)
            self.assertEqual([bone['parent'] for bone in model['bones']], expected_parents)
            for mesh in model['meshes']:
                self.assertEqual(mesh['used_bones'], [0])

    def test_anchors_name_mount_and_gem_points(self):
        for name, model in self.models.items():
            labels = ANCHOR_LABELS[name]
            bones = model['bones']
            self.assertEqual([bone['name'] for bone in bones],
                             [f'Poseidon_{name}_{label}' for label in labels])
            recorded = self.build['pieces'][name]['anchors']
            self.assertEqual(list(recorded), list(labels))
            for bone, label in zip(bones, labels):
                position = bone['clips'][0]['first_position']
                self.assertEqual([round(value, 4) for value in position], recorded[label])
        # Pendant anchors are recorded in item space: the export maps authored
        # +Z (up, toward bail and chain) to item -Y. The bail sits above the
        # frame top (authored z 13.5 -> item y -8.5) but below the chain tip;
        # the drop hangs below the frame bottom (z -12.5 -> item y 17.5).
        points = [point for mesh in self.models['Pendant']['meshes'] for _, point in mesh['points']]
        top, bottom = min(p[1] for p in points), max(p[1] for p in points)
        anchors = self.build['pieces']['Pendant']['anchors']
        self.assertLess(anchors['Bail'][1], -8.5)
        self.assertGreater(anchors['Bail'][1], top)
        self.assertGreater(anchors['Drop'][1], 17)
        self.assertLess(anchors['Drop'][1], bottom)
        for name in RINGS:
            gem_z = self.build['pieces'][name]['anchors']['Gem'][2]
            self.assertGreater(gem_z, 5)

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
                    for indices, limit in ((vertices, mesh['vertices']),
                                           (uvs, len(mesh['texcoords'])),
                                           (normals, len(mesh['normal_vectors']))):
                        self.assertTrue(all(0 <= index < limit for index in indices))

    def test_triangle_budgets_hold_inside_the_jewelry_band(self):
        self.assertIn('budget_justification', self.build)
        for name, model in self.models.items():
            triangles = sum(mesh['triangles'] for mesh in model['meshes'])
            low, high = self.build['triangle_budgets'][name]
            self.assertTrue(JEWELRY_LOW <= low <= triangles <= high <= JEWELRY_HIGH, name)
            self.assertLess(triangles, HELM_FLOOR, name)
            self.assertEqual(triangles, self.build['pieces'][name]['triangles'])

    def test_two_rings_are_distinct_authored_models(self):
        shas = {name: self.models[name]['sha256'] for name in RINGS}
        self.assertNotEqual(shas['RingTide'], shas['RingEmperor'])
        self.assertNotEqual(shas['RingTide'], self.build['pieces']['Pendant']['sha256'])
        self.assertNotEqual(shas['RingEmperor'], self.build['pieces']['Pendant']['sha256'])
        tris = {name: sum(mesh['triangles'] for mesh in self.models[name]['meshes'])
                for name in RINGS}
        self.assertGreater(abs(tris['RingTide'] - tris['RingEmperor']), 100)
        crest_heights = {}
        for name in RINGS:
            points = [point for mesh in self.models[name]['meshes'] for _, point in mesh['points']]
            crest_heights[name] = max(p[2] for p in points)
        self.assertGreater(crest_heights['RingEmperor'], crest_heights['RingTide'] + 1.5)

    def test_generation_and_saved_scene_roundtrip(self):
        saved = json.loads((self.output / 'jewels-saved-roundtrip-report.json').read_text())
        blend_hash = hashlib.sha256(
            (self.output / 'poseidon-jewels.blend').read_bytes()).hexdigest()
        self.assertEqual(saved['saved_blend_sha256'], blend_hash)
        self.assertEqual(self.build['blend_sha256'], blend_hash)
        for name, model in self.models.items():
            self.assertLessEqual(saved['models'][name]['max_error'], ROUNDTRIP_GATE, name)
            self.assertEqual(saved['models'][name]['sha256'], model['sha256'])
            self.assertEqual(self.build['pieces'][name]['sha256'], model['sha256'])
            self.assertEqual(self.build['pieces'][name]['topology']['nonmanifold_components'], [])
            self.assertGreater(self.build['pieces'][name]['minimum_triangle_area'], 1e-8)

    def test_preview_cannot_be_mistaken_for_an_installable_package(self):
        self.assertEqual(self.build['status'], 'AUTHORED_JEWELS_PROTOTYPE_PREVIEW_ONLY')
        self.assertEqual(self.build['renderer'].startswith('Blender Cycles'), True)
        dependencies = json.loads((self.output / 'texture-dependencies.json').read_text())
        self.assertEqual(len(dependencies), 3)
        self.assertTrue(all(d['status'] == 'MISSING_ATLAS_NOT_FOR_CLIENT'
                            for d in dependencies))
        self.assertFalse((self.output / 'Data').exists())

    def test_inspection_renders_exist(self):
        renders = self.output / 'renders'
        for name in PIECES:
            for view in ('front', 'side', 'detail'):
                self.assertTrue((renders / f'Poseidon_{name}_{view}.png').exists(),
                                f'{name}/{view}')


if __name__ == '__main__':
    unittest.main()
