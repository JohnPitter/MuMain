"""Regression checks for the catalog's read-only mapping and wrapper decoding."""
import unittest
from io import BytesIO
from pathlib import Path

from PIL import Image

from build_catalog import texture_script
from item_models import literal_item_loads, loop_identity
from mappings import enrich_mapping, filename_mapping, item_layout, runtime_primary_paths, server_names
from textures import open_texture, palette, runtime_semantics


CLIENT = Path(__file__).resolve().parents[2]
SERVER = Path('C:/_wt-celestial-server')


class CatalogTests(unittest.TestCase):
    def test_server_class_positions(self):
        names = server_names(SERVER)
        self.assertEqual([entry['name'] for entry in names['8:0']['classes']], ['Dark Knight', 'Magic Gladiator', 'Dark Lord'])
        for key in ('7:30', '8:30', '9:30', '10:30', '11:30'):
            self.assertEqual(names[key]['classes'], [{'name': 'Dark Wizard', 'evolution_requirement': 2}])
        self.assertEqual(names['8:74']['minimum_level'], 400)

    def test_filename_families(self):
        examples = {'HelmMale47.bmd': 46, 'HDK_ArmorMale02.bmd': 30, 'CW_BootMale03.bmd': 36,
                    'PantElfC02.bmd': 11, 'ArmorMonk03.bmd': 8, 'Celestial_Boots.bmd': 74}
        for name, number in examples.items():
            with self.subTest(name=name):
                self.assertEqual(filename_mapping('Data/Player/' + name)['item_number'], number)
        self.assertEqual(filename_mapping('Data/Player/Armor_inventory60.bmd')['variant'], 'inventory_only')

    def test_class_body_is_not_equipment(self):
        value = filename_mapping('Data/Player/HelmClass302.bmd')
        self.assertEqual(value['category'], 'class_body_or_head')
        self.assertEqual(value['class'], 'Dark Knight')
        self.assertEqual(value['evolution_mesh'], 3)
        self.assertNotIn('item_number', value)

    def test_native_loader_exclusions(self):
        loaded = runtime_primary_paths()
        self.assertIn('data/player/t_pantmale19.bmd', loaded)
        self.assertNotIn('data/player/pantmale19.bmd', loaded)
        self.assertIn('data/player/armormaletest20.bmd', loaded)
        self.assertNotIn('data/player/helmmale48.bmd', loaded)
        self.assertNotIn('data/player/glovemale74.bmd', loaded)

    def test_literal_kundun_and_celestial_staff(self):
        loads = literal_item_loads(CLIENT)
        self.assertEqual((loads['data/item/staff12.bmd'][0]['group'], loads['data/item/staff12.bmd'][0]['item_number']), (5, 11))
        self.assertEqual((loads['data/item/celestial_staff.bmd'][0]['group'], loads['data/item/celestial_staff.bmd'][0]['item_number']), (5, 37))
        self.assertEqual(loads['data/item/swordl33.bmd'][0]['texture_directories'], ['Data/player/'])

    def test_wing_and_crossbow_numbering(self):
        self.assertEqual(loop_identity('wing08'), (12, 36))
        self.assertEqual(loop_identity('crossbow01'), (4, 8))
        self.assertIsNone(loop_identity('wing99'))

    def test_item_record_layout(self):
        header = (CLIENT / 'src/source/Data/GameData/ItemData/ItemFieldDefs.h').read_text()
        fields, size = item_layout(header, 30)
        self.assertEqual(size, 84)
        self.assertEqual(fields['RequireClass'], (69, 7))
        self.assertEqual(item_layout(header, 50)[1], 104)

    def test_ozj_repeated_jpeg_prefix(self):
        buffer = BytesIO()
        Image.new('RGB', (8, 8), '#DDB34C').save(buffer, format='JPEG')
        raw = buffer.getvalue()
        decoded = open_texture('gold.OZJ', raw[:24] + raw)
        self.assertEqual(decoded.size, (8, 8))
        self.assertTrue(palette(decoded))

    def test_ozt_wrapper(self):
        buffer = BytesIO()
        Image.new('RGBA', (8, 8), (1, 2, 3, 123)).save(buffer, format='TGA')
        decoded = open_texture('cloth.OZT', bytes(4) + buffer.getvalue())
        self.assertEqual(decoded.getpixel((0, 0)), (1, 2, 3, 123))

    def test_texture_script_is_case_sensitive_and_stops_at_invalid_token(self):
        self.assertEqual(texture_script('glow_RHSN.jpg')['flags'], ['bright', 'hidden', 'stream', 'none_blend'])
        self.assertFalse(texture_script('glow_R2.jpg')['enabled'])
        self.assertFalse(texture_script('Celestial_Gold.jpg')['enabled'])

    def test_hidden_skin_and_hair_are_runtime_semantics(self):
        self.assertTrue(runtime_semantics('hide_m.jpg')['hidden_sentinel'])
        self.assertFalse(runtime_semantics('Hide_m.jpg')['hidden_sentinel'])
        self.assertTrue(runtime_semantics('LevelSkin.jpg')['skin'])
        self.assertTrue(runtime_semantics('hair01.tga')['hair'])

    def test_cp932_names_are_not_semantically_confirmed(self):
        names = {'server': {}, 'client': {'Eng': {'8:62': {'name': 'legacy name', 'name_encoding': 'cp932-fallback'}}}}
        mapping = {'group': 8, 'item_number': 62}
        self.assertEqual(enrich_mapping(mapping, names)['name_reliability'], 'client_cp932_inferred')


if __name__ == '__main__':
    unittest.main()
