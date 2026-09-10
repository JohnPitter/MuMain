"""Crown, layered cuirass and split tabard authored from the supplied concept."""
import math

from armor_surfaces import loft, section, segment_shell
from celestial_geometry import ellipse, feather, gem, mesh, plate, rim, tendril


def helmet_shell(m):
    levels = [(158, 11, 12), (176, 14, 20), (186, 13.5, 18), (194, 8, 12), (197, 1, 1)]
    vertices = [(rx * math.cos(a), ry * math.sin(a), z)
                for z, rx, ry in levels for a in (math.tau * i / 24 for i in range(24))]
    faces = []
    for row in range(len(levels) - 1):
        for i in range(24):
            angle = math.tau * (i + .5) / 24
            if row == 0 and math.sin(angle) < -.25:
                continue
            a, b = row * 24 + i, row * 24 + (i + 1) % 24
            faces.append((a, b, b + 24, a + 24))
    mesh('Helm / open-face crown shell', vertices, faces, m['Ivory'])


def helmet(m):
    section(20, lambda: helmet_details(m))


def helmet_details(m):
    helmet_shell(m)
    gem('Helm / forehead diamond', (0, -23, 186), (3.3, 1.7, 7.5), m['Sapphire'])
    rim('Helm / diamond setting', [(0, 194), (4, 186), (0, 178), (-4, 186)], -23, m['Gold'], .4)
    for side in (-1, 1):
        brow = [(side * 14, 179), (side * 8, 194), (0, 188), (0, 178), (side * 6, 173)]
        plate('Helm / split brow crest', brow, (-20, 1.5), m['Gold'])
        cheek = [(side * 11, 181), (side * 13, 174), (side * 8, 158), (side * 6, 171)]
        plate('Helm / pointed cheek guard', cheek, (-16, 2), m['Ivory'])
        rim('Helm / cheek edging', cheek, -16.5, m['Gold'], .5)
        for i in range(3):
            controls = [(side * (8 + i * 2), i * 2, 177),
                        (side * (19 + i * 2), i * 2, 183),
                        (side * (12 + i * 2), i * 2, 196),
                        (side * (10 + i * 4), i * 2, 213 - i * 6)]
            feather('Helm / swept crown lance', controls, (2.0 - i * .25, .6), m['Gold'])
        feather('Helm / temple ivory blade', [(side * 10, -8, 173), (side * 20, -3, 182),
                (side * 19, 1, 188), (side * 17, 2, 196)], (2.1, .7), m['Ivory'])
        tendril('Helm / brow tracery', [(side * 2, -23, 180), (side * 17, -22, 188),
                (side * 14, -20, 176), (side * 9, -18, 171)], .38, m['Gold'])
    feather('Helm / axial crown', [(0, 0, 184), (0, 0, 192), (0, 0, 205),
            (0, 0, 218)], (2.1, .7), m['Gold'])


def torso_bone(_obj, position):
    return 18 if position.z >= 133 else 17 if position.z >= 117 else 2


def armor(m):
    section(torso_bone, lambda: torso(m))
    for side, bone in ((-1, 26), (1, 35)):
        section(bone, lambda s=side: shoulder(m, s))
    section(17, lambda: rear_crest(m))


def torso(m):
    rings = [((0, 0, z), x, y) for z, x, y in
             ((112, 13, 9), (122, 12, 8.5), (135, 20, 12), (147, 22, 12), (155, 16, 8.5))]
    loft('Armor / anatomical cuirass', rings, m['Ivory'])
    ellipse('Armor / standing collar', (0, 0, 157), ((10, 0, 0), (0, 8, 0)), m['Gold'], .9, 32)
    for side in (-1, 1):
        chest = [(side * 2, 151), (side * 16, 158), (side * 22, 148),
                 (side * 16, 135), (side * 2, 132)]
        plate('Armor / sculpted pectoral', chest, (-11, 3.5), m['Ivory'])
        rim('Armor / pectoral border', chest, -12, m['Gold'], .65)
        for i in range(3):
            feather('Armor / rib lamella', [(side * 2, -11, 124 - i * 5),
                    (side * 9, -13, 128 - i * 4), (side * 17, -8, 129 - i * 3),
                    (side * (17 - i * 2), -6, 138 - i * 6)], (2.0, .6), m['Gold'])
        tendril('Armor / chest filigree', [(side * 2, -15, 142), (side * 16, -17, 152),
                (side * 18, -15, 140), (side * 10, -15, 138)], .48, m['Gold'])
    diamond = [(0, 158), (5.5, 147), (0, 136), (-5.5, 147)]
    rim('Armor / heart setting', diamond, -15.5, m['Gold'], .7)
    gem('Armor / heart sapphire', (0, -15.8, 147), (4.3, 2, 9), m['Sapphire'])
    feather('Armor / abdominal spear', [(0, -10, 132), (0, -13, 121),
            (0, -11, 113), (0, -10, 105)], (3.3, 1.1), m['Gold'])


def shoulder(m, side):
    shell = [(side * 16, 155), (side * 22, 168), (side * 35, 164),
             (side * 41, 147), (side * 26, 147)]
    plate('Armor / domed pauldron', shell, (-3, 7), m['Ivory'])
    rim('Armor / pauldron border', shell, -3.5, m['Gold'], .8)
    segment_shell('Armor / upper-arm sleeve', ((side * 20, 0, 154), (side * 27, 0, 119)),
                  [(0, 8, 8), (.35, 9, 8), (.7, 6, 6), (1, 5.5, 5.5)], m['Ivory'])
    for i in range(4):
        feather('Armor / radiant shoulder spire', [(side * (22 + i * 2), 3 - i, 156 - i * 2),
                (side * (26 + i * 4), 3 - i, 166), (side * (26 + i * 6), 2 - i, 180 - i * 3),
                (side * (25 + i * 7), 2 - i, 190 - i * 8)], (2.8 - i * .3, .8), m['Gold'])
        feather('Armor / layered shoulder feathers', [(side * 23, -5, 157 - i * 4),
                (side * 31, -8, 154 - i * 4), (side * 42, -4, 159 - i * 5),
                (side * (43 - i), 0, 170 - i * 7)], (2.5, .8), m['Ivory'])
    gem('Armor / shoulder sapphire', (side * 29, -9, 160), (2.5, 1.2, 5.5), m['Sapphire'])
    for i in range(3):
        feather('Armor / upper-arm gold inset', [(side * 24, -7, 148 - i * 7),
                (side * 32, -6, 145 - i * 6), (side * 30, -4, 137 - i * 6),
                (side * 26, -3, 133 - i * 6)], (1.3, .45), m['Gold'])


def rear_crest(m):
    for side in (-1, 1):
        tendril('Armor / rear spine goldwork', [(0, 10, 113), (side * 15, 15, 128),
                (side * 22, 13, 144), (side * 8, 8, 153)], .7, m['Gold'])
    feather('Armor / back crest', [(0, 9, 118), (0, 16, 132), (0, 13, 143),
            (0, 8, 158)], (4, 1), m['Gold'])
