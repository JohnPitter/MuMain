"""Original Zeus cape silhouette, the "Manto do Herdeiro".

Authored in the player bind-pose world space (Z up, frame 0 of the frozen
player.bmd; front is -Y) exactly where the runtime link math places the cape
ferragem (zeus_cape_rig.link_matrix). Everything is rigid: the contract
(cape-cloth-contract.md) reserves the fabric mass to the runtime's procedural
cloth grids, so the BMD carries only the tall collar, the bolt emblem and the
pointed lightning-tail panels that read as the long blue cape of the concept.
Blue is the principal color of the rigid faces; platina carries structure and
edges; emissive is restricted to bolt channels and runes, per the family rule.

Cloth coordination: the runtime pins the main grid's top row on a straight line
0.6 x 180 wide, 8 units behind the neck base (bone 19), so the collar band
circles z 150..170 around the pin zone and the rigid panels stay behind
y >= 33, outside the cloth sweep (~20 units) and clear of the r=25..30
collision spheres on bones 17 and 2. Lightning tips are rigid geometry, never
cloth: a CPhysicsCloth grid is always a rectangle with a straight hem.
"""
from math import cos, sin
from math import tau as TAU

import bpy

from celestial_geometry import gem, mesh, rim, tube
from zeus_shapes import edged_blade, star_outline
from zeus_armor_shapes import bolt, mounted_outline, mounted_star, shift, zigzag

COLLAR_FACETS = 12
PANEL_PLANE = 34.0
TAIL_THICKNESS = 2.0


def collar_ring(mid_z, amp, radius_x, radius_y, phase=TAU / 24.0):
    """Faceted elliptical ring; sin>0 is the back (+y) and rides higher."""
    return [(radius_x * cos(phase + TAU * i / COLLAR_FACETS),
             radius_y * sin(phase + TAU * i / COLLAR_FACETS),
             mid_z + amp * sin(phase + TAU * i / COLLAR_FACETS))
            for i in range(COLLAR_FACETS)]


def facet_band(name, rings, material):
    """Angular (flat-shaded) band lofted over closed facet rings."""
    count = COLLAR_FACETS
    vertices = [point for ring in rings for point in ring]
    faces = []
    for row in range(len(rings) - 1):
        for i in range(count):
            a, b = row * count + i, row * count + (i + 1) % count
            faces.append((a, b, b + count, a + count))
    return mesh(name, vertices, faces, material)


def bolt_tip_wedge(name, base_x, base_y, base_z, reach, lean, m, width=2.6):
    """Angular bolt-tip crest point rising from the collar's top edge."""
    outline = [(-width, 0), (0, reach), (width, 0), (width * .45, -2.2),
               (-width * .45, -2.2)]
    mounted_outline(name, [(base_x + lean + x, base_z + z) for x, z in outline],
                    base_y, m, 1.4)


def collar(m):
    """Tall faceted platina collar with blue lining and storm channels.

    Covers the cloth pin zone (straight pinned row on the bone 19 anchor,
    world z around the neck base) and reads as the concept's alta gola:
    higher at the back, dipping toward the open front, five bolt-tip crests
    across the back rim.
    """
    outer = [collar_ring(150.5, 8.2, 15.4, 12.4), collar_ring(157.5, 8.8, 15.0, 12.0),
             collar_ring(164.5, 9.4, 14.4, 11.4)]
    facet_band('Cape_collar_platina_shell', outer, m['Platina'])
    facet_band('Cape_collar_blue_lining',
               [collar_ring(150.9, 8.2, 14.2, 11.2), collar_ring(157.5, 8.8, 13.8, 10.8),
                collar_ring(164.1, 9.4, 13.2, 10.2)], m['Blue'])
    for name, ring, radius in (('bottom', outer[0], .55), ('top', outer[2], .6)):
        tube(f'Cape_collar_platina_edge_{name}',
             ring + [ring[0]], radius, m['Platina'], sides=4)
    tube('Cape_collar_platina_ridge_mid',
         outer[1] + [outer[1][0]], .38, m['Platina'], sides=4)
    # Storm channel: emissive bolt climbing the back of the collar.
    mid = outer[1]
    back = sorted(sorted(mid, key=lambda p: p[1])[-4:], key=lambda p: p[0])
    bolt('Cape_collar_storm_channel',
         [(back[0][0], back[0][1] + 1.2, back[0][2] - 2),
          (back[1][0] * .45, back[1][1] + 1.6, (back[0][2] + back[1][2]) / 2 + 1.5),
          (back[2][0] * .45, back[2][1] + 1.6, (back[1][2] + back[2][2]) / 2 - 1.5),
          (back[3][0], back[3][1] + 1.2, back[3][2] + 2)],
         (0.5, 0.42, 0.5, 0.42), m['Emissive'])
    # Five bolt-tip crests across the back rim (concept: gola alta recortada).
    top = outer[2]
    for index, fraction in enumerate((-0.62, -0.31, 0.0, 0.31, 0.62)):
        angle = TAU / 4.0 + fraction * 0.72
        x = top[0][0] * 1.06 * cos(angle)
        y = top[0][1] * 1.06 * sin(angle)
        z = 164.5 + 9.4 * sin(angle)
        bolt_tip_wedge(f'Cape_collar_crest_{index}', x, y + 1.1, z,
                       7.5 - 2.2 * abs(fraction), 2.6 * fraction, m)
    gem('Cape_crown_gem_left', (-9.4, 10.6, 168.5), (1.1, .7, 2.0), m['Blue'])
    gem('Cape_crown_gem_right', (9.4, 10.6, 168.5), (1.1, .7, 2.0), m['Blue'])
    gem('Cape_crown_gem_center', (0, 12.0, 171.8), (1.3, .8, 2.4), m['Blue'])


def emblem_panel(m):
    """Center back shield with the 8-point star and the bolt brand."""
    previous = set(bpy.data.objects)
    edged_blade('Cape_emblem_shield',
                [(-11.5, 152.5), (-6, 146.5), (0, 144.5), (6, 146.5), (11.5, 152.5),
                 (7.5, 128), (3.5, 110), (0, 98), (-3.5, 110), (-7.5, 128)],
                m, 2.2)
    for obj in set(bpy.data.objects) - previous:
        shift(obj, (0, PANEL_PLANE, 0))
    mounted_star('Cape_emblem_star', 0, 138, 7.6, 3.4, 8, PANEL_PLANE + 1.15, m, 1.5)
    gem('Cape_emblem_star_core', (0, PANEL_PLANE + 1.7, 138), (1.5, .8, 1.5), m['Blue'])
    # The bolt brand: platina facet under an emissive lightning channel.
    bolt_polygon = [(1.6, 148.5), (-4.2, 133.5), (-0.8, 133.5), (-4.6, 118),
                    (-1.2, 118), (-6.2, 101.5), (1.8, 116.5), (-1.6, 116.5), (2.2, 132)]
    mounted_outline('Cape_emblem_bolt_facet',
                    [(x, z) for x, z in bolt_polygon], PANEL_PLANE + .75, m, 1.1)
    bolt('Cape_emblem_bolt_channel',
         zigzag(-.4, 104, 147, (2.8, 2.2, 2.6, 1.8), PANEL_PLANE + 3.4),
         (1.05, 0.9, 1.0, 0.9, 1.05, 0.9, 1.0, 0.9), m['Emissive'])
    for x, z in ((-9.2, 132), (9.2, 132)):
        rune(m, f'Cape_emblem_side_rune_{int(x)}', x, PANEL_PLANE + .8, z, 3.4)


def rune(m, name, x, y, z, reach):
    """Platina lozenge setting with a celestial gem: the Zeus cape rune."""
    lozenge = [(x, z + reach), (x + reach * .42, z), (x, z - reach), (x - reach * .42, z)]
    rim(f'{name}_setting', lozenge, y, m['Platina'], .4)
    gem(f'{name}_gem', (x, y + .9, z), (1.1, .7, reach * .62), m['Blue'])


def tail_panel(m, name, outline):
    """Pointed lightning-tail panel on the back plane (blue core, platina edge)."""
    previous = set(bpy.data.objects)
    edged_blade(name, outline, m, TAIL_THICKNESS)
    for obj in set(bpy.data.objects) - previous:
        shift(obj, (0, PANEL_PLANE, 0))


def tail_panels(m, side):
    """Three jagged lightning tails per side, the concept's caudas em ponta."""
    inner = [(side * 10.5, 151), (side * 18, 147.5), (side * 15, 139), (side * 23, 131),
             (side * 19, 121), (side * 26, 109), (side * 21.5, 97), (side * 28, 84),
             (side * 23.5, 58)]
    middle = [(side * 19.5, 149), (side * 28, 144.5), (side * 24.5, 135), (side * 33, 125),
              (side * 28.5, 113), (side * 37, 100), (side * 32, 88), (side * 39, 63)]
    outer = [(side * 28.5, 145), (side * 37.5, 139), (side * 34, 130), (side * 44, 119),
             (side * 39.5, 108), (side * 49, 95), (side * 44.5, 86), (side * 52, 74)]
    for name, outline in (('inner', inner), ('middle', middle), ('outer', outer)):
        tail_panel(m, f'Cape_tail_{name}_{side}', outline)
        tip = outline[-1]
        gem(f'Cape_tail_{name}_gem_tip_{side}', (tip[0], PANEL_PLANE + 1.5, tip[1] + 1.4),
            (1.2, .8, 2.8), m['Blue'])
        for bead, (bx, bz) in enumerate(outline[-3:-1]):
            gem(f'Cape_tail_{name}_bead_{side}_{bead}', (bx, PANEL_PLANE + 1.3, bz),
                (1.0, .7, 1.0), m['Platina'])
        # Blue rune channel: emissive zigzag down the tail's spine.
        spine = outline[1:-2]
        channel = [(x * 0.82, PANEL_PLANE + 1.25, z) for x, z in spine]
        bolt(f'Cape_tail_{name}_rune_channel_{side}', channel,
             tuple(.48 + .1 * (i % 2) for i in range(len(channel))), m['Emissive'])
    rune(m, f'Cape_tail_middle_rune_high_{side}', side * 27.5, PANEL_PLANE + .8, 122, 4.2)
    rune(m, f'Cape_tail_outer_rune_high_{side}', side * 39, PANEL_PLANE + .8, 117, 4.2)
    rune(m, f'Cape_tail_inner_rune_low_{side}', side * 19.5, PANEL_PLANE + .8, 90, 3.8)
    rune(m, f'Cape_tail_outer_rune_low_{side}', side * 42, PANEL_PLANE + .8, 100, 3.8)


def clasps(m):
    """Chest clasps: storm stars with celestial cores and the platina chain."""
    for side in (-1, 1):
        mounted_star(f'Cape_clasp_star_{side}', side * 7.2, 151.5, 4.6, 2.1, 6, -16.0, m, 1.4)
        gem(f'Cape_clasp_core_{side}', (side * 7.2, -16.8, 151.5), (1.4, .8, 2.6), m['Blue'])
        bolt(f'Cape_clasp_bolt_{side}',
             [(side * 7.2, -17.3, 154.4), (side * 8.6, -17.3, 152.6),
              (side * 6.0, -17.3, 150.6), (side * 7.2, -17.3, 148.2)],
             (.42, .38, .42, .38), m['Emissive'])
    tube('Cape_breast_platina_chain',
         [(-14.5, -13.0, 155.5), (-7.2, -14.6, 153.2), (0, -15.2, 152.4),
          (7.2, -14.6, 153.2), (14.5, -13.0, 155.5)], .8, m['Platina'], sides=8)
    gem('Cape_chain_gem_center', (0, -16.3, 152.0), (1.2, .8, 2.2), m['Blue'])


def cape(m):
    collar(m)
    emblem_panel(m)
    for side in (1, -1):
        tail_panels(m, side)
    clasps(m)
