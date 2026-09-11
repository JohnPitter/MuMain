"""Tests for the bounded native-monster diagnostic measurements."""
import unittest

from PIL import Image

from compare_finish_textures import compare
from inspect_knight_reference import mesh_record, structural_digest, texture_stats


class KnightMeasurementsTests(unittest.TestCase):
    def test_counts_real_triangles_including_quads(self):
        mesh = dict(vertices=4, triangles=2, texture='plate.jpg', used_bones=[0],
                    faces=[((0, 1, 2), (0, 1, 2)), ((0, 1, 2, 3), (0, 1, 2, 3))],
                    normal_indices=[(0, 0, 0), (1, 0, 0, 0)], normal_vectors=[(0, 0, 0, 1, 0)] * 2,
                    points=[(0, (0, 0, 0))] * 4, texcoords=[(0, 0)] * 4)
        result = mesh_record(mesh)
        self.assertEqual(2, result['polygon_records'])
        self.assertEqual(3, result['triangles'])
        self.assertEqual(1, result['vertices_with_multiple_normal_indices'])

    def test_atlas_statistics_do_not_add_lighting(self):
        result = texture_stats(Image.new('RGBA', (8, 8), (229, 229, 229, 255)))
        self.assertEqual([229] * 3, [result[name] for name in ('luminance_p05', 'luminance_p50', 'luminance_p95')])

    def test_structural_hash_detects_geometry_change(self):
        value = {'points': [(0, (1, 2, 3))]}
        self.assertEqual(structural_digest(value), structural_digest(value))
        self.assertNotEqual(structural_digest(value), structural_digest({'points': [(0, (1, 2, 4))]}))

    def test_finish_comparison_uses_percentile_span(self):
        old = {'statistics': {'luminance_p05': 129, 'luminance_p95': 161}}
        new = {'statistics': {'luminance_p05': 83, 'luminance_p95': 209}}
        self.assertEqual(3.9375, compare(old, new)['comparison']['contrast_span_ratio'])

    def test_flat_baseline_has_no_division_by_zero(self):
        flat = {'statistics': {'luminance_p05': 229, 'luminance_p95': 229}}
        self.assertIsNone(compare(flat, flat)['comparison']['contrast_span_ratio'])


if __name__ == '__main__':
    unittest.main()
