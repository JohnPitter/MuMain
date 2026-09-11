"""Layered golden flight feathers with contrasting pale trim and a sun-wheel halo."""
import math

from mathutils import Vector

from armor_surfaces import section
from celestial_geometry import bezier, ellipse, feather, gem, tendril, tube
from celestial_gilding import inlaid_leaf
from wing_animation import HALO_CENTER

CHAINS = {1: (1, 2, 3, 4, 5), -1: (24, 25, 26, 27, 28)}
FEATHERS_PER_ROW = (22, 19, 16)


def nearest_bone(point, bind, indices):
    return min(indices, key=lambda i: (bind[i].translation - Vector(point)).length_squared)


def flight_path(side, index, row):
    t = index / (FEATHERS_PER_ROW[row] - 1)
    root = Vector((side * (22 + 62 * t), -3 - row * 4, 8 + 90 * t))
    stagger = 1 + .035 * math.sin(index * 2.3 + row * 1.1)
    tip = Vector((side * (66 + 65 * math.sin(math.pi * t) + 32 * t) * stagger,
                  20 + 17 * math.sin(math.pi * t), -73 + 264 * t))
    tip = root.lerp(tip, 1 - row * .22)
    tip.y -= row * 5
    bend = Vector((side * 12, -14, 13))
    return [root, root.lerp(tip, .36) + bend, root.lerp(tip, .78) + bend * .35, tip]


def flight_feather(palette, controls, row):
    width = (6.0, 5.1, 4.2)[row]
    vane = feather('Wings / layered ivory flight feather', controls, (width, .65, 8), palette['Base'])
    for face in vane.data.polygons:
        face.use_smooth = True
    path = bezier(controls, 9)
    quill = [p + Vector((0, -.85, 0)) for p in path]
    tube('Wings / gilded feather rachis', quill,
         [.22 * (1 - i / len(quill)) + .03 for i in range(len(quill))], palette['Trim'], 5)
    tip = path[-1]
    cap = [path[-3], path[-3].lerp(tip, .35), path[-2], tip]
    cap = [p + Vector((0, -.9, 0)) for p in cap]
    feather('Wings / gilded feather tip', cap, (width * .32, .18, 4), palette['Trim'])


def fan(palette, bind, side):
    indices = CHAINS[side]
    for row, count in enumerate(FEATHERS_PER_ROW):
        for index in range(count):
            controls = flight_path(side, index, row)
            bone = nearest_bone(controls[0], bind, indices)
            section(bone, lambda p=controls, r=row: flight_feather(palette, p, r))
    crest(palette, bind, side)


def crest(palette, bind, side):
    for row in range(2):
        for index in range(12):
            t = index / 11
            root = Vector((side * (19 + 65 * t + row * 7), -18 - row * 3, 8 + 100 * t))
            tip = root + Vector((side * (17 - 7 * t + row * 5), -3, 30 + 40 * t - row * 12))
            path = [root, root.lerp(tip, .3) + Vector((side * 9, -4, 0)),
                    root.lerp(tip, .7), tip]
            bone = nearest_bone(root, bind, CHAINS[side])
            section(bone, lambda p=path, w=5.3 - t * 1.6:
                    inlaid_leaf('Wings / overlapping gold covert', p, (w, .95), palette))
    root = (side * 33, -23, 35)
    section(nearest_bone(root, bind, CHAINS[side]), lambda:
            gem('Wings / shoulder crystal', root, (3, 1.5, 8), palette['Sapphire']))


def halo(palette):
    center = Vector(HALO_CENTER)
    for radius, width in ((39, .85), (43, .4)):
        ellipse('Wings / solar halo orbit', center, ((radius, 0, 0), (0, 0, radius)),
                palette['Trim'], width, 64)
    for index in range(12):
        angle = math.tau * index / 12
        direction = Vector((math.sin(angle), 0, math.cos(angle)))
        start = center + direction * 35
        length = 21 if index in (0, 6) else 11 if index % 3 == 0 else 6
        end = center + direction * (43 + length)
        path = [start.lerp(end, t) for t in (0, .33, .66, 1)]
        feather('Wings / halo sun ray', path, (1.15, .38, 6), palette['Trim'])
    gem('Wings / halo zenith crystal', (0, center.y - 1, center.z + 58), (1.6, .8, 4), palette['Sapphire'])
    for side in (-1, 1):
        tendril('Wings / lower solar scroll', [(side * 3, 18, center.z - 47), (side * 30, 18, center.z - 35),
                (side * 25, 18, center.z - 16), (side * 8, 18, center.z - 25)], .5, palette['Trim'])


def wings(palette, bind):
    for side in (-1, 1):
        fan(palette, bind, side)
    section(0, lambda: halo(palette))
