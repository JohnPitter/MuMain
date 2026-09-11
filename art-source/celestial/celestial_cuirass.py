"""Crown, layered cuirass and split tabard authored from the supplied concept."""
import math

from armor_surfaces import loft, rear_facing, section, segment_shell
from celestial_geometry import ellipse, feather, gem, mesh, plate, rim
from celestial_gilding import chest_relief, inlaid_leaf, scroll, shoulder_relief
from celestial_rear_armor import backplate, crown_back, shoulder_back
from curved_armor_plate import curved_plate, rounded_outline


def helmet_shell(m):
    levels = [(158, 10, 11), (176, 13, 18), (186, 12, 16), (192, 8, 11), (195, 1, 1)]
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
    shell = mesh('Helm / open-face crown shell', vertices, faces, m['Base'])
    for face in shell.data.polygons:
        face.use_smooth = True


def helmet(m):
    section(20, lambda: helmet_details(m))


def helmet_details(m):
    helmet_shell(m)
    rear_facing(lambda: crown_back(m))
    gem('Helm / forehead diamond', (0, -24, 186), (2.9, 1.7, 6.5), m['Sapphire'])
    rim('Helm / diamond setting', [(0, 195), (4, 186), (0, 176), (-4, 186)], -24, m['Trim'], .6)
    for side in (-1, 1):
        inlaid_leaf('Helm / split brow crest', [(side * 1, -24, 175), (side * 7, -24, 181),
                    (side * 15, -22, 187), (side * 17, -14, 197)], (3.7, .9), m)
        cheek = [(side * 11, 181), (side * 13, 174), (side * 8, 158), (side * 6, 171)]
        plate('Helm / pointed cheek guard', cheek, (-16, 2), m['Base'])
        rim('Helm / cheek edging', cheek, -16.5, m['Trim'], .5)
        for i in range(4):
            controls = [(side * (5 + i * 2), -14 + i * 5, 180),
                        (side * (16 + i * 2), -18 + i * 5, 186),
                        (side * (13 + i * 3), -9 + i * 5, 199),
                        (side * (7 + i * 5), -3 + i * 5, 216 - i * 5)]
            inlaid_leaf('Helm / swept crown lance', controls, (3.5 - i * .35, .8), m)
        for i in range(2):
            scroll('Helm / temple acanthus', [(side * 7, -23, 176 + i * 5),
                   (side * 22, -23, 183 + i * 7), (side * 19, -16, 190 + i * 5),
                   (side * 11, -18, 186 + i * 6)], .6, m['Trim'])
        feather('Helm / temple ivory blade', [(side * 10, -8, 173), (side * 20, -3, 182),
                (side * 19, 1, 188), (side * 17, 2, 196)], (2.1, .7), m['Base'])
    inlaid_leaf('Helm / axial crown', [(0, -20, 188), (0, -22, 197), (0, -8, 210),
                (0, -1, 225)], (3.6, .9), m)


def torso_bone(_obj, position):
    return 18 if position.z >= 133 else 17 if position.z >= 117 else 2


def armor(m):
    section(torso_bone, lambda: torso(m))
    for side, bone in ((-1, 26), (1, 35)):
        section(bone, lambda s=side: shoulder(m, s))
    section(torso_bone, lambda: rear_facing(lambda: backplate(m)))


def torso(m):
    rings = [((0, 0, z), x, y) for z, x, y in
             ((112, 13, 9), (122, 12, 8.5), (135, 20, 12), (147, 22, 12), (155, 16, 8.5))]
    loft('Armor / anatomical cuirass', rings, m['Base'])
    ellipse('Armor / standing collar', (0, 0, 157), ((10, 0, 0), (0, 8, 0)), m['Trim'], .9, 32)
    for side in (-1, 1):
        chest = rounded_outline([(side * 2, 151), (side * 16, 158), (side * 22, 148),
                                 (side * 16, 135), (side * 2, 132)])
        curved_plate('Armor / sculpted pectoral', chest, (-11, 3.5), m['Base'])
        rim('Armor / pectoral border', chest, -12, m['Trim'], .65)
        for i in range(3):
            feather('Armor / rib lamella', [(side * 2, -11, 124 - i * 5),
                    (side * 9, -13, 128 - i * 4), (side * 17, -8, 129 - i * 3),
                    (side * (17 - i * 2), -6, 138 - i * 6)], (2.0, .6), m['Trim'])
        chest_relief(m, side)
    diamond = [(0, 156), (4.3, 147), (0, 138), (-4.3, 147)]
    rim('Armor / heart setting', diamond, -15.5, m['Trim'], .7)
    gem('Armor / heart sapphire', (0, -17, 147), (3.3, 2, 7), m['Sapphire'])
    feather('Armor / abdominal spear', [(0, -10, 132), (0, -13, 121),
            (0, -11, 113), (0, -10, 105)], (3.3, 1.1), m['Trim'])


def shoulder(m, side):
    shell = rounded_outline([(side * 16, 155), (side * 22, 168), (side * 35, 164),
                             (side * 41, 147), (side * 26, 147)])
    curved_plate('Armor / domed pauldron', shell, (-3, 7), m['Base'])
    rim('Armor / pauldron border', shell, -3.5, m['Trim'], .8)
    segment_shell('Armor / upper-arm sleeve', ((side * 20, 0, 154), (side * 27, 0, 119)),
                  [(0, 8, 8), (.35, 9, 8), (.7, 6, 6), (1, 5.5, 5.5)], m['Base'])
    for i in range(4):
        feather('Armor / radiant shoulder spire', [(side * (22 + i * 2), 3 - i, 156 - i * 2),
                (side * (26 + i * 4), 3 - i, 166), (side * (26 + i * 6), 2 - i, 180 - i * 3),
                (side * (25 + i * 7), 2 - i, 190 - i * 8)], (2.8 - i * .3, .8), m['Trim'])
        feather('Armor / layered shoulder feathers', [(side * 23, -5, 157 - i * 4),
                (side * 31, -8, 154 - i * 4), (side * 42, -4, 159 - i * 5),
                (side * (43 - i), 0, 170 - i * 7)], (2.5, .8), m['Base'])
    gem('Armor / shoulder sapphire', (side * 29, -9, 160), (2.5, 1.2, 5.5), m['Sapphire'])
    shoulder_relief(m, side)
    rear_facing(lambda: shoulder_back(m, side))
    for i in range(3):
        feather('Armor / upper-arm gold inset', [(side * 24, -7, 148 - i * 7),
                (side * 32, -6, 145 - i * 6), (side * 30, -4, 137 - i * 6),
                (side * 26, -3, 133 - i * 6)], (1.3, .45), m['Trim'])
