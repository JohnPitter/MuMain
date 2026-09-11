"""Original Zeus silhouettes, assembled from shared geometric primitives."""
import math

from mathutils import Vector
from mathutils.geometry import tessellate_polygon

from celestial_geometry import bezier, ellipse, gem, mesh, rim, tube

# Grip sits at the origin: the sword runs -81 (pommel gem) to 152 (tip), the
# staff -84 (bottom spike) to 178 (crown star point). Hand bones carry these as
# rigid props; budgets mirror the Poseidon weapons (~6-8k triangles each).
SWORD_TIP = 152
SWORD_CORE = 64
STAFF_CROWN = 152
STAFF_TIP = 178


def star_outline(center_z, outer, inner, points, phase=math.pi / 2):
    """Angular star used by the Zeus family (pommel, crown)."""
    outline = []
    for index in range(points * 2):
        radius = outer if index % 2 == 0 else inner
        angle = phase + math.pi * index / points
        outline.append((radius * math.cos(angle), center_z + radius * math.sin(angle)))
    return outline


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


def edged_blade(name, outline, materials, thickness=2.1, core='Blue', edge='Platina'):
    blade_profile(name + '_blue_core', outline, thickness, materials[core])
    for depth, side in ((-thickness / 2, 'front'), (thickness / 2, 'rear')):
        rim(name + '_platina_edge_' + side, outline, depth, materials[edge], .52)


def grip_handle(name, materials, low, high, radius=1.5):
    tube(name + '_blue_grip', [(0, 0, z) for z in (low, (low + high) / 2, high)],
         [radius, radius * 1.08, radius], materials['Blue'], sides=12)
    ribs = 8
    for index in range(ribs):
        z = low + 2 + (high - low - 4) * index / (ribs - 1)
        ellipse(f'{name}_platina_rib_{index}', (0, 0, z), ((radius * 1.06, 0, 0), (0, radius * 1.06, 0)),
                materials['Platina'], .3, 12)
    for sign in (-1, 1):
        tube(f'{name}_grip_inlay_{sign}',
             [(sign * radius * 1.04, -1.0, z) for z in (low + 3, (low + high) / 2, high - 3)],
             .16, materials['Platina'], sides=6)


def sword(materials):
    # Pommel: eight-point star with an emissive drop gem and collar.
    edged_blade('Sword_pommel_star', star_outline(-70, 7.5, 3, 8), materials, 1.7)
    ellipse('Sword_pommel_collar', (0, 0, -62), ((2.3, 0, 0), (0, 2.3, 0)),
            materials['Platina'], .45, 16)
    gem('Sword_pommel_gem', (0, -1.2, -80), (1.7, .95, 3.4), materials['Emissive'])
    grip_handle('Sword', materials, -60, -24)
    # Crossguard: housing plus two swept thunderbolt quillons with counterblades.
    edged_blade('Sword_guard_housing', [(0, -2), (6.4, -14), (0, -27), (-6.4, -14)],
                materials, 3.6)
    edged_blade('Sword_guard_rear_plate', [(0, -30), (4.4, -36), (0, -44), (-4.4, -36)],
                materials, 2.2)
    gem('Sword_guard_gem', (0, -2.6, -14), (2.4, 1.3, 5), materials['Emissive'])
    quillon = [(1.8, -10), (13, -6), (24.5, 1), (32.5, 12), (36.5, 26), (28.5, 20),
               (20.5, 10), (11, 3.5), (3.8, 1.5), (1.8, -3)]
    counter = [(1.6, -24), (9.5, -29), (17, -37), (21, -47), (15.5, -43), (10, -36),
               (4.5, -30), (1.6, -27)]
    for sign in (-1, 1):
        edged_blade(f'Sword_quillon_{sign}', [(sign * x, z) for x, z in quillon], materials)
        edged_blade(f'Sword_counterblade_{sign}', [(sign * x, z) for x, z in counter],
                    materials, 1.6)
        path = [(sign * x, -2.0, z) for x, z in ((5, -4), (16, 2), (26, 11), (33, 22))]
        tube(f'Sword_quillon_fluting_{sign}', path, [.26, .38, .3, .07], materials['Platina'])
        inner = bezier([(sign * 3.4, -2.4, -8), (sign * 9, -2.6, -2),
                        (sign * 7, -2.4, 6), (sign * 2.6, -1.6, 12)], 10)
        tube(f'Sword_guard_arc_{sign}', inner, [.5 - i * .04 for i in range(10)],
             materials['Platina'])
    # Blade: long facet with platina edges and a proud emissive energy channel.
    blade = [(0, SWORD_TIP), (3.2, 138), (4.6, 118), (5.2, 96), (5.0, 72), (4.2, 46),
             (3.2, 22), (2.4, 4), (0, 0), (-2.4, 4), (-3.2, 22), (-4.2, 46), (-5.0, 72),
             (-5.2, 96), (-4.6, 118), (-3.2, 138)]
    edged_blade('Sword_blade', blade, materials, 2.4)
    channel = [(0, 142), (1.7, 122), (2.3, 98), (2.1, 66), (1.5, 32), (0, 12),
               (-1.5, 32), (-2.1, 66), (-2.3, 98), (-1.7, 122)]
    edged_blade('Sword_blade_energy_channel', channel, materials, 3.1, core='Emissive')
    gem('Sword_blade_base_seal', (0, -2.0, 3), (1.2, .7, 2.6), materials['Emissive'])
    for sign in (-1, 1):
        tube(f'Sword_blade_spine_{sign}', [(sign * 2.2, -1.0, z) for z in (4, 44, 88, 126)],
             [.3, .42, .34, .08], materials['Platina'], sides=6)


def staff(materials):
    # Bottom spike so no view shows an unfinished shaft.
    edged_blade('Staff_bottom_spike', [(0, -84), (2.4, -76), (1.5, -66), (0, -58),
                                       (-1.5, -66), (-2.4, -76)], materials, 1.9)
    ellipse('Staff_bottom_collar', (0, 0, -56), ((2.1, 0, 0), (0, 2.1, 0)),
            materials['Platina'], .45, 16)
    tube('Staff_blue_shaft', [(0, 0, z) for z in (-58, -16, 34, 88, 126)],
         [1.15, 1.05, 1.35, 1.1, 1.5], materials['Blue'], sides=12)
    tube('Staff_platina_grip', [(0, 0, -16), (0, 0, 2), (0, 0, 20)], 1.62,
         materials['Platina'], sides=12)
    for index in range(7):
        z = -13 + index * 5.4
        ellipse(f'Staff_grip_rib_{index}', (0, 0, z), ((1.68, 0, 0), (0, 1.68, 0)),
                materials['Blue'], .27, 12)
    for index, z in enumerate((-48, -24, 28, 54, 94, 118)):
        ellipse(f'Staff_ferrule_{index}', (0, 0, z), ((2, 0, 0), (0, 2, 0)),
                materials['Platina'], .4, 16)
    for z in (36, 50, 62, 76, 88):
        gem(f'Staff_shaft_diamond_{z}', (0, -1.5, z), (.7, .4, 2.2), materials['Emissive'])
    for sign, z in ((-1, 44), (1, 72)):
        tube(f'Staff_shaft_spine_{sign}', [(sign * 1.1, -1.0, w) for w in (-50, 40, 116)],
             [.2, .34, .12], materials['Platina'], sides=6)
        diamond = [(0, z + 5), (3, z), (0, z - 5), (-3, z)]
        edged_blade(f'Staff_shaft_escutcheon_{sign}', [(sign * x, w) for x, w in diamond],
                    materials, 1.6)
    # Crown: collar, big eight-point star, emissive core and lightning arcs.
    edged_blade('Staff_crown_collar', [(0, 138), (4.6, 130), (0, 122), (-4.6, 130)],
                materials, 3.0)
    ellipse('Staff_crown_collar_ring', (0, 0, 134), ((3.4, 0, 0), (0, 3.4, 0)),
            materials['Platina'], .5, 16)
    edged_blade('Staff_crown_star', star_outline(STAFF_CROWN, 26, 10, 8), materials, 2.6)
    for sign in (-1, 1):
        edged_blade(f'Staff_crown_minor_star_{sign}',
                    star_outline(STAFF_CROWN - 2, 9, 3.4, 4), materials, 1.5)
    gem('Staff_crown_core', (0, -2.6, STAFF_CROWN), (4.4, 2.3, 7.2), materials['Emissive'])
    gem('Staff_crown_rear_seal', (0, 2.4, STAFF_CROWN), (2.6, 1.1, 4.4), materials['Emissive'])
    for sign in (-1, 1):
        path = bezier([(sign * 8, -1.6, 136), (sign * 20, -2.2, 148),
                       (sign * 13, -1.8, 164), (sign * 3.5, -1, 170)], 12)
        tube(f'Staff_crown_arc_{sign}', path, [.55 - i * .035 for i in range(12)],
             materials['Platina'])
        inner = bezier([(sign * 5, -2.2, 132), (sign * 12, -2.6, 146),
                        (sign * 8, -2.4, 160), (sign * 2.4, -1.2, 166)], 10)
        tube(f'Staff_crown_inner_arc_{sign}', inner, [.4 - i * .03 for i in range(10)],
             materials['Emissive'])
