"""Pure-Python gates for the definitive Zeus atlases; no client, no Blender."""
import hashlib
import json
import subprocess
import sys
import unittest
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent.parent
TEXTURES = ROOT / 'textures'
GENERATOR = ROOT / 'generate_zeus_atlases.py'
PYTHON = Path(sys.executable)
ROLES = ('Blue', 'Platina', 'Emissive')
MODELS = ('Sword', 'Staff', 'Armor', 'Pant', 'Glove', 'Boot', 'Wings',
          'Pendant', 'Ring_Storm', 'Ring_Wisdom', 'Cape')
# Measured finish bands (atlas-report.json, game JPEG P95-P05): Blue carries
# the engraved celeste metal grammar (Legendary-study band like Poseidon
# Gold/Blue), Emissive the storm-energy channels, and Platina holds the
# owner-approved white-platina band (reference 154-166, floor 130).
CONTRAST_BANDS = {'Blue': (100, 160), 'Platina': (130, 172), 'Emissive': (100, 160)}
ANCHOR_MIDS = {'Blue': '#2f66b8', 'Platina': '#d7d3d4', 'Emissive': '#1f74d8'}
SAMPLED = {'azul_celeste_principal': '#328efa',
           'azul_celeste_massa': '#3a62a0',
           'azul_celeste_profundo': '#16325e',
           'energia_emissiva': '#2579d8',
           'branco_platinado_principal': '#d7d3d4',
           'branco_platinado_brilho': '#faf8f6'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ZeusAtlasTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((TEXTURES / 'atlas-report.json').read_text(encoding='utf-8'))
        cls.dependencies = {entry['material_role']: entry for entry in json.loads(
            (ROOT / 'prototype' / 'texture-dependencies.json').read_text(encoding='utf-8'))}
        cls.regions = json.loads(
            (ROOT / 'prototype' / 'uv-region-report.json').read_text(encoding='utf-8'))

    def repo(self, relative):
        return REPO / relative

    def test_every_role_ships_master_game_jpeg_and_ozj(self):
        for role in ROLES:
            atlas = self.report['atlases'][role]
            master, jpeg, ozj = (self.repo(atlas[part]['path']) for part in ('master', 'game_jpeg', 'ozj'))
            self.assertTrue(master.exists() and jpeg.exists() and ozj.exists(), role)
            with Image.open(master) as image:
                image.load()
                self.assertEqual(image.size, (1024, 1024), role)
                self.assertEqual(image.mode, 'RGB', role)
                self.assertEqual(image.format, 'PNG', role)
            with Image.open(jpeg) as image:
                image.load()
                self.assertEqual(image.size, (512, 512), role)
                self.assertEqual(image.mode, 'RGB', role)
                self.assertEqual(image.format, 'JPEG', role)
            payload = ozj.read_bytes()
            self.assertEqual(payload[:24], bytes(24), f'{role}: OZJ wrapper header')
            self.assertEqual(payload[24:26], b'\xff\xd8', f'{role}: OZJ JPEG payload')
            self.assertEqual(payload[24:], jpeg.read_bytes(), f'{role}: OZJ wraps the game JPEG bytes')

    def test_report_hashes_match_the_written_artifacts(self):
        for role in ROLES:
            atlas = self.report['atlases'][role]
            for part in ('master', 'game_jpeg', 'ozj'):
                self.assertEqual(atlas[part]['sha256'], digest(self.repo(atlas[part]['path'])),
                                 f'{role}/{part}')

    def test_dependency_contract_points_at_the_authored_atlases(self):
        self.assertEqual(sorted(self.dependencies), sorted(ROLES))
        for role, entry in self.dependencies.items():
            self.assertEqual(entry['texture'], f'Zeus_{role}.jpg')
            self.assertEqual(entry['status'], 'AUTHORED_ATLAS_REGENERABLE')
            self.assertEqual(entry['generator'], 'generate_zeus_atlases.py')
            self.assertEqual(entry['master_sha256'],
                             self.report['atlases'][role]['master']['sha256'])
            self.assertEqual(entry['game_jpeg_sha256'],
                             self.report['atlases'][role]['game_jpeg']['sha256'])
            self.assertEqual(entry['ozj_sha256'],
                             self.report['atlases'][role]['ozj']['sha256'])

    def test_finish_contrast_bands_hold_on_the_game_jpeg(self):
        for role, (low, high) in CONTRAST_BANDS.items():
            span = self.report['atlases'][role]['game_jpeg']['contrast_span_p95_p05']
            self.assertGreaterEqual(span, low, role)
            self.assertLessEqual(span, high, role)

    def test_platina_carries_the_approved_platinum_white_finish(self):
        """Owner rule: the detail white is the approved branco platina.

        The approved white-platina masters landed master P95-P05 166 / game
        154 under the sampled anchors #545156/#d7d3d4/#faf8f6 with the
        P05-P95 normalization window; Platina must hold that band, those
        anchors and that exact window (no other white may be invented).
        """
        platina = self.report['atlases']['Platina']
        master, game = platina['master'], platina['game_jpeg']
        self.assertGreaterEqual(master['contrast_span_p95_p05'], 130)
        self.assertGreaterEqual(game['contrast_span_p95_p05'], 130)
        self.assertLessEqual(abs(master['luminance_p05'] - 82), 6)
        self.assertLessEqual(abs(master['luminance_p95'] - 248), 6)
        self.assertEqual(self.report['palette']['anchors']['Platina'],
                         dict(shadow='#545156', mid='#d7d3d4', highlight='#faf8f6'))
        self.assertEqual(self.report['palette']['sampled']['branco_platinado_principal'],
                         '#d7d3d4')

    def test_palette_anchors_come_from_the_concept_board(self):
        sampled = self.report['palette']['sampled']
        for key, value in SAMPLED.items():
            self.assertEqual(sampled[key], value, key)
        for role, anchor in ANCHOR_MIDS.items():
            self.assertEqual(self.report['palette']['anchors'][role]['mid'], anchor)
        self.assertTrue((ROOT / 'prototype' / 'renders' / 'Zeus_atlas_palette.png').exists())

    def test_uv_layout_maps_all_eleven_models_to_the_three_atlases(self):
        self.assertEqual(self.regions['status'], 'UV_LAYOUT_PROOF_FOR_DEFINITIVE_ATLASES')
        self.assertEqual(sorted(self.regions['models']), sorted(MODELS))
        atlases = {f'Zeus_{role}.jpg' for role in ROLES}
        for name, data in self.regions['models'].items():
            self.assertEqual(set(data['atlas_components']), atlases, name)
            self.assertEqual(set(data['bmd_atlas_texcoords']), atlases, name)
            for texture, texcoords in data['bmd_atlas_texcoords'].items():
                self.assertGreater(texcoords, 0, f'{name}/{texture}')
            for record in data['components']:
                self.assertLessEqual(record['triangles'], 2200)
                for axis in record['uv_bounds'].values():
                    self.assertLessEqual(axis[1] - axis[0], 1.0001)

    def test_every_bmd_still_names_only_the_three_atlases(self):
        """The rebuilt BMDs are byte-identical and wire every authored atlas.

        Each saved-roundtrip report proves its models independently re-decode
        from the saved scenes below the 1e-6 gate; the per-model sha256 binds
        the proof to the exact BMD bytes shipped in this lane.
        """
        reports = (('saved-roundtrip-report.json', lambda r: r['models'], 'max_error'),
                   ('armor-saved-roundtrip-report.json', lambda r: r['models'], 'max_error'),
                   ('wings-saved-roundtrip-report.json', lambda r: {'Wings': r}, 'max_geometry_error'),
                   ('jewels-saved-roundtrip-report.json', lambda r: r['models'], 'max_error'),
                   ('cape-saved-roundtrip-report.json', lambda r: {'Cape': r['Cape']}, 'max_error'))
        found = {}
        for filename, group, error_key in reports:
            report = json.loads((ROOT / 'prototype' / filename).read_text())
            entries = group(report)
            for name, entry in entries.items():
                found[name] = (entry['sha256'], entry[error_key])
        self.assertEqual(sorted(found), sorted(MODELS))
        for name, (sha, error) in found.items():
            self.assertEqual(len(sha), 64, name)
            self.assertLessEqual(error, 1e-6, name)

    def test_atlases_are_absent_from_any_client_install_layout(self):
        self.assertFalse((ROOT / 'prototype' / 'Data').exists())
        self.assertFalse((REPO / 'Data' / 'Item' / 'Zeus_Sword.OZJ').exists())
        self.assertFalse((REPO / 'Data' / 'Player' / 'Zeus_Armor.OZJ').exists())

    def test_generation_is_deterministic(self):
        result = subprocess.run([str(PYTHON), str(GENERATOR), '--check'],
                                capture_output=True, text=True, timeout=900)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('ZEUS_ATLASES_CHECK_OK', result.stdout)


if __name__ == '__main__':
    unittest.main()
