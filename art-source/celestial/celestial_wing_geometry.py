"""Original layered ivory flight feathers, gold coverts, and a sun-wheel halo."""
import math

from mathutils import Vector

from armor_surfaces import section
from celestial_geometry import bezier, ellipse, feather, gem, tendril, tube
from wing_animation import HALO_CENTER

CHAINS = {1: (1, 2, 3, 4, 5), -1: (24, 25, 26, 27, 28)}
FEATHERS_PER_ROW = (22, 19, 16)


def nearest_bone(point, bind, indices):
    return min(indices, key=lambda i: (bind[i].translation - Vector(point)).length_squared)


def flight_path(side, index, row):
    t = index / (FEATHERS_PER_ROW[row] - 1)
    root = Vector((side * (22 + 62 * t), -3 - row * 4, 8 + 90 * t))
    tip = Vector((side * (85 + 90 * math.sin(math.pi * t) + 40 * t),
                  16 + 12 * math.sin(math.pi * t), -65 + 230 * t))
    tip = root.lerp(tip, 1 - row * .22)
    tip.y -= row * 5
    bend = Vector((side * 8, -10, 10))
    return [root, root.lerp(tip, .36) + bend, root.lerp(tip, .78) + bend * .35, tip]


def flight_feather(palette, controls, row):
    width = (6.0, 5.1, 4.2)[row]
    feather('Wings / layered ivory flight feather', controls, (width, .75, 8), palette['Ivory'])
    path = bezier(controls, 9)
    quill = [p + Vector((0, -.85, 0)) for p in path]
    tube('Wings / gilded feather rachis', quill,
         [.22 * (1 - i / len(quill)) + .03 for i in range(len(quill))], palette['Gold'], 5)
    tip = path[-1]
    cap = [path[-3], path[-3].lerp(tip, .35), path[-2], tip]
    cap = [p + Vector((0, -.9, 0)) for p in cap]
    feather('Wings / gilded feather tip', cap, (width * .32, .18, 4), palette['Gold'])


def fan(palette, bind, side):
    indices = CHAINS[side]
    for row, count in enumerate(FEATHERS_PER_ROW):
        for index in range(count):
            controls = flight_path(side, index, row)
            bone = nearest_bone(controls[0], bind, indices)
            section(bone, lambda p=controls, r=row: flight_feather(palette, p, r))
    section(lambda _obj, p: nearest_bone(p, bind, indices), lambda: crest(palette, side))


def crest(palette, side):
    controls = [(side * 13, -13, -4), (side * 41, -19, 28),
                (side * 92, -13, 101), (side * 106, -7, 156)]
    tendril('Wings / sculpted gold leading edge', controls, 2.5, palette['Gold'])
    for i in range(10):
        t = i / 9
        root = Vector((side * (21 + 64 * t), -14, 12 + 101 * t))
        tip = root + Vector((side * (23 - 13 * t), -3, 22 + 29 * t))
        path = [root, root.lerp(tip, .3) + Vector((side * 8, -3, 0)),
                root.lerp(tip, .7), tip]
        feather('Wings / radiant gold covert', path, (4 - t, 1.1, 8), palette['Gold'])
        inset = [p + Vector((0, -1.2, 0)) for p in path]
        feather('Wings / ivory covert inlay', inset, (1.25, .25, 8), palette['Ivory'])
    gem('Wings / shoulder crystal', (side * 33, -18, 35), (3, 1.5, 8), palette['Sapphire'])


def halo(palette):
    center = Vector(HALO_CENTER)
    for radius, width in ((39, .85), (43, .4)):
        ellipse('Wings / solar halo orbit', center, ((radius, 0, 0), (0, 0, radius)),
                palette['Gold'], width, 64)
    for index in range(12):
        angle = math.tau * index / 12
        direction = Vector((math.sin(angle), 0, math.cos(angle)))
        start = center + direction * 35
        length = 21 if index in (0, 6) else 11 if index % 3 == 0 else 6
        end = center + direction * (43 + length)
        path = [start.lerp(end, t) for t in (0, .33, .66, 1)]
        feather('Wings / halo sun ray', path, (1.15, .38, 6), palette['Gold'])
    gem('Wings / halo zenith crystal', (0, center.y - 1, center.z + 58), (1.6, .8, 4), palette['Sapphire'])
    for side in (-1, 1):
        tendril('Wings / lower solar scroll', [(side * 3, 18, center.z - 47), (side * 30, 18, center.z - 35),
                (side * 25, 18, center.z - 16), (side * 8, 18, center.z - 25)], .5, palette['Gold'])


def wings(palette, bind):
    for side in (-1, 1):
        fan(palette, bind, side)
    section(0, lambda: halo(palette))
