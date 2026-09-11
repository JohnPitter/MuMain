"""Original Poseidon jewelry silhouettes: the Royal Trident pendant and two rings.

Continues the Poseidon visual language from poseidon_shapes.py and
poseidon_armor_shapes.py: abyssal-black mass, antique-gold structure and
fillets, ocean-blue restricted to the central jewels. The concept panel
(ITENS ADICIONAIS) defines three distinct pieces: the pendant carries a
golden trident in relief over an ocean lozenge on a black pedestal, the
left ring (Tides) is rounded with wave curls around a drop gem, and the
right ring (Emperor) is angular with a crown of prongs over a faceted
lozenge - two authored models, never one ring equipped twice.

Pearl-white accents of the concept are expressed as bright gold studs here;
design-spec.md leaves true pearl white to the future atlases, so no fourth
texture is declared.

Items are authored rigid (audited native item contract, cf.
native_scepter_orientation_only in native-reference-audit.json): the pendant
hangs along Z with its face toward -Y; both rings stand with the band circle
in the XZ plane, finger axis along Y and the crest rising along +Z, so the
rings export with an identity transform like the Celestial ring prototype.
Scale follows the Celestial authored props (ring band radius ~7, pendant
crest ~26 tall) without reusing any of their meshes.
"""
from math import cos, sin
from math import tau as TAU

from celestial_geometry import ellipse, gem, mesh, plate, rim, tube
from poseidon_armor_shapes import rear_facing, wave

FRAME_OUTLINE = [(0, 13.5), (6.2, 8.6), (8.4, 1.5), (5.2, -6.8),
                 (0, -12.5), (-5.2, -6.8), (-8.4, 1.5), (-6.2, 8.6)]
TIDE_SEAT = [(0, 13.8), (3.9, 12.3), (5.6, 9.3), (4.5, 6.3),
             (0, 5.5), (-4.5, 6.3), (-5.6, 9.3), (-3.9, 12.3)]
CROWN_SEAT = [(0, 13.2), (4.3, 11.8), (5.7, 8.6), (3.6, 5.8),
              (0, 5.2), (-3.6, 5.8), (-5.7, 8.6), (-4.3, 11.8)]
BAND_RADIUS, BAND_WIDTH = 7.0, 1.2


def torus(name, radius, width, material, segments=24, sides=10, center=(0, 0, 0)):
    """Watertight band ring in the XZ plane (finger axis along Y); no seam caps."""
    vertices, faces = [], []
    for i in range(segments):
        angle = TAU * i / segments
        ring_center = (center[0] + radius * cos(angle), center[1],
                       center[2] + radius * sin(angle))
        for n in range(sides):
            roll = TAU * n / sides
            vertices.append((ring_center[0] + width * cos(angle) * cos(roll),
                             ring_center[1] + width * sin(roll),
                             ring_center[2] + width * sin(angle) * cos(roll)))
    for row in range(segments):
        following = (row + 1) % segments
        for n in range(sides):
            a, b = row * sides + n, row * sides + (n + 1) % sides
            faces.append((a, b, following * sides + (n + 1) % sides,
                          following * sides + n))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons:
        face.use_smooth = True
    return result


def inset(outline, factor):
    cx = sum(x for x, _ in outline) / len(outline)
    cz = sum(z for _, z in outline) / len(outline)
    return [(cx + (x - cx) * factor, cz + (z - cz) * factor) for x, z in outline]


def studs(name, material, points, size=(0.75, 0.5, 1.5)):
    """Bright gold studs standing in for the pearl dots of the concept."""
    for index, (x, y, z) in enumerate(points):
        gem(f'{name}_{index}', (x, y, z), size, material)


def band_circle(z, radius=BAND_RADIUS + .45):
    """Point on the band surface envelope so relief hugs the ring, not the hole."""
    return (radius * radius - z * z) ** .5


def chain_links(m, count, start, reach):
    """Fine gold chain stub rising from the bail; alternates link orientation."""
    for index in range(count):
        t = index / max(count - 1, 1)
        z = start + t * reach
        sway = .45 * sin(t * 2.2)
        flat = index % 2 == 0
        axes = ((1.15 if flat else 0.5, 0, 0), (0, 0.5 if flat else 0.4, 1.0))
        ellipse(f'Pendant_chain_link_{index}', (sway, -0.9 + 0.25 * (index % 2), z),
                axes, m['Gold'], .2, 6)


# --------------------------------------------------------------- pendant ----
def pendant(m):
    frame(m)
    trident_relief(m)
    rear_facing(lambda: rear_detail(m))
    ellipse('Pendant_bail_gold', (0, -0.9, 16.6), ((2.0, 0, 0), (0, 0, 1.4)),
            m['Gold'], .45, 10)
    chain_links(m, 8, 18.6, 6.6)


def frame(m):
    plate('Pendant_pedestal_black', FRAME_OUTLINE, (-1.5, 1.6), m['Black'])
    rim('Pendant_frame_gold_bezel', FRAME_OUTLINE, -2.0, m['Gold'], .62)
    rim('Pendant_frame_inner_gold_line', inset(FRAME_OUTLINE, .82), -.85,
        m['Gold'], .3)
    gem('Pendant_ocean_lozenge', (0, -2.6, -.6), (2.5, 1.2, 5.4), m['Blue'])
    rim('Pendant_ocean_bezel_gold', [(0, 4.6), (2.7, -.6), (0, -5.8), (-2.7, -.6)],
        -3.2, m['Gold'], .45)
    studs('Pendant_frame_pearl_stud_gold', m['Gold'],
          ((-6.9, -2.1, 5.1), (6.9, -2.1, 5.1), (0, -2.1, -10.9)))
    wave('Pendant_frame_wave_left_gold',
         [(-6.1, -2.2, 8.4), (-7.6, -2.4, 4.4), (-6.4, -2.3, .8),
          (-4.4, -2.2, -1.8)], .42, m['Gold'])
    wave('Pendant_frame_wave_right_gold',
         [(6.1, -2.2, 8.4), (7.6, -2.4, 4.4), (6.4, -2.3, .8),
          (4.4, -2.2, -1.8)], .42, m['Gold'])
    tube('Pendant_top_spire_gold', [(0, -1.1, 13.4), (0, -1.1, 15.1)],
         [.55, .1], m['Gold'], sides=6)
    tube('Pendant_drop_link_gold', [(0, -1.4, -12.4), (0, -1.5, -13.2)],
         [.32, .26], m['Gold'], sides=5)
    gem('Pendant_drop_ocean_gem', (0, -1.7, -14.2), (1.0, .65, 2.1), m['Blue'])


def trident_relief(m):
    """Golden trident raised in front of the lozenge, the pendant's namesake."""
    y = -5.0
    tube('Pendant_trident_shaft_gold',
         [(0, y, -8.6), (0, y + .1, -3.4), (0, y + .1, 2), (0, y, 7.2),
          (0, y - .1, 10.4)], [.52, .48, .44, .4, .36], m['Gold'], sides=6)
    plate('Pendant_trident_foot_lozenge_black',
          [(0, -10.6), (1.5, -8.9), (0, -7.2), (-1.5, -8.9)], (y - .2, .7),
          m['Black'])
    tube('Pendant_trident_crossbar_gold',
         [(-5.4, y + .05, 7.8), (0, y + .25, 8.3), (5.4, y + .05, 7.8)],
         .46, m['Gold'], sides=6)
    tube('Pendant_trident_center_tine_gold',
         [(0, y + .05, 10.2), (0, y, 13.2), (0, y - .1, 15.7)],
         [.4, .29, .05], m['Gold'], sides=5)
    for side in (-1, 1):
        path = [(side * 5.4, y + .05, 7.8), (side * 5.6, y, 10.2),
                (side * 4.0, y - .1, 12.6), (side * 1.9, y - .15, 14.3)]
        tube(f'Pendant_trident_outer_tine_gold_{side}', path,
             [.38, .32, .22, .05], m['Gold'], sides=5)
        gem(f'Pendant_trident_butt_gem_{side}', (side * 4.4, y + .2, 6.1),
            (.7, .5, 1.4), m['Blue'])


def rear_detail(m):
    """Authored facing front; mirrored onto the back of the pedestal."""
    plate('Pendant_rear_seat_black', inset(FRAME_OUTLINE, .68), (1.3, 1.2),
          m['Black'])
    rim('Pendant_rear_gold_hem', inset(FRAME_OUTLINE, .68), 1.8, m['Gold'], .4)
    gem('Pendant_rear_ocean_seal', (0, 2.9, -.6), (1.4, .75, 3.1), m['Blue'])
    wave('Pendant_rear_wave_gold',
         [(-4.4, 1.9, 7.6), (0, 2.3, 9.6), (4.4, 1.9, 7.6), (1.6, 1.7, 5)],
         .4, m['Gold'])


# ----------------------------------------------------------------- rings ----
def ring_band(m, name):
    torus(f'{name}_band_black', BAND_RADIUS, BAND_WIDTH, m['Black'])
    torus(f'{name}_band_upper_edge_gold', BAND_RADIUS + .18, .34, m['Gold'],
          segments=20, sides=6, center=(0, -.5, 0))
    torus(f'{name}_band_lower_edge_gold', BAND_RADIUS + .1, .27, m['Gold'],
          segments=20, sides=6, center=(0, .5, 0))


def ring_tide(m):
    """Left ring: rounded seat, three wave curls and foam around a drop gem."""
    ring_band(m, 'RingTide')
    plate('RingTide_seat_black', TIDE_SEAT, (-1.7, 1.3), m['Black'])
    rim('RingTide_seat_gold_bezel', TIDE_SEAT, -2.2, m['Gold'], .5)
    gem('RingTide_ocean_drop', (0, -3.5, 9.6), (2.4, 1.5, 4.7), m['Blue'])
    rim('RingTide_ocean_bezel_gold', [(0, 13.5), (2.6, 9.6), (0, 5.7), (-2.6, 9.6)],
        -3.7, m['Gold'], .45)
    for index, controls in enumerate((
            [(-6.4, -.5, 5.4), (-5.8, -2.4, 9.2), (-3.1, -3.1, 11.6), (-.4, -2.6, 12.4)],
            [(6.4, -.5, 5.4), (5.8, -2.4, 9.2), (3.1, -3.1, 11.6), (.4, -2.6, 12.4)],
            [(-3.4, -1.2, 6.1), (-1.4, -3.2, 7.2), (1.8, -3.2, 7.4), (3.3, -1.4, 6.3)])):
        wave(f'RingTide_wave_curl_gold_{index}', controls, .45, m['Gold'])
    for side in (-1, 1):
        torus(f'RingTide_foam_bubble_gold_{side}', .42, .15, m['Gold'],
              segments=10, sides=6, center=(side * 3.7, -2.7, 12.6))
    for side in (-1, 1):
        wave(f'RingTide_shoulder_wave_gold_{side}',
             [(side * band_circle(-2.2), -.9, -2.2), (side * band_circle(.8), -.7, .8),
              (side * band_circle(3.4), -.6, 3.4), (side * band_circle(5.4), -.7, 5.4)],
             .4, m['Gold'])
    rear_facing(lambda: rear_crest(m, 'RingTide', TIDE_SEAT, drop=True))


def ring_emperor(m):
    """Right ring: angular seat under a five-prong crown over a faceted gem."""
    ring_band(m, 'RingEmperor')
    plate('RingEmperor_seat_black', CROWN_SEAT, (-1.7, 1.5), m['Black'])
    rim('RingEmperor_seat_gold_bezel', CROWN_SEAT, -2.2, m['Gold'], .5)
    gem('RingEmperor_ocean_lozenge', (0, -3.6, 9.2), (2.2, 1.5, 4.2), m['Blue'])
    rim('RingEmperor_ocean_bezel_gold', [(0, 12.7), (2.4, 9.2), (0, 5.7), (-2.4, 9.2)],
        -3.7, m['Gold'], .45)
    for index, (x, base, top, wing) in enumerate(((0, 11.2, 16.4, 1.4),
                                                  (3.1, 10.5, 14.9, 1.0),
                                                  (5.4, 9.3, 13.1, .8))):
        for side in ((1,) if x == 0 else (1, -1)):
            label = f'{index}_{side}' if x else str(index)
            plate(f'RingEmperor_crown_prong_gold_{label}',
                  [(side * x - wing, base), (side * x, top),
                   (side * x + wing, base)], (-2.1, .8), m['Gold'])
    studs('RingEmperor_pearl_stud_gold', m['Gold'],
          ((-5.3, -2, 10.4), (5.3, -2, 10.4), (0, -2.4, 6.8)))
    for side in (-1, 1):
        tube(f'RingEmperor_shoulder_chevron_gold_{side}',
             [(side * band_circle(-2.6) - side * .3, -.8, -2.6),
              (side * band_circle(1.2), -.8, 1.2),
              (side * band_circle(4.6) - side * .3, -.9, 4.6)],
             .42, m['Gold'], sides=5)
    rear_facing(lambda: rear_crest(m, 'RingEmperor', CROWN_SEAT, drop=False))


def rear_crest(m, name, outline, drop):
    """Authored facing front; mirrored onto the back of the crest seat."""
    plate(f'{name}_rear_seat_black', inset(outline, .74), (1.5, 1.1), m['Black'])
    rim(f'{name}_rear_gold_hem', inset(outline, .74), 2.0, m['Gold'], .38)
    gem(f'{name}_rear_ocean_seal', (0, 2.9, 9.4 if drop else 9), (1.3, .7, 2.8),
        m['Blue'])
    wave(f'{name}_rear_wave_gold',
         [(-3.8, 2, 12.2), (0, 2.4, 10.6), (3.8, 2, 12.2), (1.4, 2, 13.6)],
         .38, m['Gold'])


BUILDERS = {'Pendant': pendant, 'RingTide': ring_tide, 'RingEmperor': ring_emperor}
