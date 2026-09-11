"""Occipital crown plates and dorsal reliefs following the concept's rear view."""
from celestial_geometry import feather, gem, rim
from celestial_gilding import edged_plate, inlaid_leaf, scroll, shoulder_relief


def crown_back(palette):
    for side in (-1, 1):
        for index in range(3):
            controls = [(side * (1 + index * 2), -18 + index * 2, 162 + index * 2),
                        (side * (8 + index * 2), -20 + index * 2, 169 + index * 2),
                        (side * (9 + index * 3), -17 + index * 3, 185 + index * 2),
                        (side * (3 + index * 5), -7 + index * 4, 202 - index * 3)]
            inlaid_leaf('Helm / occipital crown lamella', controls, (2.7, .65), palette)
        scroll('Helm / engraved nape scroll', [(side * 1, -17, 163), (side * 11, -20, 164),
               (side * 14, -19, 174), (side * 7, -20, 177)], .42, palette['Trim'])
        inlaid_leaf('Helm / nape blade', [(side * 10, -11, 178), (side * 15, -10, 170),
                    (side * 13, -10, 158), (side * 9, -9, 154)], (2.1, .5), palette)
    inlaid_leaf('Helm / sagittal crest', [(0, -18, 164), (0, -23, 181),
                (0, -13, 197), (0, 0, 210)], (2.8, .7), palette)
    gem('Helm / rear crown sapphire', (0, -22, 183), (1.8, 1, 3.8), palette['Sapphire'])


def backplate(palette):
    for side in (-1, 1):
        outline = [(side * 2, 151), (side * 10, 156), (side * 18, 151),
                   (side * 19, 144), (side * 13, 132), (side * 4, 126)]
        edged_plate('Armor / scapular shield', outline, (-12, 3), palette)
        dorsal_leaves(palette, side)
        scroll('Armor / dorsal acanthus', [(side * 3, -16, 132), (side * 17, -17, 134),
               (side * 20, -15, 149), (side * 11, -16, 150)], .5, palette['Trim'])
        scroll('Armor / lumbar scroll', [(side * 1, -10, 111), (side * 10, -7.5, 116),
               (side * 11, -8, 126), (side * 7, -10.5, 127)], .45, palette['Trim'])
    for index in range(4):
        z = 151 - index * 10
        depth = -17 + index * 1.4
        outline = [(0, z + 3), (3.7 - index * .4, z - 2),
                   (0, z - 12), (-3.7 + index * .4, z - 2)]
        edged_plate('Armor / articulated dorsal keel', outline, (depth, 1.1), palette)
    diamond = [(0, 156), (2.5, 151), (0, 146), (-2.5, 151)]
    rim('Armor / dorsal jewel setting', diamond, -18.2, palette['Trim'], .4)
    gem('Armor / dorsal jewel', (0, -19, 151), (1.9, 1.0, 4), palette['Sapphire'])


def dorsal_leaves(palette, side):
    for index in range(3):
        controls = [(side * 2, -16 + index, 139 - index * 9),
                    (side * (11 - index), -19 + index, 142 - index * 8),
                    (side * (21 - index * 3), -14 + index, 149 - index * 9),
                    (side * (16 - index * 2), -10, 159 - index * 10)]
        inlaid_leaf('Armor / scapular feather overlay', controls, (2.7 - index * .35, .55), palette)


def shoulder_back(palette, side):
    shoulder_relief(palette, side)
    for index in range(3):
        controls = [(side * 24, -8, 146 - index * 7), (side * 32, -10, 145 - index * 7),
                    (side * 33, -7, 136 - index * 7), (side * 27, -6, 129 - index * 7)]
        feather('Armor / rear arm lamella', controls, (2.2, .5, 8), palette['Trim'])
