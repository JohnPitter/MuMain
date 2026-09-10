"""Reference-specific silhouettes: solar staff, winged kite shield, and jewelry."""
import math

import bpy

from celestial_geometry import ellipse, feather, gem, plate, rim, tendril, tube


def staff_shaft(m):
    tube('Staff / tapered ivory shaft', [(0, 0, z) for z in (-87, -76, -40, 0, 35, 63)],
         [0.15, 0.85, 0.95, 1.25, 1.0, 1.65], m['Ivory'], sides=12)
    for z in (-75, -49, -21, 0, 24, 47, 61):
        ellipse('Staff / gold ferrule', (0, 0, z), ((1.5, 0, 0), (0, 1.5, 0)),
                m['Gold'], width=0.45, steps=16)
    for phase in (0, math.pi):
        path = [(1.2 * math.cos(t * 5 * math.tau + phase),
                 1.2 * math.sin(t * 5 * math.tau + phase), -70 + t * 132)
                for t in (i / 120 for i in range(121))]
        tube('Staff / helical chased gold', path, 0.22, m['Gold'], sides=5)
    for side in (-1, 1):
        feather('Staff / lower spear blade', [(0, 0, -56), (side * 2, 0, -65),
                (side * 1.5, 0, -78), (0, 0, -91)], (0.9, 0.35), m['Gold'])


def staff_halo(m):
    center = (0, 0, 86)
    ellipse('Staff / outer solar ring', center, ((17.7, 0, 0), (0, 0, 21)), m['Gold'], 0.7)
    ellipse('Staff / inner solar ring', (0, -0.7, 86), ((14.5, 0, 0), (0, 0, 18)),
            m['Gold'], 0.4)
    ellipse('Staff / orbital meridian', center, ((10, 5.0, 0), (0, 0, 19.5)), m['Ivory'], 0.55)
    for i in range(12):
        a = math.tau * i / 12
        x, z = 17.7 * math.sin(a), 86 + 21 * math.cos(a)
        extension = 8 if i % 3 == 0 else 4.5
        end = ((17.7 + extension) * math.sin(a), 0, 86 + (21 + extension) * math.cos(a))
        feather('Staff / solar ray', [(x * .83, 0, 86 + (z - 86) * .83),
                (x, 0, z), (end[0], 0, end[2] - 1), end], (1.15, 0.55), m['Gold'])
        gem('Staff / ray diamond', (x, -0.8, z), (0.65, 0.5, 1.2), m['Emissive'])


def staff(m):
    staff_shaft(m)
    staff_halo(m)
    for side in (-1, 1):
        tendril('Staff / rising split prong', [(side * 1.2, -0.5, 52),
                (side * 27, 0, 74), (side * 4, 0, 99), (side * 9, 0, 121)], 1.1, m['Gold'])
        feather('Staff / ivory lance', [(side * 3, -0.9, 63), (side * 13, -1, 81),
                (side * 8, -1, 99), (side * 11, -1, 113)], (2.4, 0.7), m['Ivory'])
        tendril('Staff / scrolling bridge', [(side * 2, -1.5, 76),
                (side * 16, -2, 78), (side * 14, -2, 97), (0, -1, 100)], 0.6, m['Gold'])
    feather('Staff / long crown spear', [(0, 0, 95), (0, 0, 103),
            (0, 0, 115), (0, 0, 128)], (1.85, 0.8), m['Gold'])
    diamond = [(0, 98), (5.5, 86), (0, 74), (-5.5, 86)]
    rim('Staff / gemstone bezel', diamond, -3.0, m['Gold'], 0.65)
    gem('Staff / cyan heart', (0, -3, 86), (4.5, 2.6, 10), m['Sapphire'])
    gem('Staff / white core', (0, -5.7, 86), (1.15, 0.3, 3.5), m['Emissive'])


def shield(m):
    outline = [(0, 58), (14, 45), (26, 35), (25, 15), (17, -7),
               (0, -43), (-17, -7), (-25, 15), (-26, 35), (-14, 45)]
    plate('Shield / crowned kite', outline, (0, 6.0), m['Ivory'])
    rim('Shield / thick gilded border', outline, -0.7, m['Gold'], 1.0)
    inner = [(x * .8, (z - 6) * .8 + 6) for x, z in outline]
    rim('Shield / inner border', inner, -4, m['Gold'], 0.65)
    for side in (-1, 1):
        for i in range(4):
            feather('Shield / overlapping ivory wing', [(side * 4, -7, 24 - i * 8),
                    (side * 12, -6, 22 - i * 9), (side * (31 - i * 3), -3, 43 - i * 12),
                    (side * (27 - i * 3), -2, 53 - i * 14)], (3.2, 1.1), m['Ivory'])
            tendril('Shield / gilded wing spine', [(side * 3, -8.2, 25 - i * 8),
                    (side * 12, -7.5, 21 - i * 9), (side * (30 - i * 3), -4, 42 - i * 12),
                    (side * (27 - i * 3), -3, 53 - i * 14)], 0.55, m['Gold'])
        tendril('Shield / upper crown', [(side * 9, -6, 30), (side * 18, -5, 42),
                (side * 14, -3, 49), (side * 13, -2, 60)], 1.0, m['Gold'])
        tendril('Shield / lower fleur', [(0, -7, -31), (side * 22, -5, -2),
                (side * 5, -8, 12), (0, -9, 3)], 0.75, m['Gold'])
    gem('Shield / central azure diamond', (0, -8.5, 26), (6, 2.8, 13), m['Sapphire'])
    rim('Shield / central bezel', [(0, 41), (8, 26), (0, 11), (-8, 26)], -8.1, m['Gold'], .85)
    feather('Shield / raised axial keel', [(0, -7, 13), (0, -8, -1),
            (0, -3, -28), (0, -1, -43)], (2.4, 1.0), m['Gold'])
    ellipse('Shield / rear grip', (0, 7, 7), ((10, 0, 0), (0, 0, 16)), m['Gold'], 1.5)
    shield_ornaments(m)


def shield_ornaments(m):
    for side in (-1, 1):
        for i in range(4):
            z = 33 - i * 16
            outline = [(side * (shield_width(height) + inset), height)
                       for height, inset in ((z + 7, -3), (z + 2, .5),
                                             (z - 13, -2), (z - 2, -7))]
            plate('Shield / gilded side scales', outline, (-3, 1.3), m['Gold'])
        for i in range(3):
            z = 27 - i * 13
            tendril('Shield / openwork acanthus', [(side * 6, -10, z),
                    (side * 16, -9, z + 12), (side * 23, -8, z + 3),
                    (side * 14, -8, z - 2)], .5, m['Gold'])
            gem('Shield / filigree clasp', (side * 12, -9, z), (1.3, .5, 2.5), m['Gold'])
        feather('Shield / crown leaf', [(side * 5, -6, 35), (side * 16, -6, 39),
                (side * 12, -4, 53), (side * 13, -2, 62)], (3.1, .8), m['Gold'])
    gem('Shield / crown star', (0, -3, 51), (3, 1.2, 7), m['Sapphire'])
    rim('Shield / crown bezel', [(0, 58), (3.3, 51), (0, 44), (-3.3, 51)],
        -3.2, m['Gold'], .5)


def shield_width(z):
    profile = [(-43, 0), (-7, 17), (15, 25), (35, 26), (45, 14), (58, 0)]
    for (low, left), (high, right) in zip(profile, profile[1:]):
        if low <= z <= high:
            return left + (right - left) * (z - low) / (high - low)
    raise ValueError(f'Shield ornament outside outline: {z}')


def jewel_crest(m, prefix, factor=1):
    outline = [(0, 10), (6, 1), (4.5, -4), (0, -10), (-4.5, -4), (-6, 1)]
    outline = [(x * factor, z * factor) for x, z in outline]
    plate(prefix + ' / ivory seat', outline, (-1.5 * factor, factor), m['Ivory'])
    rim(prefix + ' / gold bezel', outline, -2.2 * factor, m['Gold'], .45 * factor)
    gem(prefix + ' / azure lozenge', (0, -3.0 * factor, 0),
        (2.7 * factor, 1.3 * factor, 5 * factor), m['Sapphire'])
    rim(prefix + ' / azure bezel', [(x * factor, z * factor)
        for x, z in ((0, 5.4), (3.2, 0), (0, -5.4), (-3.2, 0))],
        -3.2 * factor, m['Gold'], .28 * factor)
    for side in (-1, 1):
        for i in range(3):
            controls = [(side * 1.8, -3, -4 + i * 3), (side * 8, -2, -3 + i * 3),
                        (side * 4.5, -1, 5 + i * 2), (side * (5 - i), -1, 8 + i * 2)]
            controls = [tuple(c * factor for c in point) for point in controls]
            feather(prefix + ' / feather crown', controls, (factor, .4 * factor), m['Gold'])
        for i in range(3):
            z = -5 + i * 4
            controls = [(side * 3, -3, z), (side * 8, -3, z + 2),
                        (side * 5, -3, z + 5), (side * 3.5, -3, z + 2)]
            controls = [tuple(c * factor for c in point) for point in controls]
            tendril(prefix + ' / pierced leafwork', controls, .3 * factor, m['Gold'])
    for z in (-7, 7):
        gem(prefix + ' / star clasp', (0, -3 * factor, z * factor),
            (1.0 * factor, .5 * factor, 1.8 * factor), m['Gold'])


def pendant(m):
    jewel_crest(m, 'Pendant')
    gem('Pendant / hanging gold drop', (0, -1.5, -11.7), (1.3, .7, 2.7), m['Gold'])
    ellipse('Pendant / bail', (0, 0, 11.3), ((1.5, 0, 0), (0, 0, 2.2)), m['Gold'], .4, 24)
    for side in (-1, 1):
        for i in range(11):
            t = i / 10
            x, z = side * (.8 + t * 12), 12.2 + t * 9 + t * t * 3
            ellipse('Pendant / chain link', (x, .35 * (i % 2), z),
                    ((.65, .3 * (i % 2), -.4 * side), (.65 * side, 0, .85)),
                    m['Gold'], .17, 12)


def ring(m):
    ellipse('Ring / outer band', (0, 4, -3), ((7.5, 0, 0), (0, 0, 7.5)), m['Gold'], 1.3)
    ellipse('Ring / upper engraved edge', (0, 2.9, -3), ((7.6, 0, 0), (0, 0, 7.6)),
            m['Gold'], .45)
    ellipse('Ring / lower engraved edge', (0, 5.1, -3), ((7.6, 0, 0), (0, 0, 7.6)),
            m['Gold'], .45)
    previous = set(bpy.data.objects)
    jewel_crest(m, 'Ring', .62)
    for obj in set(bpy.data.objects) - previous:
        obj.rotation_euler.x = -math.pi / 2
        obj.location = (0, 4, 5)
    for side in (-1, 1):
        for i in range(2):
            feather('Ring / shoulder inlay', [(side * 2, 2 + i * 3, 5), (side * 7, 2 + i * 3, 4),
                    (side * 9, 3, -2 - i * 2), (side * 7, 4, -5 - i * 2)],
                    (1.2, .3), m['Ivory'])


BUILDERS = {'Staff': staff, 'Shield': shield, 'Pendant': pendant, 'Ring': ring}
