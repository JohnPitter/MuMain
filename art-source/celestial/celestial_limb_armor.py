"""Articulated trousers, split skirt panels, gauntlets and greaves."""
import math
from armor_surfaces import loft, section, segment_shell
from celestial_geometry import ellipse, feather, gem, mesh, plate, rim, tendril, tube


def trousers(m):
    section(2, lambda: waist(m))
    for side, thigh in ((-1, 10), (1, 3)):
        section(thigh, lambda s=side: thigh_plate(m, s))
        section(lambda obj, p, b=thigh: b if p.z > 69 else b + 1,
                lambda s=side: knee(m, s))
    section(lambda obj, p: 2 if p.z > 109 else 44 if p.z > 102 else 45 if p.z > 77 else 46,
            lambda: tabards(m, -1))
    section(lambda obj, p: 2 if p.z > 110 else 48 if p.z > 102 else 49 if p.z > 77 else 50,
            lambda: tabards(m, 1))


def waist(m):
    loft('Pants / articulated hip girdle', [((0, 0, 100), 15, 10),
         ((0, 0, 112), 17, 11), ((0, 0, 119), 13, 9)], m['Ivory'])
    ellipse('Pants / waist band', (0, 0, 115), ((15, 0, 0), (0, 10, 0)), m['Gold'], 1, 32)
    gem('Pants / belt star', (0, -12, 113), (3.1, 1.2, 5.7), m['Sapphire'])
    for side in (-1, 1):
        plate('Pants / hip fauld', [(side * 11, 116), (side * 24, 112),
              (side * 23, 91), (side * 15, 96)], (-6, 5), m['Gold'])


def thigh_plate(m, side):
    segment_shell('Pants / thigh casing', ((side * 10.3, 0, 109), (side * 10.3, -4.9, 64)),
                  [(0, 10, 10), (.3, 10.5, 10), (.7, 8, 8), (1, 7, 7)], m['Ivory'])
    for i in range(3):
        outline = [(side * 4, 108 - i * 11), (side * 18, 106 - i * 11),
                   (side * 16, 91 - i * 9), (side * 11, 86 - i * 9), (side * 6, 93 - i * 10)]
        plate('Pants / overlapping cuisse', outline, (-9 - i * .6, 1.8), m['Ivory'])
        rim('Pants / cuisse edge', outline, -9.5 - i * .6, m['Gold'], .5)


def knee(m, side):
    diamond = [(side * 10.3, 78), (side * 19, 67), (side * 10.3, 56), (side * 2, 67)]
    plate('Pants / knee couter', diamond, (-13, 2.6), m['Gold'])
    gem('Pants / knee ivory inlay', (side * 10.3, -15.2, 67), (3.5, 1, 6.5), m['Ivory'])


def tabards(m, front):
    for side in (-1, 1):
        tabard_surface(m, side, front)
        for i in range(5):
            t = .09 + i * .16
            for direction in (-1, 1):
                samples = ((.5, t), (.5 + direction * .38, t + .03),
                           (.5 + direction * .43, t + .12), (.5, t + .13))
                path = [panel_point(u, v, (side, front), .6) for u, v in samples]
                tendril('Pants / branching acanthus embroidery', path, .32, m['Gold'])


def panel_point(u, t, sides, lift=0):
    side, front = sides
    x = side * (3 + 11 * t + u * (11 + 5 * t))
    z = 111 - (79 + 14 * u) * t
    y = front * (14 + 3 * t + 1.6 * math.sin(u * math.tau) + lift)
    return x, y, z


def tabard_surface(m, side, front):
    rows, columns = 17, 7
    top = [panel_point(u / (columns - 1), t / (rows - 1), (side, front))
           for t in range(rows) for u in range(columns)]
    vertices = top + [(x, y - front * .5, z) for x, y, z in top]
    faces, count = [], len(top)
    for row in range(rows - 1):
        for column in range(columns - 1):
            a = row * columns + column
            face = (a, a + 1, a + 1 + columns, a + columns)
            faces.extend((face, tuple(count + i for i in reversed(face))))
    boundary = list(range(columns)) + [r * columns + columns - 1 for r in range(1, rows)]
    boundary += list(range(count - 2, count - columns - 1, -1))
    boundary += [r * columns for r in range(rows - 2, 0, -1)]
    for a, b in zip(boundary, boundary[1:] + boundary[:1]):
        faces.append((a, count + a, count + b, b))
    mesh('Pants / draped split tabard', vertices, faces, m['Ivory'])
    edge = [(top[i][0], top[i][1] + front * .3, top[i][2]) for i in boundary]
    tube('Pants / continuous tabard edge', edge + edge[:1], .5, m['Gold'], 6)


def gloves(m):
    for side, forearm, hand, finger in ((-1, 27, 28, 29), (1, 36, 37, 38)):
        section(forearm, lambda s=side: vambrace(m, s))
        section(hand, lambda s=side: hand_plate(m, s))
        for segment in range(3):
            section(finger + segment, lambda s=side, n=segment: fingers(m, s, n))


def vambrace(m, side):
    segment_shell('Gloves / shaped vambrace', ((side * 25.5, 0, 128), (side * 31.7, -.7, 93)),
                  [(0, 7.8, 8), (.25, 8, 7.5), (.65, 5, 5), (1, 4.5, 4.2)], m['Ivory'])
    outline = [(side * 26, 132), (side * 34, 122), (side * 36, 106),
               (side * 31, 98), (side * 25, 113), (side * 20, 122)]
    plate('Gloves / raised forearm plate', outline, (-6, 2), m['Ivory'])
    rim('Gloves / forearm bezel', outline, -6.6, m['Gold'], .55)
    gem('Gloves / blue forearm inset', (side * 27, -8, 121), (2.1, 1, 5.3), m['Sapphire'])
    for i in range(2):
        feather('Gloves / outer gilded blade', [(side * 31, 0, 100 + i * 5),
                (side * 41, -2, 110 + i * 5), (side * 39, 0, 122 + i * 4),
                (side * 36, 0, 136 + i * 3)], (1.7, .5), m['Gold'])


def hand_plate(m, side):
    outline = [(side * 28, 96), (side * 36, 95), (side * 41, 85),
               (side * 38, 80), (side * 31, 83)]
    plate('Gloves / metacarpal plate', outline, (-3, 3), m['Ivory'])
    rim('Gloves / hand gold edge', outline, -3.5, m['Gold'], .45)
    for i in range(4):
        tube('Gloves / knuckle ridge', [(side * (31 + i * 1.8), -5.5, 88),
             (side * (33 + i * 1.8), -5.5, 84)], .38, m['Gold'], 6)
    tube('Gloves / thumb armor', [(side * 30, -1, 91), (side * 28, -5, 88),
         (side * 29, -6, 84)], [1.5, 1.5, .8], m['Ivory'], 8)


def fingers(m, side, segment):
    for i in range(4):
        x = side * (33 + i * 1.9 + segment * .7)
        z = 84 - segment * 3.5 - abs(i - 1.5) * .6
        tube('Gloves / articulated finger scale', [(x, -1.5, z), (x + side * .7, -1.5, z - 3.3)],
             [1, .8], m['Ivory'], 6)
        tube('Gloves / finger gold rim', [(x + side * .1, -2.5, z - .6),
             (x + side * .6, -2.5, z - 1.2)], .22, m['Gold'], 5)


def boots(m):
    for side, calf, foot in ((-1, 11, 12), (1, 4, 5)):
        section(calf, lambda s=side: greave(m, s))
        section(foot, lambda s=side: sabaton(m, s))


def greave(m, side):
    segment_shell('Boots / tapered greave', ((side * 10.3, 0, 13), (side * 10.3, -4.9, 63)),
                  [(0, 5.5, 6), (.25, 7, 7), (.65, 9, 9), (1, 9, 9)], m['Ivory'])
    outline = [(side * 10.3, 75), (side * 21, 59), (side * 16, 30),
               (side * 13, 15), (side * 7, 15), (side * 3, 57)]
    plate('Boots / pointed shin plate', outline, (-10, 3), m['Ivory'])
    rim('Boots / shin gold frame', outline, -10.5, m['Gold'], .7)
    gem('Boots / shin blue jewel', (side * 10.3, -13.2, 54), (2.6, 1, 6), m['Sapphire'])
    for offset in (-1, 1):
        tendril('Boots / flowing gold inlay', [(side * 10.3, -12, 21),
                (side * 10.3 + offset * 6, -13, 38),
                (side * 10.3 + offset * 7, -12, 56),
                (side * 10.3, -11, 70)], .55, m['Gold'])


def sabaton(m, side):
    segment_shell('Boots / pointed sabaton', ((side * 10.3, 6, 10), (side * 10.3, -25, 5)),
                  [(0, 6, 5), (.3, 7, 6), (.7, 5.5, 4.5), (1, .4, .5)], m['Ivory'])
    for offset in (-1, 1):
        tendril('Boots / toe gold rail', [(side * 10.3, -25, 5),
                (side * 10.3 + offset * 7, -18, 11),
                (side * 10.3 + offset * 6, -6, 16),
                (side * 10.3 + offset * 5, 4, 12)], .55, m['Gold'])


BUILDERS = {'Pants': trousers, 'Gloves': gloves, 'Boots': boots}
