"""Small isolated ZIPs exercise the finish-only release integrity gate."""
import hashlib
from io import BytesIO
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch
import zipfile

from PIL import Image

import verify_metal_revision as revision

MODEL_PATHS = [f'Data/Player/Celestial_{name}.bmd'
               for name in ('Helm', 'Armor', 'Pants', 'Gloves', 'Boots')]
MODEL_PATHS += [f'Data/Item/Celestial_{name}.bmd'
                for name in ('Staff', 'Shield', 'Pendant', 'Ring', 'Wings')]


def jpeg(size=(512, 512), mode='RGB', image_format='JPEG'):
    data = BytesIO()
    Image.new(mode, size, 96).save(data, format=image_format)
    return data.getvalue()


class MetalRevisionTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(TemporaryDirectory()))
        self.enterContext(patch.object(revision, 'ROOT', self.root))
        self.current = {name: f'unchanged BMD fixture: {name}'.encode() for name in MODEL_PATHS}
        self.current.update({f'Data/{folder}/Celestial_{name}.OZJ': bytes(24) + jpeg()
                             for folder in ('Player', 'Item') for name in ('Gold', 'Ivory')})
        for folder in ('Player', 'Item'):
            self.current[f'Data/{folder}/Celestial_Sapphire.OZJ'] = b'unchanged sapphire'
        self.current['Data/Item/Celestial_Emissive.OZJ'] = b'unchanged emission'
        self.previous = self.current.copy()
        for name in revision.CHANGED_TEXTURES:
            self.previous[name] = b'previous finish'
        self.base = self.root / 'previous.zip'
        with zipfile.ZipFile(self.base, 'w') as archive:
            for name, raw in self.previous.items():
                archive.writestr(name, raw)
        expected = hashlib.sha256(self.base.read_bytes()).hexdigest()
        self.enterContext(patch.object(revision, 'EXPECTED_PREVIOUS_BASE', expected))
        for name, raw in self.current.items():
            target = self.asset_path(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(raw)
        for name in ('Gold', 'Ivory'):
            self.source_path(name).parent.mkdir(parents=True, exist_ok=True)
            self.source_path(name).write_bytes(jpeg())

    def asset_path(self, name):
        return self.root / 'authored-armor' / name

    def source_path(self, material):
        return self.root / 'textures' / f'Celestial_{material}.jpg'

    def replace_finish(self, raw):
        self.source_path('Gold').write_bytes(raw)
        for folder in ('Item', 'Player'):
            self.asset_path(f'Data/{folder}/Celestial_Gold.OZJ').write_bytes(bytes(24) + raw)

    def test_valid_revision_proves_ten_models_and_seventeen_assets(self):
        report = revision.verify(self.base)
        self.assertTrue(report['passed'])
        self.assertTrue(report['geometry_rig_uv_actions_unchanged'])
        self.assertFalse(report['ingame_appearance_verified'])
        self.assertEqual(report['models_byte_identical'], 10)
        self.assertEqual(set(report['model_hashes']), set(MODEL_PATHS))
        self.assertEqual(len(report['changed_textures']), 4)
        self.assertEqual(len(report['preserved_textures']), 3)

    def test_wrong_previous_base_is_rejected_before_asset_comparison(self):
        self.base.write_bytes(self.base.read_bytes() + b'changed')
        with self.assertRaisesRegex(ValueError, 'previously deployed Golden base'):
            revision.verify(self.base)

    def test_single_model_byte_change_is_rejected(self):
        target = self.asset_path(MODEL_PATHS[0])
        target.write_bytes(target.read_bytes() + b'changed keyframe or geometry')
        with self.assertRaisesRegex(ValueError, 'Geometry or animation changed'):
            revision.verify(self.base)

    def test_missing_equipment_asset_is_rejected(self):
        self.asset_path(MODEL_PATHS[0]).unlink()
        with self.assertRaisesRegex(ValueError, 'seventeen equipment assets'):
            revision.verify(self.base)

    def test_extra_equipment_asset_is_rejected(self):
        self.asset_path('Data/Player/Unexpected.OZJ').write_bytes(b'extra')
        with self.assertRaisesRegex(ValueError, 'seventeen equipment assets'):
            revision.verify(self.base)

    def test_mismatched_shared_texture_copies_are_rejected(self):
        target = self.root / 'authored-wings/Data/Item/Celestial_Gold.OZJ'
        target.parent.mkdir(parents=True)
        target.write_bytes(b'conflicting copy')
        with self.assertRaisesRegex(ValueError, 'Conflicting asset copies'):
            revision.verify(self.base)

    def test_identical_shared_texture_copy_is_allowed(self):
        name = 'Data/Item/Celestial_Gold.OZJ'
        target = self.root / 'authored-wings' / name
        target.parent.mkdir(parents=True)
        target.write_bytes(self.current[name])
        self.assertTrue(revision.verify(self.base)['passed'])

    def test_gem_or_emission_change_is_rejected(self):
        target = self.asset_path('Data/Item/Celestial_Emissive.OZJ')
        target.write_bytes(b'new emission outside this release')
        with self.assertRaisesRegex(ValueError, 'only four gold/steel texture copies'):
            revision.verify(self.base)

    def test_source_and_packaged_texture_must_be_identical(self):
        self.source_path('Gold').write_bytes(jpeg((256, 256)))
        with self.assertRaisesRegex(ValueError, 'differs from editable source'):
            revision.verify(self.base)

    def test_ozj_wrapper_is_checked(self):
        target = self.asset_path('Data/Player/Celestial_Gold.OZJ')
        target.write_bytes(b'x' + target.read_bytes()[1:])
        with self.assertRaisesRegex(ValueError, 'Unexpected OZJ wrapper'):
            revision.verify(self.base)

    def test_non_rgb_or_wrong_dimensions_or_non_jpeg_are_rejected(self):
        for raw in (jpeg((256, 512)), jpeg(mode='L'), jpeg(image_format='PNG')):
            with self.subTest(size=len(raw)):
                self.replace_finish(raw)
                with self.assertRaisesRegex(ValueError, 'Texture format changed'):
                    revision.verify(self.base)

    def test_truncated_jpeg_body_is_rejected_even_when_header_is_valid(self):
        self.replace_finish(jpeg()[:-100])
        with self.assertRaises(OSError):
            revision.verify(self.base)

    def test_revision_cannot_leave_one_gold_or_steel_copy_unmodified(self):
        name = 'Data/Player/Celestial_Gold.OZJ'
        self.previous[name] = self.current[name]
        with zipfile.ZipFile(self.base, 'w') as archive:
            for entry, raw in self.previous.items():
                archive.writestr(entry, raw)
        expected = hashlib.sha256(self.base.read_bytes()).hexdigest()
        with patch.object(revision, 'EXPECTED_PREVIOUS_BASE', expected):
            with self.assertRaisesRegex(ValueError, 'only four gold/steel texture copies'):
                revision.verify(self.base)


if __name__ == '__main__':
    unittest.main()
