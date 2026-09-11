"""Color roles are shared without changing physical texture identifiers."""
import unittest

from celestial_palette import CONSTRUCTION_ROLES, MATERIALS, construction_palette


class CelestialPaletteTests(unittest.TestCase):
    def test_large_surfaces_are_gold_and_trim_keeps_legacy_texture_identifier(self):
        self.assertEqual(CONSTRUCTION_ROLES['Base'], 'Gold')
        self.assertEqual(CONSTRUCTION_ROLES['Trim'], 'Ivory')

    def test_trim_is_metallic_gray_instead_of_cream(self):
        color, metallic, roughness = MATERIALS['Ivory']
        self.assertLess(max(color), 0.65)
        self.assertLess(max(color) - min(color), 0.15)
        self.assertGreaterEqual(metallic, 0.85)
        self.assertLessEqual(roughness, 0.25)

    def test_gold_is_yellow_polished_metal(self):
        color, metallic, roughness = MATERIALS['Gold']
        self.assertGreater(color[0], color[1])
        self.assertGreater(color[1], color[2] * 3)
        self.assertGreaterEqual(metallic, 0.85)
        self.assertLessEqual(roughness, 0.20)

    def test_crystals_and_emission_keep_their_materials(self):
        for name in ('Sapphire', 'Emissive'):
            self.assertEqual(CONSTRUCTION_ROLES[name], name)

    def test_roles_reference_the_original_material_instances(self):
        original = {name: object() for name in MATERIALS}
        palette = construction_palette(original)
        for role, name in CONSTRUCTION_ROLES.items():
            self.assertIs(palette[role], original[name])
        self.assertEqual(len(original), 4)

    def test_missing_physical_material_is_not_silently_replaced(self):
        with self.assertRaises(KeyError):
            construction_palette({'Gold': object()})


if __name__ == '__main__':
    unittest.main()
