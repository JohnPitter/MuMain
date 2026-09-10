"""Reject missing or conflicting assets before any launcher publication."""
import unittest
from artifact_proof import digest, validate_proof

from package_candidate import MODEL_PATHS, ROOT, add_asset, archive, collect_payload, validate_assets


class CandidatePackageTests(unittest.TestCase):
    def test_ten_models_and_seventeen_assets(self):
        payload = collect_payload()
        self.assertEqual(len(payload), 17)
        self.assertEqual(len(MODEL_PATHS), 10)
        self.assertEqual(len(validate_assets(payload)), 17)

    def test_missing_item_texture_is_rejected(self):
        payload = collect_payload()
        payload.pop('Data/Item/Celestial_Ivory.OZJ')
        with self.assertRaisesRegex(ValueError, 'Missing texture'):
            validate_assets(payload)

    def test_missing_armor_model_is_rejected(self):
        payload = collect_payload()
        payload.pop('Data/Player/Celestial_Armor.bmd')
        with self.assertRaisesRegex(ValueError, 'Incomplete'):
            validate_assets(payload)

    def test_conflicting_shared_texture_is_rejected(self):
        payload = {'Data/Item/Celestial_Gold.OZJ': ROOT / 'textures/Celestial_Gold.jpg'}
        with self.assertRaisesRegex(ValueError, 'Conflicting'):
            add_asset(payload, 'Data/Item/Celestial_Gold.OZJ', ROOT / 'textures/Celestial_Ivory.jpg')

    def test_traversal_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Unsafe'):
            add_asset({}, '../Main.exe', ROOT / 'textures/Celestial_Gold.jpg')

    def test_archive_is_deterministic(self):
        payload = collect_payload()
        self.assertEqual(archive(payload), archive(payload))

    def test_stale_or_missing_proof_is_rejected(self):
        model = ROOT / 'authored-wings/Data/Item/Celestial_Wings.bmd'
        blend = ROOT / 'authored-wings/celestial-authored-wings.blend'
        valid = dict(model_sha256=digest(model), blend_sha256=digest(blend))
        validate_proof(valid, model, blend)
        for field in valid:
            stale = {**valid, field: '0' * 64}
            with self.assertRaisesRegex(ValueError, 'Stale'):
                validate_proof(stale, model, blend)
        with self.assertRaisesRegex(ValueError, 'missing'):
            validate_proof({}, model, blend)


if __name__ == '__main__':
    unittest.main(verbosity=2)
