"""Layered gilded leaves and reliefs shared by the wearable Celestial pieces."""
import math

from mathutils import Vector

from celestial_geometry import bezier, feather, plate, tube


def scroll(name, controls, width, material):
    points = bezier(controls, 12)
    radii = [width * (.18 + .82 * math.sin(math.pi * i / 11) ** .65) for i in range(12)]
    return tube(name, points, radii, material, 6)


def inlaid_leaf(name, controls, dimensions, palette):
    width, thickness = dimensions
    feather(name + ' / gold frame', controls, (width, thickness, 8), palette['Trim'])
    inner = [Vector(point) + Vector((0, -thickness - .1, 0)) for point in controls]
    feather(name + ' / ivory enamel', inner, (width * .43, .18, 8), palette['Base'])


def edged_plate(name, outline, shape, palette):
    depth, rise = shape
    outer = plate(name + ' / gold border', outline, (depth, rise), palette['Trim'])
    center = sum((Vector(point) for point in outline), Vector((0, 0))) / len(outline)
    inner = [center + (Vector(point) - center) * .76 for point in outline]
    plate(name + ' / ivory enamel', inner, (depth - rise * .3 - .25, rise * .76), palette['Base'])
    return outer


def chest_relief(palette, side):
    for index in range(3):
        controls = [(side * (2 + index * .8), -16, 139 - index * 4),
                    (side * (10 + index * 2), -17, 146 - index * 3),
                    (side * (18 + index), -15, 148 - index * 5),
                    (side * (22 - index), -11, 157 - index * 8)]
        inlaid_leaf('Armor / pectoral flame', controls, (2.5 - index * .2, .5), palette)
    for index in range(2):
        z = 130 - index * 10
        controls = [(side * 2, -13, z - 9), (side * 10, -15, z - 5),
                    (side * 14, -13, z + 3), (side * 7, -13, z + 5)]
        scroll('Armor / abdominal acanthus', controls, .65, palette['Trim'])
    inlaid_leaf('Armor / gorget petal', [(side * 2, -12, 153), (side * 7, -12, 157),
                (side * 10, -8, 160), (side * 9, -5, 164)], (2, .5), palette)


def shoulder_relief(palette, side):
    for index in range(3):
        controls = [(side * (23 + index * 3), -11, 150 - index * 3),
                    (side * (24 + index * 4), -13, 159 - index * 2),
                    (side * (33 + index * 4), -10, 165 - index * 4),
                    (side * (36 + index * 5), -6, 174 - index * 5)]
        inlaid_leaf('Armor / overlapping pauldron vane', controls, (3.3, .7), palette)


def shin_relief(palette, side):
    center = side * 10.3
    for direction in (-1, 1):
        for index in range(3):
            z = 27 + index * 12
            controls = [(center, -14, z - 8), (center + direction * 4, -15, z - 2),
                        (center + direction * 6, -13, z + 2), (center + direction * 5, -12, z + 10)]
            inlaid_leaf('Boots / sculpted shin flame', controls, (1.7, .4), palette)
