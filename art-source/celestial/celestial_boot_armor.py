"""Anatomical greaves and articulated sabatons, bound to native calf/foot joints."""
import math

import bpy
from mathutils import Matrix

from armor_surfaces import rear_facing, section, segment_shell
from celestial_geometry import gem, mesh, plate, rim, tendril, tube
from celestial_gilding import edged_plate, inlaid_leaf, shin_relief

LEG_CENTER = 10.3
FOOT_PROFILE = (
    (8, 4.3, 2.5, 10.5), (6.5, 5.3, 2.3, 13),
    (2, 6, 2.3, 15.5), (-5, 6.5, 2.5, 14),
    (-13, 5.3, 3, 10.5), (-20, 3.2, 3.8, 7.2),
    (-25, .45, 4.5, 5.5),
)


def boots(palette):
    for side, calf, foot in ((-1, 11, 12), (1, 4, 5)):
        section(calf, lambda s=side: greave(palette, s))
        section(foot, lambda s=side: sabaton(palette, s))


def greave(palette, side):
    center = side * LEG_CENTER
    segment_shell('Boots / tapered greave', ((center, 0, 13), (center, -4.9, 63)),
                  [(0, 5.5, 6), (.25, 7, 7), (.65, 9, 9), (1, 9, 9)], palette['Ivory'])
    outline = [(center, 75), (side * 21, 59), (side * 16, 30),
               (side * 13, 15), (side * 7, 15), (side * 3, 57)]
    plate('Boots / pointed shin plate', outline, (-10, 3), palette['Ivory'])
    rim('Boots / shin gold frame', outline, -10.5, palette['Gold'], .7)
    gem('Boots / shin blue jewel', (center, -13.2, 54), (2.6, 1, 6), palette['Sapphire'])
    for direction in (-1, 1):
        tendril('Boots / flowing gold inlay', [(center, -12, 21),
                (center + direction * 6, -13, 38), (center + direction * 7, -12, 56),
                (center, -11, 70)], .55, palette['Gold'])
    shin_relief(palette, side)
    rear_facing(lambda: posterior_greave(palette, side))
    lateral_greave(palette, side)
    ankle_collar(palette, side)


def posterior_greave(palette, side):
    center = side * LEG_CENTER
    for direction in (-1, 1):
        for index in range(3):
            z = 19 + index * 13
            controls = [(center, -7.1, z), (center + direction * 4.4, -8, z + 5),
                        (center + direction * 6.1, -6, z + 10),
                        (center + direction * 5.1, -4.8, z + 17)]
            inlaid_leaf('Boots / posterior calf lamella', controls, (1.65, .45), palette)
        tendril('Boots / posterior calf filigree', [(center + direction * 3, -7, 18),
                (center + direction * 8, -7, 31), (center + direction * 8, -4, 48),
                (center + direction * 6, -3.3, 61)], .42, palette['Gold'])
    inlaid_leaf('Boots / achilles spine', [(center, -7.3, 14), (center, -8, 30),
                (center, -7.1, 45), (center, -5, 61)], (2, .55), palette)
    diamond = [(center, 57), (center + 2.2, 51), (center, 45), (center - 2.2, 51)]
    rim('Boots / rear sapphire bezel', diamond, -6.8, palette['Gold'], .35)
    gem('Boots / rear sapphire', (center, -7.3, 51), (1.5, .7, 4), palette['Sapphire'])


def ankle_collar(palette, side):
    center = side * LEG_CENTER
    for direction in (-1, 1):
        path = [(center + direction * 5.1, -7, 17),
                (center + direction * 6, -1, 14),
                (center + direction * 4.8, 5, 15), (center, 6.7, 20)]
        tube('Boots / articulated ankle edging', path, .55, palette['Gold'], 6)
    rear_facing(lambda: edged_plate('Boots / achilles lower shield',
                [(center, 25), (center + 3.4, 20), (center + 2.8, 12),
                 (center, 10.7), (center - 2.8, 12), (center - 3.4, 20)],
                (-6.4, .9), palette))


def lateral_greave(palette, side):
    previous = set(bpy.data.objects)
    for direction in (-1, 1):
        controls = [(direction * 1.8, -6.1, 17), (direction * 4.5, -8.8, 33),
                    (direction * 3.3, -9.1, 48), (direction * .7, -9, 61)]
        inlaid_leaf('Boots / outer calf feather', controls, (1.65, .45), palette)
    transform = Matrix.Translation((side * LEG_CENTER, 0, 0)) @ Matrix.Rotation(side * math.pi / 2, 4, 'Z')
    for obj in set(bpy.data.objects) - previous:
        obj.data.transform(transform)
        for vertex in obj.data.vertices:
            vertex.co.y -= (vertex.co.z - 13) / 50 * 4.9
        obj.data.update()
        obj['celestial_surface'] = 'lateral_calf_relief'


def foot_cross_section(center, profile):
    y, width, bottom, top = profile
    shoulder = bottom + (top - bottom) * .64
    return [(center - width * .85, y, bottom), (center + width * .85, y, bottom),
            (center + width, y, bottom + .65), (center + width, y, shoulder),
            (center + width * .58, y, top - .3), (center, y, top),
            (center - width * .58, y, top - .3), (center - width, y, shoulder),
            (center - width, y, bottom + .65)]


def foot_shell(palette, side):
    center = side * LEG_CENTER
    vertices = [point for row in FOOT_PROFILE for point in foot_cross_section(center, row)]
    count = len(foot_cross_section(center, FOOT_PROFILE[0]))
    faces = []
    for row in range(len(FOOT_PROFILE) - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    faces.extend((tuple(reversed(range(count))),
                  tuple((len(FOOT_PROFILE) - 1) * count + index for index in range(count))))
    obj = mesh('Boots / pointed sabaton', vertices, faces, palette['Ivory'])
    obj['celestial_surface'] = 'anatomical_sabaton'


def sole(palette, side):
    center = side * LEG_CENTER
    boundary = [(center - width * .91, y, bottom) for y, width, bottom, _ in FOOT_PROFILE]
    boundary += [(center + width * .91, y, bottom)
                 for y, width, bottom, _ in reversed(FOOT_PROFILE)]
    count = len(boundary)
    vertices = boundary + [(x, y, z - .9) for x, y, z in boundary]
    faces = [(index, (index + 1) % count, count + (index + 1) % count, count + index)
             for index in range(count)]
    faces.extend((tuple(range(count)), tuple(reversed(range(count, count * 2)))))
    obj = mesh('Boots / defined gilded sole', vertices, faces, palette['Gold'])
    obj['celestial_surface'] = 'gilded_sole'


def heel_counter(palette, side):
    center = side * LEG_CENTER
    rear_facing(lambda: edged_plate('Boots / angular heel counter',
                [(center - 4.1, 10.7), (center, 13), (center + 4.1, 10.7),
                 (center + 4.1, 4), (center, 3.5), (center - 4.1, 4)],
                (-8.15, .6), palette))
    for direction in (-1, 1):
        tendril('Boots / heel-to-instep scroll', [(center + direction * 4.3, 7.5, 7),
                (center + direction * 6.6, 2, 9), (center + direction * 6.8, -5, 11),
                (center + direction * 4.4, -14, 8)], .42, palette['Gold'])


def dorsal_scales(palette, side):
    center = side * LEG_CENTER
    for index, (y, width, height) in enumerate(((-18, 3.7, 9.5), (-12, 5.4, 12.2),
                                               (-6, 6, 15.2), (0, 5.7, 17))):
        outline = [(center - width, y + 1.5, height - 2.8), (center, y + 2.5, height),
                   (center + width, y + 1.5, height - 2.8),
                   (center + width * .84, y - 2.1, height - 3.1),
                   (center, y - 3.5, height - 1.5),
                   (center - width * .84, y - 2.1, height - 3.1)]
        top = outline + [(x, depth, z - .7) for x, depth, z in outline]
        count = len(outline)
        faces = [(0, 1, 4, 5), (1, 2, 3, 4), (11, 10, 7, 6), (10, 9, 8, 7)]
        faces.extend((n, (n + 1) % count, count + (n + 1) % count, count + n)
                     for n in range(count))
        obj = mesh('Boots / articulated instep scale', top, faces, palette['Ivory'])
        obj['celestial_surface'] = 'instep_lamella'
        edging = [(x, depth, z + .12) for x, depth, z in (outline[5], outline[4], outline[3])]
        tube('Boots / instep gold chevron', edging,
             .35 + index * .015, palette['Gold'], 6)


def sabaton(palette, side):
    foot_shell(palette, side)
    sole(palette, side)
    heel_counter(palette, side)
    dorsal_scales(palette, side)
