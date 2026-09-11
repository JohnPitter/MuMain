"""Original Poseidon silhouettes, assembled from shared geometric primitives."""
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

from celestial_geometry import bezier, ellipse, gem, mesh, rim, tube


def blade_profile(name, outline, thickness, material):
    """Closed, beveled concave extrusion; the bevel is real geometry, not a shader."""
    cx = sum(x for x, _ in outline) / len(outline)
    cz = sum(z for _, z in outline) / len(outline)
    inset = [(cx + (x - cx) * .94, cz + (z - cz) * .94) for x, z in outline]
    rings = [[Vector((x, y, z)) for x, z in points] for points, y in
             ((inset, -thickness / 2 - .55), (outline, -thickness / 2),
              (outline, thickness / 2), (inset, thickness / 2 + .55))]
    count = len(outline)
    vertices = [vertex for ring in rings for vertex in ring]
    faces = []
    for triangle in tessellate_polygon([rings[0]]):
        indices = tuple(v if isinstance(v, int) else rings[0].index(v) for v in triangle)
        faces.extend((indices, tuple(i + 3 * count for i in reversed(indices))))
    for row in range(3):
        base = row * count
        faces.extend((base + i, base + (i + 1) % count, base + (i + 1) % count + count,
                      base + i + count) for i in range(count))
    return mesh(name, vertices, faces, material)


def edged_blade(name, outline, materials, thickness=2.1):
    blade_profile(name + '_black_metal', outline, thickness, materials['Black'])
    for depth, side in ((-thickness / 2, 'front'), (thickness / 2, 'rear')):
        rim(name + '_gold_edge_' + side, outline, depth, materials['Gold'], .52)


def shaft(name, materials, top):
    tube(name + '_structural_shaft', [(0, 0, z) for z in (-73, -43, -17, 25, top)],
         [1.25, 1.05, 1.4, 1.1, 1.7], materials['Gold'], sides=12)
    tube(name + '_black_grip', [(0, 0, -19), (0, 0, 18)], 1.65,
         materials['Black'], sides=12)
    for index, z in enumerate((-68, -43, -21, 20, 36, top - 3)):
        ellipse(f'{name}_ferrule_{index}', (0, 0, z), ((2, 0, 0), (0, 2, 0)),
                materials['Gold'], .4, 16)
    for index in range(9):
        z = -16 + index * 3.8
        ellipse(f'{name}_grip_rib_{index}', (0, 0, z), ((1.7, 0, 0), (0, 1.7, 0)),
                materials['Black'], .27, 12)
    for sign in (-1, 1):
        tube(f'{name}_grip_inlay_{sign}', [(sign * 1.25, -1.1, z) for z in (-17, 0, 18)],
             .18, materials['Gold'], sides=6)
    finial = [(0, -87), (3.7, -75), (2.2, -65), (0, -61), (-2.2, -65), (-3.7, -75)]
    edged_blade(name + '_ocean_finial', finial, materials, 1.6)
    gem(name + '_pommel_crystal', (0, -1.3, -73), (1.5, .8, 3.2), materials['Blue'])


def core_housing(name, materials, center, width):
    outline = [(0, center + width * 1.8), (width, center),
               (0, center - width * 1.8), (-width, center)]
    edged_blade(name + '_core_socket', outline, materials, 3.4)
    gem(name + '_ocean_core', (0, -2.2, center),
        (width * .63, 2.2, width * 1.22), materials['Blue'])
    gem(name + '_rear_ocean_seal', (0, 2.5, center),
        (width * .4, 1, width * .75), materials['Blue'])
    for sign in (-1, 1):
        path = bezier([(sign * width, -.8, center - 7), (sign * 12, -2, center - 1),
                       (sign * 9, -2, center + 5), (sign * 3, -.8, center + 8)], 12)
        tube(f'{name}_curl_{sign}', path, [.6 - i * .035 for i in range(12)], materials['Gold'])


def crown_spine(name, materials, ends):
    low, high = ends
    path = [(0, 1.2, low), (0, 1.2, 70), (0, .7, high)]
    tube(name + '_structural_black_spine', path, [1.8, 2.2, .8], materials['Black'], sides=10)
    for sign in (-1, 1):
        tube(f'{name}_spine_gold_rib_{sign}', [(sign * 1.5, -.1, low),
             (sign * 1.5, -.1, 70), (sign * .5, -.1, high)], [.4, .45, .18], materials['Gold'])


def trident(materials):
    shaft('Trident', materials, 62)
    crown_spine('Trident', materials, (60, 94))
    core_housing('Trident', materials, 70, 5.7)
    center = [(0, 136), (4.5, 124), (2.8, 118), (3.1, 105), (6.5, 95),
              (4.8, 87), (0, 80), (-4.8, 87), (-6.5, 95), (-3.1, 105),
              (-2.8, 118), (-4.5, 124)]
    edged_blade('Trident_center_spear', center, materials)
    gem('Trident_spear_inlay', (0, -1.6, 110), (1.4, .65, 10), materials['Blue'])
    outer = [(3.8, 61), (9, 73), (12.5, 88), (17.3, 102), (18.2, 116),
             (24.5, 130), (23.8, 111), (27, 102), (22, 93), (18.5, 80),
             (15, 70), (5, 57)]
    fluke = [(12, 75), (20, 85), (26, 89), (21, 79), (22, 74), (14, 68)]
    for sign in (-1, 1):
        edged_blade(f'Trident_outer_tine_{sign}', [(sign * x, z) for x, z in outer], materials)
        edged_blade(f'Trident_fluke_{sign}', [(sign * x, z) for x, z in fluke], materials, 1.5)
        path = [(sign * x, -1.8, z) for x, z in ((7, 65), (14, 80), (19, 98), (21, 119))]
        tube(f'Trident_tine_fluting_{sign}', path, [.28, .4, .32, .08], materials['Gold'])
    for z in (28, 43, 55):
        gem(f'Trident_shaft_diamond_{z}', (0, -1.5, z), (.6, .35, 2), materials['Blue'])


def scepter(materials):
    shaft('Scepter', materials, 51)
    crown_spine('Scepter', materials, (49, 95))
    core_housing('Scepter', materials, 70, 7)
    center = [(0, 110), (4.5, 98), (2.7, 90), (0, 85), (-2.7, 90), (-4.5, 98)]
    edged_blade('Scepter_crown_peak', center, materials, 2.8)
    crown = [(2, 48), (10, 55), (17, 69), (18, 82), (14, 91), (15, 102),
             (10, 94), (10.5, 84), (13, 75), (10, 65), (5, 58)]
    barb = [(13, 62), (25, 77), (27, 91), (21, 82), (18, 73), (14, 70)]
    for sign in (-1, 1):
        edged_blade(f'Scepter_wave_crown_{sign}', [(sign * x, z) for x, z in crown], materials, 2.6)
        edged_blade(f'Scepter_lateral_crown_{sign}', [(sign * x, z) for x, z in barb], materials, 1.6)
        gem(f'Scepter_crown_gem_{sign}', (sign * 14, -2, 73), (1.7, .8, 4), materials['Blue'])
    for z in (30, 39, 49):
        diamond = [(0, z + 3), (2.6, z), (0, z - 3), (-2.6, z)]
        edged_blade(f'Scepter_shaft_escutcheon_{z}', diamond, materials, 1.5)
