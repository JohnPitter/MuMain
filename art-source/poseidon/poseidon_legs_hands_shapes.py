"""Original Poseidon Pants and Gloves silhouettes on the audited Class305 rig.

Continues the Helm/Armor/Boot visual language from poseidon_armor_shapes.py:
polished abyssal-black mass, antique-gold structure lines and wave fillets,
ocean-blue restricted to lozenge jewels, pearl-white left to future atlases.
Shapes are authored in the native bind-pose world space (Z-up, frame 0 of the
frozen Pant/GloveClass305.bmd skeletons) with single full influences, and the
binding bones are exactly the native distribution measured in the audit:
Pant -> 2/3/4/10/11/17/44, Glove -> 27/28/29/36/37/38.

Rear surfaces are authored facing front (-Y) and mirrored by rear_facing(), so
their recorded depths are negative like any front relief.
"""
from math import cos, sin
from math import tau as TAU

from mathutils import Matrix

from armor_surfaces import loft, section, segment_shell
from celestial_geometry import ellipse, feather, gem, mesh, plate, rim, tube
from poseidon_armor_shapes import (curved_shell, dome_shell, mounted_blade,
                                   rear_facing, wave)

WAIST_BONE, PELVIS_BONE, SKIRT_BONE = 17, 2, 44
THIGH_BONES = {1: 3, -1: 10}   # +X is the left leg in the native bind pose
CALF_BONES = {1: 4, -1: 11}
FOREARM_BONES = {1: 36, -1: 27}
HAND_BONES = {1: 37, -1: 28}
FINGER_BONES = {1: 38, -1: 29}


# ----------------------------------------------------------------- pants ----
def pants(m):
    section(WAIST_BONE, lambda: waistband(m))
    section(PELVIS_BONE, lambda: fauld(m))
    section(PELVIS_BONE, lambda: rear_facing(lambda: seat_plates(m)))
    for side in (1, -1):
        section(THIGH_BONES[side], lambda s=side: thigh_plates(m, s))
        section(THIGH_BONES[side], lambda s=side: rear_facing(lambda: rear_thigh(m, s)))
        section(CALF_BONES[side], lambda s=side: knee_cop(m, s))
    section(SKIRT_BONE, lambda: skirt_flaps(m))


def waistband(m):
    """Waist line that tucks under the authored cuirass (bottom ring at z=106)."""
    loft('Pants_waistline_black',
         [((0, 0, z), width, depth) for z, width, depth in
          ((100.8, 12.7, 9.95), (103.6, 12.2, 9.6), (106, 11.3, 8.95))], m['Black'])
    ellipse('Pants_waistline_gold_hem', (0, 0, 100.8), ((12.9, 0, 0), (0, 10.1, 0)),
            m['Gold'], .8, 16)
    mounted_blade('Pants_buckle_lozenge',
                  [(0, 105.6), (2.2, 103.3), (0, 101), (-2.2, 103.3)], -13.8, m, 1.3)
    gem('Pants_buckle_ocean_gem', (0, -14.9, 103.3), (1.2, .7, 2.5), m['Blue'])
    for side in (-1, 1):
        wave(f'Pants_waist_wave_gold_{side}',
             [(side * 10, -10.9, 102.4), (side * 12.3, -6.4, 103.2),
              (side * 12.5, 2, 102.6), (side * 10.4, 6.6, 101.7)], .5, m['Gold'])


def fauld(m):
    """Hip bell flaring below the waistband, bound to the pelvis only."""
    loft('Pants_hip_fauld_black',
         [((0, 0, z), width, depth) for z, width, depth in
          ((95.4, 13.6, 10.8), (98.2, 12.8, 10.2), (101, 12, 9.6))], m['Black'])
    ellipse('Pants_fauld_gold_hem', (0, 0, 95.4), ((13.7, 0, 0), (0, 10.9, 0)),
            m['Gold'], .8, 16)
    for side in (-1, 1):
        ellipse(f'Pants_hip_disc_{side}', (side * 12.9, -2.5, 98.6),
                ((0, 0, 2.4), (0, 2.4, 0)), m['Gold'], .5, 10)
        gem(f'Pants_hip_ocean_pin_{side}', (side * 12.9, -5.1, 98.6), (.8, .55, 1.6), m['Blue'])


def seat_plates(m):
    """Authored facing front; rear_facing() mirrors them onto the seat bell."""
    for index, (low, width, depth) in enumerate(((99.8, 5.6, -10.1), (95.8, 5.2, -10.9))):
        outline = [(-width, low + 1.4), (0, low + 2.2), (width, low + 1.4),
                   (width * .8, low - 2.4), (0, low - 3.2), (-width * .8, low - 2.4)]
        plate(f'Pants_seat_lamella_{index}', outline, (depth, 1.2), m['Black'])
        rim(f'Pants_seat_gold_edge_{index}', outline, depth - .5, m['Gold'], .45)
    feather('Pants_seat_spine_gold',
            [(0, -10.1, 101.8), (0, -10.8, 99.2), (0, -11.1, 96.6), (0, -11.4, 94.2)],
            (1.1, .5, 8), m['Gold'])


def thigh_plates(m, side):
    """Segmented metal bands over a dark under-sleeve, per native thigh bone."""
    axis = side * 10.45
    segment_shell(f'Pants_thigh_sleeve_black_{side}',
                  ((axis, -1.4, 108), (axis, -3.4, 66)),
                  [(0, 7.2, 6.9), (.35, 8.2, 7.7), (.7, 7.3, 6.9), (1, 5.8, 5.6)], m['Black'])
    for index, (low, width) in enumerate(((95.2, 8.6), (86.8, 8.2), (78.6, 7.6))):
        band = [(axis - side * 2.6, low + 3.6), (axis + side * 2, low + 4.6),
                (axis + side * width, low + 3.2), (axis + side * (width + 1), low + .8),
                (axis + side * (width * .82), low - 1.6), (axis - side * 1.4, low - 2.2)]
        curved_shell(f'Pants_thigh_lamella_{index}_{side}', band, -9.9, 2.2, m['Black'])
        rim(f'Pants_lamella_gold_edge_{index}_{side}', band, -10.6, m['Gold'], .5)
    wave(f'Pants_thigh_wave_gold_{side}',
         [(axis + side * 2.4, -10.2, 104), (axis + side * 7.6, -9.4, 97),
          (axis + side * 8.9, -6.8, 90), (axis + side * 6, -4.8, 83.5)], .5, m['Gold'])
    feather(f'Pants_thigh_front_ridge_gold_{side}',
            [(axis - side * .9, -8.9, 106), (axis + side * .3, -9.6, 98),
             (axis + side * .1, -9, 90), (axis - side * .7, -7.9, 84)],
            (1.2, .45, 8), m['Gold'])
    lozenge = [(axis + side * 5.2, 92.6), (axis + side * 8.4, 89.6),
               (axis + side * 5.2, 86.6), (axis + side * 2, 89.6)]
    rim(f'Pants_thigh_diamond_setting_{side}', lozenge, -9.4, m['Gold'], .45)
    gem(f'Pants_thigh_ocean_diamond_{side}', (axis + side * 5.2, -10.2, 89.6),
        (1.5, .8, 3.3), m['Blue'])


def rear_thigh(m, side):
    """Authored facing front; rear_facing() mirrors it onto the back of the thigh."""
    axis = side * 10.45
    for index, (low, width, depth) in enumerate(((99.6, 3.2, -6.1), (92.4, 2.8, -5.8))):
        outline = [(axis - width, low + 1.2), (axis, low + 2), (axis + width, low + 1.2),
                   (axis + width * .8, low - 2.2), (axis, low - 3), (axis - width * .8, low - 2.2)]
        plate(f'Pants_thigh_rear_plate_{index}_{side}', outline, (depth, 1.1), m['Black'])
        rim(f'Pants_thigh_rear_gold_edge_{index}_{side}', outline, depth - .5, m['Gold'], .4)


def knee_cop(m, side):
    """Bridging cop over the boot greave top (z=57), on the native calf bone."""
    center = side * 10.35
    dome_shell(f'Pants_knee_cop_{side}', (center, -1),
               [(57.6, 4.6, 4.4), (60.6, 5.6, 5.4), (63.6, 4.4, 4.2)], 12, m['Black'])
    ellipse(f'Pants_knee_gold_ring_{side}', (center, -1, 58.4), ((5.5, 0, 0), (0, 5.3, 0)),
            m['Gold'], .5, 12)
    gem(f'Pants_knee_ocean_gem_{side}', (center, -6.8, 61.2), (.9, .6, 1.8), m['Blue'])


def skirt_flaps(m):
    """Front fald hanging free on the native Bone02; never welded to the legs."""
    for index, (x, width, low, high) in enumerate(
            ((-7.4, 3.3, 89, 102.5), (0, 4.4, 87.5, 104), (7.4, 3.3, 89, 102.5))):
        outline = [(x - width, high), (x + width, high), (x + width * .8, low + 3.2),
                   (x, low), (x - width * .8, low + 3.2)]
        curved_shell(f'Pants_skirt_lamella_{index}', outline, -11.4, 1.4, m['Black'],
                     crown_rows=3, back_gap=.5)
        rim(f'Pants_skirt_gold_edge_{index}', outline, -12.2, m['Gold'], .45)
    for side in (-1, 1):
        wave(f'Pants_skirt_top_wave_gold_{side}',
             [(side * 4.4, -13.2, 102.8), (side * 6.2, -13.6, 104.2),
              (side * 8.2, -13.2, 103.4), (side * 9.4, -12.4, 101.6)], .45, m['Gold'])
    mounted_blade('Pants_skirt_lozenge', [(0, 98.4), (2, 95.4), (0, 92.4), (-2, 95.4)],
                  -13.6, m, 1.2)
    gem('Pants_skirt_ocean_gem', (0, -14.7, 95.4), (1.1, .7, 2.7), m['Blue'])


# ----------------------------------------------------------------- gloves ----
def gloves(m):
    for side in (1, -1):
        section(FOREARM_BONES[side], lambda s=side: gauntlet(m, s))
        section(FOREARM_BONES[side], lambda s=side: rear_facing(lambda: palm_guard(m, s)))
        section(HAND_BONES[side], lambda s=side: hand_guard(m, s))
        section(FINGER_BONES[side], lambda s=side: claws(m, s))


def gauntlet(m, side):
    """Flared cuff over the elbow, tapering shell, dorsal plates and jewels."""
    segment_shell(f'Glove_gauntlet_shell_black_{side}',
                  ((side * 26.2, .2, 122.5), (side * 31.2, -.9, 89.5)),
                  [(0, 4.9, 4.6), (.3, 5.8, 5.4), (.65, 6.2, 5.8), (1, 5.2, 4.9)], m['Black'])
    segment_shell(f'Glove_cuff_flare_black_{side}',
                  ((side * 25.8, .3, 120.2), (side * 25, .5, 126.8)),
                  [(0, 5.3, 5), (1, 7, 6.6)], m['Black'])
    ellipse(f'Glove_cuff_gold_rim_{side}', (side * 25, .5, 126.8), ((7.1, 0, 0), (0, 6.7, 0)),
            m['Gold'], .7, 14)
    for index, (high, low, depth) in enumerate(((117.5, 108.5, -7.2), (109.5, 100, -7.8),
                                                (101, 92, -8.2))):
        outline = [(side * 27.2, high), (side * 30.2, high - 1.6), (side * 31.7, (high + low) / 2),
                   (side * 30.4, low + 1.6), (side * 27, low)]
        plate(f'Glove_dorsal_plate_{index}_{side}', outline, (depth, 1), m['Black'])
        tube(f'Glove_plate_gold_chevron_{index}_{side}',
             [(side * 28.4, depth - .9, low + 2.4), (side * 29.7, depth - .9, low + 3.3),
              (side * 31, depth - .9, low + 2.4)], .38, m['Gold'], sides=6)
    wave(f'Glove_outer_wave_gold_{side}',
         [(side * 27.4, -5.6, 118.5), (side * 32.2, -6.2, 110),
          (side * 34.6, -5.4, 101), (side * 32.6, -3.4, 93.5)], .45, m['Gold'])
    lozenge = [(side * 28.7, 106.4), (side * 30.8, 104.7), (side * 28.7, 103), (side * 26.6, 104.7)]
    rim(f'Glove_dorsal_diamond_setting_{side}', lozenge, -8.9, m['Gold'], .4)
    gem(f'Glove_dorsal_ocean_diamond_{side}', (side * 28.7, -9.7, 104.7), (1.1, .7, 2.2), m['Blue'])


def palm_guard(m, side):
    """Authored facing front; rear_facing() mirrors it onto the palm side."""
    outline = [(side * 28.4, 87.2), (side * 31, 86.4), (side * 31.8, 83.4),
               (side * 30.6, 80.6), (side * 28.2, 80.2), (side * 27.2, 83.8)]
    plate(f'Glove_palm_guard_black_{side}', outline, (-(6.4), 1), m['Black'])
    rim(f'Glove_palm_gold_border_{side}', outline, -6.9, m['Gold'], .4)


def hand_guard(m, side):
    """Segmented hand shell with a raised dorsal plate; palm stays usable."""
    center = side * 33.4
    profile = ((88.6, 4.4, 4.2), (85.5, 5, 4.7), (82.5, 4.7, 4.5), (79.8, 3.6, 3.5))
    sections = [hand_section(side, center, row) for row in profile]
    vertices = [point for points in sections for point in points]
    count = len(sections[0])
    faces = []
    for row in range(len(sections) - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    faces.append(tuple(range(count - 1, -1, -1)))
    base = (len(sections) - 1) * count
    faces.append(tuple(base + index for index in range(count)))
    result = mesh(f'Glove_hand_shell_black_{side}', vertices, faces, m['Black'])
    for face in result.data.polygons:
        face.use_smooth = True
    guard = [(center - 2.6, 87.2), (center, 87.8), (center + 2.6, 87.2),
             (center + 2.2, 82.8), (center, 81.2), (center - 2.2, 82.8)]
    plate(f'Glove_hand_dorsal_guard_{side}', guard, (-5.6, 1), m['Black'])
    rim(f'Glove_guard_gold_border_{side}', guard, -6.3, m['Gold'], .45)
    lozenge = [(center - 1.4, 85.6), (center, 84.4), (center + 1.4, 85.6), (center, 86.8)]
    rim(f'Glove_hand_diamond_setting_{side}', lozenge, -6.1, m['Gold'], .35)
    gem(f'Glove_hand_ocean_diamond_{side}', (center, -6.8, 85.6), (.9, .6, 1.7), m['Blue'])
    tube(f'Glove_knuckle_bar_gold_{side}',
         [(center - 3, -5, 80.4), (center, -5.4, 79.7), (center + 3, -5, 80.4)],
         .5, m['Gold'], sides=6)
    ellipse(f'Glove_wrist_gold_collar_{side}', (side * 31.5, -.8, 89.4),
            ((5.5, 0, 0), (0, 5.2, 0)), m['Gold'], .7, 12)


def hand_section(side, center, row):
    z, width, depth = row
    return [(center - width * .9, -depth * .55, z), (center - width, -depth * .2, z),
            (center - width * .62, depth * .8, z), (center, depth, z),
            (center + width * .62, depth * .8, z), (center + width, -depth * .2, z),
            (center + width * .9, -depth * .55, z)]


def claws(m, side):
    """Segmented fingers ending in gold sea-claws, on the native Finger0 bone."""
    for index, (x, y, top, length) in enumerate(
            ((side * 34.9, -2.3, 79.4, 8.2), (side * 36.9, -1.7, 78.8, 9),
             (side * 38.7, -.7, 77.6, 8.4))):
        mid = (x + side * .5, y - .3, top - length * .55)
        tip = (x + side * 1.1, y - .9, top - length)
        tube(f'Glove_finger_segment_{index}_{side}', [(x, y, top), mid], [1.05, .8],
             m['Black'], sides=6)
        tube(f'Glove_claw_tip_gold_{index}_{side}', [mid, tip], [.6, .06], m['Gold'], sides=5)
    tube(f'Glove_thumb_segment_{side}', [(side * 32.4, -3.5, 82.6), (side * 33.6, -4.7, 78.8)],
         [1.05, .8], m['Black'], sides=6)
    tube(f'Glove_thumb_claw_gold_{side}', [(side * 33.6, -4.7, 78.8), (side * 34.6, -5.7, 75.6)],
         [.6, .06], m['Gold'], sides=5)
