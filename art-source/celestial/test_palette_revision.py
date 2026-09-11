"""A recolor proof must reject geometry/pose regressions and decimal-bin errors."""
from copy import deepcopy
import unittest

from verify_palette_revision import compare_models, match_surfaces


def model(texture='Celestial_Ivory.jpg'):
    mesh = dict(texture=texture,
                points=[(0, [0., 0., 0.]), (0, [1., 0., 0.]), (0, [0., 1., 0.])],
                normal_vectors=[(0, 0., 0., 1., vertex) for vertex in range(3)],
                texcoords=[(.1976351, .5), (.3, .4), (.6, .7)],
                faces=[((0, 1, 2), (0, 1, 2))], normal_indices=[(0, 1, 2)])
    return dict(name='Celestial', sha256='fixture', meshes=[mesh],
                bones=[dict(name='root', parent=-1, clips=[dict(positions=[(0, 0, 0)],
                                                              rotations=[(0, 0, 0)])])],
                action_frames=[1], action_locks=[True], action_positions=[[(0, 0, 0)]])


class PaletteRevisionTests(unittest.TestCase):
    def setUp(self):
        self.old = model()
        self.new = deepcopy(self.old)
        self.new['meshes'][0]['texture'] = 'Celestial_Gold.jpg'

    def compare(self, name='Celestial_Helm.bmd'):
        return compare_models(self.old, self.new, name)

    def test_base_trim_swap_is_exact(self):
        self.assertTrue(self.compare()['geometry_normals_winding_exact'])
        self.assertEqual(self.compare()['maximum_uv_error'], 0)

    def test_cyclic_corner_rotation_preserves_winding(self):
        mesh = self.new['meshes'][0]
        mesh['faces'] = [((1, 2, 0), (1, 2, 0))]
        mesh['normal_indices'] = [(1, 2, 0)]
        self.assertTrue(self.compare()['geometry_normals_winding_exact'])

    def test_reversed_winding_is_rejected(self):
        mesh = self.new['meshes'][0]
        mesh['faces'] = [((0, 2, 1), (0, 2, 1))]
        mesh['normal_indices'] = [(0, 2, 1)]
        with self.assertRaisesRegex(ValueError, 'winding'):
            self.compare()

    def test_microscopic_geometry_change_is_not_hidden_by_rounding(self):
        self.new['meshes'][0]['points'][0][1][0] = .00000001
        with self.assertRaisesRegex(ValueError, 'geometry'):
            self.compare('Celestial_Pendant.bmd')

    def test_normal_vector_or_bone_change_is_rejected(self):
        for normal in ((0, .00000001, 0., 1., 0), (1, 0., 0., 1., 0)):
            self.new['meshes'][0]['normal_vectors'][0] = normal
            with self.assertRaisesRegex(ValueError, 'normals'):
                self.compare()

    def test_root_motion_and_missing_contract_are_rejected(self):
        self.new['action_positions'] = [[(0, 0, .00000001)]]
        with self.assertRaisesRegex(ValueError, 'action_positions'):
            self.compare()
        del self.new['action_positions']
        with self.assertRaisesRegex(ValueError, 'action_positions'):
            self.compare()

    def test_animation_keyframe_change_is_rejected(self):
        self.new['bones'][0]['clips'][0]['rotations'][0] = (0, .00000001, 0)
        with self.assertRaisesRegex(ValueError, 'bones'):
            self.compare()

    def test_jewelry_uv_bin_representative_can_cross_decimal_boundary(self):
        self.new['meshes'][0]['texcoords'][0] = (.1976349, .5)
        report = self.compare('Celestial_Pendant.bmd')
        self.assertAlmostEqual(report['maximum_uv_error'], .0000002)
        self.assertEqual(report['changed_uv_triangles'], 1)
        with self.assertRaisesRegex(ValueError, 'UVs'):
            self.compare()

    def test_jewelry_uv_beyond_exporter_quantization_is_rejected(self):
        self.new['meshes'][0]['texcoords'][0] = (.1976339, .5)
        with self.assertRaisesRegex(ValueError, 'UVs'):
            self.compare('Celestial_Ring.bmd')

    def test_only_existing_jewelry_gold_can_stay_gold(self):
        self.old = model('Celestial_Gold.jpg')
        self.assertTrue(self.compare('Celestial_Ring.bmd')['geometry_uv_normals_preserved'])
        with self.assertRaises(ValueError):
            self.compare()
        self.old = model()
        self.new = deepcopy(self.old)
        with self.assertRaises(ValueError):
            self.compare('Celestial_Pendant.bmd')

    def test_crystal_uv_and_texture_must_stay_exact(self):
        self.old = model('Celestial_Sapphire.jpg')
        self.new = deepcopy(self.old)
        self.new['meshes'][0]['texcoords'][0] = (.1976352, .5)
        with self.assertRaises(ValueError):
            self.compare('Celestial_Ring.bmd')
        self.new = deepcopy(self.old)
        self.new['meshes'][0]['texture'] = 'Celestial_Emissive.jpg'
        with self.assertRaises(ValueError):
            self.compare('Celestial_Ring.bmd')

    def test_non_finite_values_are_rejected(self):
        self.new['meshes'][0]['texcoords'][0] = (float('nan'), .5)
        with self.assertRaisesRegex(ValueError, 'Non-finite'):
            self.compare('Celestial_Pendant.bmd')

    def test_triangle_multiplicity_must_stay_exact(self):
        mesh = self.new['meshes'][0]
        mesh['faces'] *= 2
        mesh['normal_indices'] *= 2
        with self.assertRaises(ValueError):
            self.compare()

    def test_duplicate_geometry_uses_one_to_one_uv_matching(self):
        texture = 'Celestial_Gold.jpg'
        before = [(texture, ((.00000075, 0),)), (texture, ((0, 0),))]
        after = [(texture, ((0, 0),)), (texture, ((.0000015, 0),))]
        maximum, changed = match_surfaces(before, after, True)
        self.assertAlmostEqual(maximum, .00000075)
        self.assertEqual(changed, 1)


if __name__ == '__main__':
    unittest.main()
