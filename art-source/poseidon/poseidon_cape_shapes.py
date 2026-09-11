"""Original Poseidon black cape silhouette, the "Asas do Governante".

Authored in the player bind-pose world space (Z up, frame 0 of the frozen
player.bmd; front is -Y) exactly where the runtime link math places the cape
ferragem (poseidon_cape_rig.link_matrix). Everything is rigid: the contract
(cape-cloth-contract.md) reserves the fabric mass to the runtime's procedural
cloth grids, so the BMD carries only the collar, the gold structures and the
pointed rigid panels that read as the ruler's wings. Ocean blue stays restricted
to runes and jewels; gold is structure and fillet, never a covering.

Cloth coordination: the runtime pins the main grid's top row on a straight line
0.6 x 180 wide, 8 units behind the neck base (bone 19), so the collar band
circles z 150..170 outside that line and the rigid panels stay behind y >= 32,
outside the cloth sweep (~20 units) and clear of the r=25..30 collision spheres
on bones 17 and 2.
"""
from math import cos, sin
from math import tau as TAU

import bpy
from mathutils import Matrix, Vector

from celestial_geometry import bezier, ellipse, feather, gem, mesh, rim, tube
from poseidon_shapes import edged_blade
from poseidon_armor_shapes import mounted_blade, wave

COLLAR_SEGMENTS = 36
PANEL_PLANE = 34.0


def collar_ring(mid_z, amp, radius_x, radius_y):
    """Tilted elliptical ring; sin>0 is the back (+y) and rides higher."""
    return [(radius_x * cos(TAU * i / COLLAR_SEGMENTS),
             radius_y * sin(TAU * i / COLLAR_SEGMENTS),
             mid_z + amp * sin(TAU * i / COLLAR_SEGMENTS))
            for i in range(COLLAR_SEGMENTS)]


def band(name, rings, material):
    count = COLLAR_SEGMENTS
    vertices = [point for ring in rings for point in ring]
    faces = []
    for row in range(len(rings) - 1):
        for i in range(count):
            a, b = row * count + i, row * count + (i + 1) % count
            faces.append((a, b, b + count, a + count))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons:
        face.use_smooth = True
    return result


def collar(m):
    """Black padded collar with gold rims and lining, covering the cloth pin line."""
    band('Cape_collar_band_black',
         [collar_ring(158, 8, 14.8, 11.8), collar_ring(161.5, 8.5, 14.6, 11.6),
          collar_ring(165, 9, 14.4, 11.4)], m['Black'])
    tube('Cape_collar_gold_edge_bottom', collar_ring(157.6, 8, 14.9, 11.9), .55,
         m['Gold'], sides=8)
    tube('Cape_collar_gold_edge_top', collar_ring(165.4, 9.2, 14.35, 11.35), .6,
         m['Gold'], sides=8)
    tube('Cape_collar_gold_ridge_mid', collar_ring(161.8, 8.6, 15.1, 12.1), .38,
         m['Gold'], sides=8)
    band('Cape_collar_lining_gold',
         [collar_ring(158.2, 8, 13.7, 10.8), collar_ring(164.4, 8.8, 13.5, 10.6)],
         m['Gold'])
    for side in (-1, 1):
        wave(f'Cape_collar_wave_{side}',
             [(side * 3.5, -12.8, 152.5), (side * 8.5, -12, 155),
              (side * 12, -9.5, 159.5), (side * 8.5, -6.5, 164)], .55, m['Gold'])
        for index, turns in enumerate((0.08, 0.21, 0.34)):
            theta = TAU * turns
            gem(f'Cape_collar_pearl_stud_{side}_{index}',
                (14.5 * cos(theta) * side, 11.5 * sin(theta), 165.2 + 9 * sin(theta)),
                (.8, .8, 1), m['Pearl'])


def crests(m, side):
    """Gold wing crests sweeping up beside the head, the set's signature."""
    feathers = (
        ([(side * 11, -3, 162), (side * 20, -7, 172), (side * 27, -11, 184),
          (side * 31, -13, 197)], (2.6, .8, 14)),
        ([(side * 9, -1, 164), (side * 16, -5, 176), (side * 21, -8, 188),
          (side * 24.5, -9.5, 199)], (2.2, .7, 12)),
        ([(side * 13.5, -5, 160), (side * 22.5, -9, 171), (side * 29, -12, 181),
          (side * 33, -14, 191)], (2.1, .7, 12)),
        ([(side * 7, .5, 165), (side * 12, -3, 176), (side * 15, -5, 186),
          (side * 17, -6, 194)], (1.9, .6, 12)))
    for index, (controls, dimensions) in enumerate(feathers):
        feather(f'Cape_crest_blade_gold_{side}_{index}', controls, dimensions, m['Gold'])
        tube(f'Cape_crest_gold_spine_{side}_{index}',
             [tuple(Vector(c) + Vector((0, .9, 0))) for c in controls[:3]],
             (.42, .34, .08), m['Gold'], sides=6)
    gem(f'Cape_crest_ocean_gem_{side}', (side * 31.6, -13.4, 198.4), (1.7, 1, 3.8), m['Blue'])
    gem(f'Cape_crest_pearl_tip_{side}', (side * 25, -10, 200.2), (1, .7, 2.2), m['Pearl'])
    gem(f'Cape_crest_pearl_tip_high_{side}', (side * 33.4, -14.2, 192.4), (.9, .6, 2), m['Pearl'])
    gem(f'Cape_crest_pearl_tip_low_{side}', (side * 17.6, -6.3, 195.6), (.8, .6, 1.8), m['Pearl'])


def shoulder_bridge(m):
    """Gold yoke bridge over the trapezius, anchoring the crests visually."""
    tube('Cape_shoulder_gold_bridge',
         [(-19, -2, 159.5), (-8, -4.5, 163.5), (0, -4.8, 164.5),
          (8, -4.5, 163.5), (19, -2, 159.5)], 1.25, m['Gold'], sides=10)
    tube('Cape_shoulder_gold_bridge_under',
         [(-17, -1, 156.5), (-8, -3, 160), (0, -3.2, 161),
          (8, -3, 160), (17, -1, 156.5)], .8, m['Gold'], sides=8)
    for side in (-1, 1):
        gem(f'Cape_bridge_pearl_boss_{side}', (side * 19, -2.6, 160.6), (1.1, .7, 2.2), m['Pearl'])
        ellipse(f'Cape_bridge_gold_ring_{side}', (side * 19, -2.2, 159.4),
                ((2.6, 0, 0), (0, 1.6, 0)), m['Gold'], .45, 14)


def wing_panel(name, outline, m, thickness=1.9):
    """Pointed rigid panel on the back plane (PANEL_PLANE), gold-edged both faces."""
    previous = set(bpy.data.objects)
    edged_blade(name, outline, m, thickness)
    panel = list(set(bpy.data.objects) - previous)
    for obj in panel:
        obj.data.transform(Matrix.Translation((0, PANEL_PLANE, 0)))
        obj['poseidon_facing'] = 'back'
    return panel


def rune_setting(m, name, center, reach=5):
    """Gold lozenge setting with an ocean rune, on a panel's outer face."""
    cx, cy, cz = center
    lozenge = [(cx, cz + reach), (cx + reach * .44, cz), (cx, cz - reach), (cx - reach * .44, cz)]
    rim(f'{name}_gold_setting', lozenge, cy + .55, m['Gold'], .4)
    gem(f'{name}_ocean_rune', (cx, cy + 1.15, cz), (1.5, .85, reach * .72), m['Blue'])


def wing_panels(m, side):
    """Three rigid pointed panels per side, radiating down as the ruler's wings."""
    inner = [(side * 9.5, 150), (side * 14.5, 145.5), (side * 17.5, 147.5),
             (side * 19, 151), (side * 16.5, 118), (side * 12, 82), (side * 8.5, 47)]
    middle = [(side * 19, 148), (side * 25, 142.5), (side * 28.5, 145),
              (side * 30.5, 149), (side * 27, 114), (side * 21, 80), (side * 16, 56)]
    outer = [(side * 30, 144), (side * 37.5, 137.5), (side * 41.5, 140.5),
             (side * 44, 146), (side * 39.5, 110), (side * 31, 76), (side * 24.5, 66)]
    for name, outline in (('inner', inner), ('middle', middle), ('outer', outer)):
        wing_panel(f'Cape_wing_{name}_{side}', outline, m)
        tip = outline[-1]
        gem(f'Cape_wing_{name}_pearl_tip_{side}', (tip[0], 35.4, tip[1] + 1.6),
            (1.1, .7, 2.4), m['Pearl'])
        for bead, (bx, bz) in enumerate(outline[-3:-1]):
            gem(f'Cape_wing_{name}_gold_bead_{side}_{bead}', (bx, 35.3, bz),
                (1.2, .8, 1.2), m['Gold'])
    spine = [(side * 14.2, 146), (side * 14.6, 118), (side * 12.2, 88), (side * 9.4, 52)]
    tube(f'Cape_wing_inner_gold_spine_{side}',
         [(x, 35.6, z) for x, z in spine], (.5, .46, .38, .1), m['Gold'], sides=6)
    rune_setting(m, f'Cape_wing_inner_rune_high_{side}', (side * 13.5, 34.6, 128), 4.6)
    rune_setting(m, f'Cape_wing_middle_rune_high_{side}', (side * 23.6, 34.6, 124), 4.6)
    rune_setting(m, f'Cape_wing_outer_rune_high_{side}', (side * 34.2, 34.6, 120), 4.6)
    rune_setting(m, f'Cape_wing_middle_rune_low_{side}', (side * 22, 34.6, 92), 4)
    rune_setting(m, f'Cape_wing_outer_rune_low_{side}', (side * 32.5, 34.6, 90), 4)
    wave(f'Cape_wing_outer_wave_{side}',
         [(side * 30.5, 35.2, 138), (side * 38, 35.2, 128),
          (side * 37.5, 35.2, 112), (side * 30, 35.2, 92)], .5, m['Gold'])


def center_panel(m):
    """Spine panel with the trident brand and the central rune column."""
    wing_panel('Cape_wing_center',
               [(-8.5, 152), (-5, 148), (0, 146.5), (5, 148), (8.5, 152),
                (5.5, 120), (2.8, 84), (0, 58), (-2.8, 84), (-5.5, 120)], m, 2.1)
    mounted = mounted_blade('Cape_back_trident_brand',
                            [(0, 151), (2.4, 146), (1.5, 141), (2.8, 134.5), (0, 130),
                             (-2.8, 134.5), (-1.5, 141), (-2.4, 146)], 35.5, m, 1.3)
    for obj in mounted:
        obj['poseidon_facing'] = 'back'
    rune_setting(m, 'Cape_center_rune_crown', (0, 34.7, 142), 5)
    rune_setting(m, 'Cape_center_rune_heart', (0, 34.7, 112), 4.6)
    rune_setting(m, 'Cape_center_rune_root', (0, 34.7, 84), 4.2)
    gem('Cape_center_pearl_drop', (0, 34.4, 60.5), (1.2, .7, 2.6), m['Pearl'])
    wave('Cape_center_lower_wave',
         [(-6, 35.2, 128), (-3, 35.4, 104), (3, 35.4, 104), (6, 35.2, 128)], .45, m['Gold'])


def clasps(m):
    """Chest clasps: trident lozenges, pearl cores and the gold breast chain."""
    for side in (-1, 1):
        lozenge = [(side * 6.5, 156), (side * 9.3, 151.5), (side * 6.5, 147),
                   (side * 3.7, 151.5)]
        rim(f'Cape_clasp_gold_frame_{side}', lozenge, -16.1, m['Gold'], .5)
        gem(f'Cape_clasp_ocean_core_{side}', (side * 6.5, -16.7, 151.5), (1.6, .9, 3.2), m['Blue'])
        gem(f'Cape_clasp_pearl_kiss_{side}', (side * 6.5, -17.1, 151.5), (.7, .5, 1.3), m['Pearl'])
    tube('Cape_breast_gold_chain',
         [(-13.5, -13.2, 156), (-6.5, -14.6, 153.5), (0, -15.2, 152.8),
          (6.5, -14.6, 153.5), (13.5, -13.2, 156)], .85, m['Gold'], sides=8)
    gem('Cape_chain_pearl_center', (0, -16.4, 152.4), (1.3, .8, 2.4), m['Pearl'])


def cape(m):
    collar(m)
    shoulder_bridge(m)
    for side in (1, -1):
        crests(m, side)
        wing_panels(m, side)
    center_panel(m)
    clasps(m)
