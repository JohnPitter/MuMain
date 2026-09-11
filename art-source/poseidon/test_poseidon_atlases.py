"""Pure-Python gates for the definitive Poseidon atlases; no client, no Blender."""
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
GENERATOR = ROOT / 'generate_poseidon_atlases.py'
PYTHON = Path(sys.executable)
ROLES = ('Black', 'Gold', 'Blue')
# The declared texture-dependency contract also carries the cape's fourth
# role, Branco Perolado, authored with the owner-approved platinum finish.
DEPENDENCY_ROLES = ROLES + ('Pearl',)
MODELS = ('Trident', 'Scepter', 'Helm', 'Armor', 'Boot')
# Sampled from the concept swatch panel (generate_poseidon_atlases.py) and the
# documented finish bands: black stays abyssal by design, gold/blue carry the
# engraved contrast grammar measured on the Celestial golden-metal masters,
# and pearl holds the approved white-platina band (~130+, reference 154).
CONTRAST_BANDS = {'Black': (24, 72), 'Gold': (100, 160), 'Blue': (100, 160),
                  'Pearl': (130, 172)}
ANCHOR_MIDS = {'Black': '#111114', 'Gold': '#d8a066', 'Blue': '#3684dd',
               'Pearl': '#d7d3d4'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class PoseidonAtlasTests(unittest.TestCase):
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
        self.assertEqual(sorted(self.dependencies), sorted(DEPENDENCY_ROLES))
        for role, entry in self.dependencies.items():
            self.assertEqual(entry['texture'], f'Poseidon_{role}.jpg')
            self.assertEqual(entry['status'], 'AUTHORED_ATLAS_REGENERABLE')
            self.assertEqual(entry['generator'], 'generate_poseidon_atlases.py')
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

    def test_pearl_carries_the_approved_platinum_white_finish(self):
        """Owner rule: the pearl white is the approved branco platina.

        The approved white-platina masters (feat/celestial-white-platina) landed
        master P95-P05 166 / game 154 under the sampled anchors
        #545156/#d7d3d4/#faf8f6; Pearl must hold that band and those anchors.
        """
        pearl = self.report['atlases']['Pearl']
        master, game = pearl['master'], pearl['game_jpeg']
        self.assertGreaterEqual(master['contrast_span_p95_p05'], 130)
        self.assertGreaterEqual(game['contrast_span_p95_p05'], 130)
        self.assertLessEqual(abs(master['luminance_p05'] - 82), 6)
        self.assertLessEqual(abs(master['luminance_p95'] - 248), 6)
        self.assertEqual(self.report['palette']['anchors']['Pearl'],
                         dict(shadow='#545156', mid='#d7d3d4', highlight='#faf8f6'))

    def test_cape_cloth_mass_texture_is_declared_full_field(self):
        """The runtime cloth grids stretch one image across UV 0..1
        (cape-cloth-contract.md, the dl_redwings02.tga role), so the mass and
        gleam atlases must be authored full-field and declared as such."""
        study = (ROOT / 'cape-cloth-contract.md').read_text(encoding='utf-8')
        self.assertIn('Poseidon_Pearl.jpg', study)
        self.assertIn('esticada', study)
        for role in ('Black', 'Pearl'):
            self.assertTrue(
                self.report['palette']['anchors'][role] is not None, role)

    def test_palette_anchors_come_from_the_concept_swatches(self):
        sampled = self.report['palette']['sampled']
        self.assertEqual(sampled['dourado_real_principal'], '#d8a066')
        self.assertEqual(sampled['preto_abissal_principal'], '#111114')
        self.assertEqual(sampled['azul_oceanico_principal'], '#3684dd')
        self.assertEqual(sampled['branco_perolado_principal'], '#e5e5ec')
        for role, anchor in ANCHOR_MIDS.items():
            self.assertEqual(self.report['palette']['anchors'][role]['mid'], anchor)
        self.assertTrue((ROOT / 'prototype' / 'renders' / 'Poseidon_atlas_palette.png').exists())

    def test_uv_layout_maps_all_five_models_to_the_three_atlases(self):
        self.assertEqual(self.regions['status'], 'UV_LAYOUT_PROOF_FOR_DEFINITIVE_ATLASES')
        self.assertEqual(sorted(self.regions['models']), sorted(MODELS))
        for name, data in self.regions['models'].items():
            atlases = {f'Poseidon_{role}.jpg' for role in ROLES}
            self.assertEqual(set(data['atlas_components']), atlases, name)
            self.assertEqual(set(data['bmd_atlas_texcoords']), atlases, name)
            for record in data['components']:
                self.assertLessEqual(record['triangles'], 2200)
                for axis in record['uv_bounds'].values():
                    self.assertLessEqual(axis[1] - axis[0], 1.0001)

    def test_atlases_are_absent_from_any_client_install_layout(self):
        self.assertFalse((ROOT / 'prototype' / 'Data').exists())
        self.assertFalse((REPO / 'Data' / 'Item' / 'Poseidon_Black.OZJ').exists())
        self.assertFalse((REPO / 'Data' / 'Player' / 'Poseidon_Armor.OZJ').exists())

    def test_generation_is_deterministic(self):
        result = subprocess.run([str(PYTHON), str(GENERATOR), '--check'],
                                capture_output=True, text=True, timeout=900)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('POSEIDON_ATLASES_CHECK_OK', result.stdout)


if __name__ == '__main__':
    unittest.main()
