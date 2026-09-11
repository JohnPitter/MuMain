"""Original Zeus Celestial Wings: faceted lightning-blade fans on the Wing44 arm chains.

Geometry is authored in the Wing44 frame-0 bind space measured from the frozen
base (arm chains 1-5 and 24-28 rising from the harness root). Blades are sharp
celeste plates with platina spines and emissive zigzag channels - the same
bolt-facet language as the Zeus armor - closed by an eight-point star crest on
the static root bone. No Wing44 surface is copied; Wing44 provides skeleton
and flap only. Every primitive is a closed manifold shell.
"""
import math

import bpy
from mathutils import Vector

from armor_surfaces import bind_objects, section
from celestial_geometry import bezier, feather, gem, plate, tendril, tube
from zeus_wings_rig import FLAP_CHAINS

# Rows: (count, length scale, depth offset, feather samples, width, rachis,
#        zigzag channels, platina tip, deck). The 'up' decks form the raised
#        fan (front to back, then covert cover at the blade bases); the 'down'
#        deck sweeps the pointed lower wing edge below the arm.
ROWS = (
    (12, 1.00, -2.0, 9, 7.6, True, 'all', True, 'up'),
    (10, 0.78, 5.0, 8, 6.1, True, 'all', True, 'up'),
    (8, 0.58, 12.0, 7, 4.8, True, None, False, 'up'),
    (12, 0.32, 2.0, 6, 4.2, False, None, False, 'up'),
    (9, 0.66, 9.0, 7, 5.4, True, None, False, 'down'),
)
ARM_POINTS = 13
STAR_OUTER, STAR_INNER, STAR_POINTS = 19.0, 8.0, 8
CREST_CENTER = Vector((0, 12, 40))


def nearest_bone(point, bind, indices):
    point = Vector(point)
    return min(indices, key=lambda i: (bind[i].translation - point).length_squared)


def arm_curve(side, count=ARM_POINTS):
    return bezier([(side * 10, 4, 0), (side * 40, -5, 22),
                   (side * 66, -12, 60), (side * 124, 4, 124)], count)


def blade_layout(side, s, row):
    count, scale, depth = ROWS[row][0], ROWS[row][1], ROWS[row][2]
    deck = ROWS[row][8]
    if deck == 'down':
        spread = 0.3 + 0.7 * s
        elevation = math.radians(14 - 42 * s)
    else:
        spread = s
        elevation = math.radians(88 - 38 * s + 4 * math.sin(math.pi * s))
    length = scale * (52 + 66 * math.sin(math.pi * s) ** 0.8)
    direction = Vector((side * math.cos(elevation), 0.24,
                        math.sin(elevation))).normalized()
    root = arm_curve(side)[max(2, min(ARM_POINTS - 1, 2 + round(10 * spread)))]
    root = root + Vector((0, depth, 0))
    width = ROWS[row][4] * (0.62 + 0.38 * math.sin(math.pi * s) ** 0.5)
    return root, direction, length, width, elevation


def blade(palette, bind, side, row, index):
    _, _, _, samples, _, has_rachis, zigzag, has_tip, _ = ROWS[row]
    count = ROWS[row][0]
    s = index / (count - 1)
    root, direction, length, width, elevation = blade_layout(side, s, row)
    tip = root + direction * length
    mid1 = root.lerp(tip, 0.34) + Vector((0, 1.4, length * 0.035))
    mid2 = root.lerp(tip, 0.72) + Vector((0, 0.7, length * 0.018))
    bone = nearest_bone(root, bind, FLAP_CHAINS[side])

    def build():
        feather('Wings / celeste bolt blade', [root, mid1, mid2, tip],
                (width, 0.85, samples), palette['Blue'])
        path = bezier([root, mid1, mid2, tip], 8)
        if has_rachis:
            quill = path[:7]
            tube('Wings / platina blade spine', quill,
                 [0.5 * (1 - i / len(quill)) + 0.1 for i in range(len(quill))],
                 palette['Platina'], 5)
        if has_tip:
            feather('Wings / platina blade point',
                    [path[5], path[6], path[7], path[7] + direction * 5],
                    (width * 0.34, 0.4, 5), palette['Platina'])
        wanted = True if zigzag == 'all' else (index % 2 == 0)
        if zigzag and wanted:
            lateral = Vector((side * math.sin(elevation), 0, -math.cos(elevation)))
            channel = [root.lerp(tip, 0.16 + 0.7 * step / 6) +
                       lateral * (width * 0.26 if step % 2 else -width * 0.26)
                       for step in range(7)]
            tube('Wings / storm channel', channel, [0.5] * 7,
                 palette['Emissive'], 4)

    section(bone, build)


def fan(palette, bind, side):
    for row in range(len(ROWS)):
        for index in range(ROWS[row][0]):
            blade(palette, bind, side, row, index)


def arm_vanes(palette, bind, side):
    """Angular platina vanes strapped over the arm chain bones."""
    for bone, scale in ((2, 0.7), (3, 0.58), (4, 0.48)):
        center = bind[bone].translation

        def build(center=center, scale=scale):
            x, y, z = center.x, center.y, center.z
            outline = [(x - 8 * scale, z - 3), (x - 2 * scale, z + 9 * scale),
                       (x + 3 * scale, z + 11 * scale), (x + 8 * scale, z + 2 * scale),
                       (x + 3 * scale, z - 8 * scale), (x - 3 * scale, z - 6 * scale)]
            plate('Wings / platina arm vane', outline, (y + 6.0, 1.6),
                  palette['Platina'])
            gem('Wings / arm vane gem', (x, y + 4.4, z + 2 * scale),
                (1.1 * scale, 0.9, 2.6 * scale), palette['Blue'])

        section(bone, build)


def arm_runner(palette, bind, side):
    """Platina runner along the arm base, bound per vertex to the nearest bone."""
    before = set(bpy.data.objects)
    curve = bezier([(side * 10, 2, 6), (side * 36, -6, 26),
                    (side * 60, -13, 60), (side * 104, -6, 96)], 9)
    tube('Wings / platina arm runner', [tuple(point) for point in curve], 1.1,
         palette['Platina'], 6)
    created = set(bpy.data.objects) - before
    bpy.context.view_layer.update()
    bind_objects(created, lambda obj, co: nearest_bone(co, bind, FLAP_CHAINS[side]))


def star_outline(center, outer, inner, points):
    x0, z0 = center
    outline = []
    for i in range(points * 2):
        radius = outer if i % 2 == 0 else inner
        angle = math.pi * i / points
        outline.append((x0 + radius * math.sin(angle), z0 + radius * math.cos(angle)))
    return outline


def crest(palette):
    """Eight-point star crest with the heart gem, riding the static root bone."""
    x0, y0, z0 = CREST_CENTER

    def build():
        plate('Wings / star crest frame',
              star_outline((x0, z0), STAR_OUTER, STAR_INNER, STAR_POINTS),
              (y0, 2.4), palette['Platina'])
        plate('Wings / star crest blade',
              star_outline((x0, z0), STAR_OUTER * 0.62, STAR_INNER * 0.62, STAR_POINTS),
              (y0 - 2.6, 1.9), palette['Blue'])
        gem('Wings / crest heart gem', (x0, y0 - 4.6, z0), (3.2, 2.6, 7.2),
            palette['Blue'])
        gem('Wings / crest core spark', (x0, y0 - 6.2, z0), (1.1, 1.0, 2.6),
            palette['Emissive'])
        for offset in ((0, STAR_OUTER - 3.5), (0, -(STAR_OUTER - 3.5)),
                       (STAR_OUTER - 3.5, 0), (-(STAR_OUTER - 3.5), 0)):
            gem('Wings / crest point gem',
                (x0 + offset[0], y0 - 1.2, z0 + offset[1]), (1.2, 1.0, 2.8),
                palette['Blue'])
        for side in (-1, 1):
            clamp = Vector((side * 17, y0 - 6, 10))

            def build_clamp(clamp=clamp):
                x, z = clamp.x, clamp.z
                plate('Wings / root clamp',
                      [(x - 6, z - 7), (x - 2, z + 6), (x + 5, z + 9),
                       (x + 7, z + 1), (x + 3, z - 9)], (clamp.y, 1.5),
                      palette['Platina'])
                gem('Wings / clamp stud', (x, clamp.y - 1.6, z + 1),
                    (0.9, 0.8, 1.9), palette['Blue'])

            build_clamp()
            tendril('Wings / crest lower scroll',
                    [(side * 6, y0 - 6, z0 - 22), (side * 26, y0 - 6, z0 - 26),
                     (side * 22, y0 - 6, z0 - 36), (side * 8, y0 - 6, z0 - 30)],
                    1.0, palette['Platina'])

    section(0, build)


def wings(palette, bind):
    """Build both wings; every created object lands in the caller's collection."""
    for side in (1, -1):
        fan(palette, bind, side)
        arm_vanes(palette, bind, side)
        arm_runner(palette, bind, side)
    crest(palette)
