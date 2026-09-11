"""Renderer dependency and checkpoint rules, without launching Blender or MU."""
from io import BytesIO
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
from zipfile import ZipFile

sys.modules.setdefault('bpy', SimpleNamespace())
sys.modules.setdefault('mathutils', SimpleNamespace(Vector=object, Matrix=object, Euler=object))
import render_native_families as renderer


class NativeRenderRulesTests(unittest.TestCase):
    def archive(self, entries):
        buffer = BytesIO()
        with ZipFile(buffer, 'w') as archive:
            for name, value in entries.items():
                archive.writestr(name, value)
        archive = ZipFile(buffer)
        self.addCleanup(archive.close)
        return renderer.ArchiveAssets(archive, '.')

    def test_verified_shared_helmet_has_explicit_origin(self):
        source = 'Data/Player/LuckyItem/65/head helmet Luck 40.OZJ'
        assets = self.archive({source: b'fixture texture'})
        entry = assets.resolve_texture('head helmet Luck 40.jpg', 'Data/Player/LuckyItem/70')
        self.assertEqual(entry.filename, source)
        self.assertFalse(assets.missing)
        record = next(iter(assets.fallbacks.values()))
        self.assertEqual(record['archive_entry'], source)
        self.assertEqual(record['resolution'], 'verified_preloaded_name_fallback')
        self.assertEqual(len(record['sha256']), 64)

    def test_fallback_does_not_search_unverified_directories(self):
        assets = self.archive({'Data/Player/Other/head helmet Luck 40.OZJ': b'texture'})
        self.assertIsNone(assets.resolve_texture('head helmet Luck 40.jpg', 'Data/Player/LuckyItem/70'))
        self.assertEqual(assets.missing, {'head helmet Luck 40.jpg'})
        self.assertFalse(assets.fallbacks)

    def test_verified_name_does_not_enable_fallback_for_another_family(self):
        assets = self.archive({'Data/Player/LuckyItem/65/head helmet Luck 40.OZJ': b'texture'})
        self.assertIsNone(assets.resolve_texture('head helmet Luck 40.jpg', 'Data/Player/LuckyItem/71'))

    def test_same_directory_texture_takes_precedence(self):
        own = 'Data/Player/LuckyItem/70/head helmet Luck 40.OZJ'
        assets = self.archive({own: b'own', 'Data/Player/LuckyItem/65/head helmet Luck 40.OZJ': b'shared'})
        self.assertEqual(assets.resolve_texture('head helmet Luck 40.jpg', 'Data/Player/LuckyItem/70').filename, own)
        self.assertFalse(assets.fallbacks)

    def test_hidden_sentinel_never_creates_a_mesh_or_material(self):
        family = dict(pieces={'Armor': [dict(archive_entry='Data/Player/Armor.bmd')]})
        model = dict(sha256='fixture', meshes=[dict(texture='hide_m.jpg'), dict(texture='Armor.jpg')])
        assets = Mock()
        assets.model.return_value = model
        visible = SimpleNamespace(data=SimpleNamespace(materials=[]))
        with patch.object(renderer, 'world_matrices'), patch.object(renderer, 'create_mesh', return_value=visible) as create:
            objects, reports, hidden = renderer.add_family(family, assets)
        self.assertEqual(objects, [visible])
        self.assertEqual(create.call_count, 1)
        assets.material.assert_called_once_with('Armor.jpg', 'Data/Player')
        self.assertEqual(hidden[0]['texture'], 'hide_m.jpg')
        self.assertEqual(hidden[0]['mesh'], 0)
        self.assertEqual(len(reports), 1)

    def test_selected_family_ids_are_not_catalog_offsets(self):
        catalog = dict(families=[dict(item_number=65), dict(item_number=69), dict(item_number=70)])
        args = SimpleNamespace(families=[70, 69], start=0, limit=None)
        self.assertEqual([index for index, _ in renderer.selected_families(catalog, args)], [1, 2])
        args.families = [500]
        with self.assertRaises(ValueError):
            renderer.selected_families(catalog, args)

    def test_partial_checkpoint_preserves_other_family_records(self):
        catalog = dict(source=dict(id='fixture'), families=[dict(item_number=i) for i in range(10)])
        records = [dict(item_number=i, missing_textures=[], views=['front', 'rear'], marker=f'old-{i}')
                   for i in range(10)]
        replacement = dict(item_number=3, missing_textures=[], views=['new-front', 'new-rear'])
        with TemporaryDirectory() as directory, patch.object(renderer, 'OUTPUT', Path(directory)):
            path = Path(directory) / 'native-render-report-000.json'
            path.write_text(json.dumps(dict(source=catalog['source'], families=records)))
            renderer.store_result(catalog, 3, replacement, Mock())
            merged = json.loads(path.read_text())['families']
        self.assertEqual(len(merged), 10)
        self.assertEqual(merged[3], replacement)
        for index in set(range(10)) - {3}:
            self.assertEqual(merged[index], records[index])


if __name__ == '__main__':
    unittest.main()
