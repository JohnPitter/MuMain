"""Original Zeus jewels: Pendant "Olho de Zeus", Storm Ring and Wisdom Ring.

Inventory-scale models on the audited Necklace02/Ring02 contracts (one static
root bone each: Tube04 for the pendant, Tube01 for both rings - names,
indices and hierarchy kept exactly, no renumbering). The pendant hangs the
eye of the storm: an celeste eye gem with emissive iris inside an eight-point
platina star frame with side bolts and a fine chain. The rings are two
distinct silhouettes: the Storm Ring crowns an angular bolt with a tall
elongated crystal, the Wisdom Ring a wide circular star-crown halo - both
with celeste gems.
"""
import math

from mathutils import Vector

from celestial_geometry import ellipse, feather, gem, plate, rim, tube

TAU = math.tau
BAND_CENTER = (0, 4, -3)
BAND_RADIUS = 7.5
BAND_STEPS = 26


def outline_circle(center, radius, steps, squash=1.0):
    x0, z0 = center
    return [(x0 + radius * math.cos(TAU * i / steps),
             z0 + radius * math.sin(TAU * i / steps) * squash)
            for i in range(steps)]


def star_outline(center, outer, inner, points):
    x0, z0 = center
    outline = []
    for i in range(points * 2):
        radius = outer if i % 2 == 0 else inner
        angle = math.pi * i / points
        outline.append((x0 + radius * math.sin(angle), z0 + radius * math.cos(angle)))
    return outline


def band(palette, width=1.2, etch_name='Ring / storm etch'):
    """Shared platinated ring band with an engraved upper edge and etch runs."""
    ellipse('Ring / platina band', BAND_CENTER,
            ((BAND_RADIUS, 0, 0), (0, 0, BAND_RADIUS)), palette['Platina'],
            width, BAND_STEPS)
    ellipse('Ring / engraved upper edge', (BAND_CENTER[0], 2.9, BAND_CENTER[2]),
            ((BAND_RADIUS + 0.1, 0, 0), (0, 0, BAND_RADIUS + 0.1)),
            palette['Platina'], 0.4, BAND_STEPS)
    ellipse('Ring / engraved lower edge', (BAND_CENTER[0], 5.1, BAND_CENTER[2]),
            ((BAND_RADIUS + 0.1, 0, 0), (0, 0, BAND_RADIUS + 0.1)),
            palette['Platina'], 0.4, BAND_STEPS)
    for side in (-1, 1):
        etch = []
        for i in range(7):
            angle = side * (0.55 - 1.1 * i / 6)
            radius = BAND_RADIUS - width * 0.7
            etch.append((radius * math.cos(angle), 4 + 0.8 * math.sin(math.pi * i / 6),
                         BAND_CENTER[2] + radius * math.sin(angle)))
        tube(etch_name, etch, [0.34] * 7, palette['Emissive'], 4)


def prong_crown(palette, center, radius, height):
    """Four platina prongs holding a gem head above the band."""
    x0, y0, z0 = center
    rim('Ring / gem bezel', outline_circle((x0, z0 + 1.2), radius, 12),
        y0, palette['Platina'], 0.55)
    for step in range(4):
        angle = TAU * step / 4 + math.pi / 4
        foot = Vector((x0 + radius * math.cos(angle), y0,
                       z0 + 1.2 + radius * math.sin(angle)))
        top = foot + Vector((0, 0, height))
        tube('Ring / crown prong', [tuple(foot), tuple(foot.lerp(top, 0.5)),
             tuple(top)], [0.5, 0.4, 0.3], palette['Platina'], 4)


def pendant(palette):
    """Amulet: celeste eye gem in an eight-point star frame with side bolts."""
    plate('Pendant / star frame', star_outline((0, 0), 10.0, 4.2, 8),
          (0.7, 1.5), palette['Platina'])
    plate('Pendant / star frame blade', star_outline((0, 0), 6.0, 2.5, 8),
          (-0.4, 1.1), palette['Blue'])
    rim('Pendant / eye bezel', outline_circle((0, 0), 5.4, 12, 0.72),
        -1.2, palette['Platina'], 0.5)
    gem('Pendant / storm eye', (0, -2.0, 0), (4.6, 2.0, 3.0), palette['Blue'])
    gem('Pendant / eye iris', (0, -3.1, 0), (1.7, 1.3, 1.9), palette['Emissive'])
    gem('Pendant / eye pupil', (0, -3.9, 0), (0.55, 0.9, 1.7), palette['Platina'])
    for side in (-1, 1):
        for sign in (-1, 1):
            bolt = [(side * 9.6, 0.4, sign * 1.6), (side * 12.2, 0.4, sign * 3.4),
                    (side * 11.6, 0.4, sign * 0.6), (side * 14.6, 0.4, sign * 2.4)]
            tube('Pendant / side bolt', bolt, [0.5, 0.42, 0.34, 0.2],
                 palette['Platina'], 4)
        gem('Pendant / frame point gem', (side * 8.2, -0.6, 0),
            (1.0, 0.9, 1.6), palette['Blue'])
    ellipse('Pendant / bail', (0, 0, 13.0), ((1.8, 0, 0), (0, 0, 3.0)),
            palette['Platina'], 0.45, 20)
    for link in range(5):
        ellipse('Pendant / chain link', (0, 0.3 * (link % 2), 16.4 + link * 2.1),
                ((1.3, 0, 0), (0, 0, 1.9)), palette['Platina'], 0.3, 12)


def ring_storm(palette):
    """Storm Ring: stepped bolt crown with a tall elongated celeste crystal."""
    band(palette, width=1.2, etch_name='Storm Ring / band etch')
    prong_crown(palette, (0, 3.6, BAND_CENTER[2] + BAND_RADIUS - 0.6), 2.6, 2.2)
    bolt = [(2.2, 5.2), (-0.6, 8.0), (1.0, 8.0), (-1.6, 11.2), (0.2, 11.2),
            (-2.4, 14.6), (2.6, 9.6), (0.8, 9.6), (3.0, 6.4)]
    plate('Storm Ring / bolt emblem', [(x, z) for x, z in bolt],
          (3.6, 1.1), palette['Platina'])
    rim('Storm Ring / bolt trim', [(x * 1.15, z * 1.15) for x, z in bolt],
        3.0, palette['Platina'], 0.38)
    gem('Storm Ring / tempest crystal', (0, 2.6, 19.0), (1.9, 1.6, 5.0),
        palette['Blue'])
    gem('Storm Ring / charge spark', (0, 1.5, 25.2), (0.8, 0.7, 1.8),
        palette['Emissive'])


def ring_wisdom(palette):
    """Wisdom Ring: wide circular star-crown halo over a round celeste gem."""
    band(palette, width=1.05, etch_name='Wisdom Ring / band etch')
    head = Vector((0, 3.2, BAND_CENTER[2] + BAND_RADIUS + 1.4))
    prong_crown(palette, (0, 3.2, BAND_CENTER[2] + BAND_RADIUS - 1.4), 2.4, 1.8)
    orbit_center = head + Vector((0, 0, 5.2))
    ellipse('Wisdom Ring / crown orbit', tuple(orbit_center),
            ((7.6, 0, 0), (0, 0, 7.6)), palette['Platina'], 0.5, 22)
    for step in range(8):
        angle = TAU * step / 8
        radial = Vector((math.cos(angle), 0, math.sin(angle)))
        point = orbit_center + radial * 7.6
        feather('Wisdom Ring / crown point',
                [tuple(point - radial * 1.4), tuple(point.lerp(point + radial * 2.6, 0.4)),
                 tuple(point.lerp(point + radial * 2.6, 0.8)),
                 tuple(point + radial * 3.0)],
                (1.05, 0.5, 5), palette['Platina'])
        if step % 2 == 0:
            gem('Wisdom Ring / crown pearl',
                tuple(point + radial * 5.2 + Vector((0, -1.2, 0))),
                (0.85, 0.8, 1.5), palette['Blue'])
    gem('Wisdom Ring / sage gem', (head.x, 1.8, orbit_center.z), (2.6, 2.2, 2.6),
        palette['Blue'])
    gem('Wisdom Ring / inner light', (head.x, 0.9, orbit_center.z), (1.0, 0.9, 1.1),
        palette['Emissive'])


BUILDERS = {'Pendant': pendant, 'Ring_Storm': ring_storm, 'Ring_Wisdom': ring_wisdom}
# Anchor positions (band/gem style) recorded per jewel for fit review; the
# probes sit on the exported surface so verification can measure placement.
BAND_TOP = (BAND_CENTER[0], BAND_CENTER[1], BAND_CENTER[2] + BAND_RADIUS)
ANCHORS = {
    'Pendant': {'ChainTop': (0, 0, 24.8), 'Frame': (0, 0, 0), 'Eye': (0, -2.0, 0)},
    'Ring_Storm': {'Band': BAND_TOP, 'Gem': (0, 2.6, 19.0)},
    'Ring_Wisdom': {'Band': BAND_TOP, 'Gem': (0, 1.8, 11.1)},
}
