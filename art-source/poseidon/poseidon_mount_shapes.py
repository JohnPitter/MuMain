"""Original Poseidon black mount silhouette on the audited DarkHorse rig.

Continues the set visual language (design-spec.md "Cavalo negro metálico"):
polished abyssal-black mass, articulated gold bard with pointed scalloped
panels over neck/barrel/flanks, ocean-blue restricted to rune jewels, metal
hooves, a banded armored tail and the rider saddle preserved on the native
saddle band. Shapes are authored in the native bind-pose world space (Z-up,
frame 0 of the frozen DarkHorse.bmd skeleton, ground at z=0, muzzle facing
-Y) with single full influences; binding bones extend the native mesh
distribution only on the animated ear bones 25/26/27 and Toe0 hooves 48/54.

The design-spec horse row defines no mounted lance (the set weapons are the
hand-held Tridente/Cetro), so none is modeled and the rider seat stays clear;
the mount keeps all seven native DarkHorse actions per design-spec.
"""
from math import cos, sin
from math import pi as PI
from math import tau as TAU

from mathutils import Matrix, Vector

from armor_surfaces import section
from celestial_geometry import ellipse, gem, mesh, plate, rim, tube
from poseidon_armor_shapes import curved_shell, mounted_blade, wave

RUMP_BONE, BARREL_BONE, SADDLE_BONE, CHEST_BONE = 17, 18, 19, 20
NECK_BONE, NECK1_BONE, HEAD_BONE = 21, 22, 23
# 25/26/27 are native effect anchors: no native mesh binds to them and their
# clips swing widely, so head-attached details stay on the head bone itself.
CREST_BONE, BASE_CREST_BONE = 28, 42
FRONT_LEG_BONES = {1: (31, 32, 33, 34), -1: (37, 38, 39, 40)}
REAR_LEG_BONES = {1: (44, 45, 46, 47, 48), -1: (50, 51, 52, 53, 54)}
TAIL_BONES = (56, 57, 58)


# ------------------------------------------------------------- primitives ----
def sweep(name, stations, material, segments=20, smooth=True):
    """Closed elliptical tube: stations are (center, u, v) ring definitions,
    each ring = center + u*cos + v*sin."""
    vertices, faces = [], []
    for center, u, v in stations:
        center, u, v = Vector(center), Vector(u), Vector(v)
        for i in range(segments):
            angle = TAU * i / segments
            vertices.append(tuple(center + u * cos(angle) + v * sin(angle)))
    for row in range(len(stations) - 1):
        for i in range(segments):
            a, b = row * segments + i, row * segments + (i + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    faces.append(tuple(range(segments - 1, -1, -1)))
    last = (len(stations) - 1) * segments
    faces.append(tuple(last + i for i in range(segments)))
    result = mesh(name, vertices, faces, material)
    if smooth:
        for face in result.data.polygons:
            face.use_smooth = True
    return result


def arc_sweep(name, centers, radii, u_axis, v_axis, a0, a1, material, steps=8, smooth=True):
    """Open band hugging an axis: each ring is an arc from a0 to a1 radians
    around (u_axis, v_axis) - a bard plate wrapping a curved body part."""
    vertices, faces = [], []
    count = steps + 1
    for center, r in zip(centers, radii):
        center = Vector(center)
        for s in range(count):
            angle = a0 + (a1 - a0) * s / steps
            vertices.append(tuple(center + u_axis * r * cos(angle) + v_axis * r * sin(angle)))
    for row in range(len(centers) - 1):
        for s in range(steps):
            a, b = row * count + s, row * count + s + 1
            faces.append((a, b, b + count, a + count))
    result = mesh(name, vertices, faces, material)
    if smooth:
        for face in result.data.polygons:
            face.use_smooth = True
    return result


def y_ring(y, x=0, z=0, width=0, depth=0):
    """Ring station across the body axis: u along X, v along Z."""
    return (Vector((x, y, z)), Vector((width, 0, 0)), Vector((0, 0, depth)))


def rotate_z(obj, degrees, pivot=(0, 0, 0)):
    """Rigid rotation about Z; rigid transforms keep winding outward."""
    pivot = Vector(pivot)
    obj.data.transform(Matrix.Translation(pivot) @
                       Matrix.Rotation(PI * degrees / 180, 4, 'Z') @
                       Matrix.Translation(-pivot))
    return obj


def blade(name, profile, x, material, thickness=1.2):
    """Vertical crest blade thin in X: profile is (world_y, z) pairs."""
    p = plate(name, profile, (0, thickness), material)
    rotate_z(p, 90)
    if x:
        p.data.transform(Matrix.Translation((x, 0, 0)))
    return p


def flank_panel(name, half, top, bottom, y_center, x_out, side, materials):
    """Scalloped bard panel authored facing -Y, rotated onto a +-X flank.

    The curved shell spans depth -3 with rise 3.4, so after the rotation its
    crown sits 6.4 units from the panel plane; x_out is the crown position."""
    outline = [(-half * .18, bottom), (half * .55, bottom + 5), (half, (top + bottom) / 2),
               (half * .5, top - 4), (-half * .2, top - 8), (-half * .55, (top + bottom) / 2)]
    shell = curved_shell(name, outline, -3, 3.4, materials['Black'])
    edge = rim(name + '_gold_edge', outline, -2.2, materials['Gold'], .55)
    for obj in (shell, edge):
        rotate_z(obj, 90 * side)
        obj.data.transform(Matrix.Translation((side * (x_out - 6.4), y_center, 0)))


def limb_sleeve(m, name, start, end, r0, r1, rings=(0.42,), sides=16):
    """Tapered black limb shell with gold rings at fractional stations."""
    start, end = Vector(start), Vector(end)
    count = 3 if (end - start).length > 44 else 2
    path = [tuple(start + (end - start) * (i / count)) for i in range(count + 1)]
    radii = [r0 + (r1 - r0) * (i / count) for i in range(count + 1)]
    tube(name, path, radii, m['Black'], sides=sides)
    for fraction in rings:
        center = start + (end - start) * fraction
        radius = r0 + (r1 - r0) * fraction + .9
        ellipse(name + f'_gold_ring_{fraction}', tuple(center),
                ((radius, 0, 0), (0, radius, 0)), m['Gold'], .55, 14)


# ------------------------------------------------------------------ horse ----
def mount(m):
    barrel(m)
    flank_bard(m)
    breastplate(m)
    neck_and_crest(m)
    head_bard(m)
    saddle(m)
    armored_tail(m)
    for side in (1, -1):
        front_leg(m, side)
        rear_leg(m, side)


def barrel(m):
    """Black barrel quarters split on the native spine band boundaries."""
    section(RUMP_BONE, lambda: sweep('Mount_rump_black',
            [y_ring(96, z=134, width=17, depth=21), y_ring(90, z=139, width=27, depth=32),
             y_ring(74, z=142, width=32.5, depth=36), y_ring(60, z=143, width=34, depth=37.5),
             y_ring(50, z=142, width=34, depth=37.5)], m['Black'], segments=24))
    section(BARREL_BONE, lambda: sweep('Mount_barrel_black',
            [y_ring(50, z=142, width=34, depth=37.5), y_ring(30, z=141, width=33.5, depth=38.5),
             y_ring(10, z=140, width=33, depth=39.5), y_ring(-8, z=141, width=32, depth=38.5),
             y_ring(-24, z=143, width=30.5, depth=36.5)], m['Black'], segments=24))
    section(SADDLE_BONE, lambda: sweep('Mount_mid_barrel_black',
            [y_ring(-24, z=143, width=30.5, depth=36.5), y_ring(-40, z=145, width=29, depth=34.5),
             y_ring(-56, z=148, width=26.5, depth=30.5)], m['Black'], segments=24))
    section(CHEST_BONE, lambda: sweep('Mount_chest_black',
            [y_ring(-56, z=148, width=26.5, depth=30.5), y_ring(-70, z=150, width=21, depth=25),
             y_ring(-78, z=151, width=14, depth=17)], m['Black'], segments=24))
    section(BARREL_BONE, lambda: belly_trim(m))
    section(RUMP_BONE, lambda: rump_trim(m))
    section(RUMP_BONE, lambda: crupper(m))


def crupper(m):
    """Gold crupper band settling over the haunch root."""
    ellipse('Mount_crupper_gold_band', (0, 66, 141), ((34.4, 0, 0), (0, 0, 37.8)),
            m['Gold'], .7, 22)


def belly_trim(m):
    """Gold wave fillets along the barrel bottom (native barrel band)."""
    for side in (1, -1):
        wave(f'Mount_belly_wave_gold_{side}',
             [(side * 10, 44, 107), (side * 6, 20, 103.8), (side * 6, -4, 105.2),
              (side * 5, -20, 106.8)], .55, m['Gold'])


def rump_trim(m):
    """Hip jewels and gold hoop over the rump (native rump band)."""
    for side in (1, -1):
        for index, y in enumerate((74, 56)):
            gem(f'Mount_hip_ocean_pin_{index}_{side}', (side * (33.6 - index * 1.2), y, 142),
                (1.2, .8, 2.8), m['Blue'])
    ellipse('Mount_rump_gold_hoop', (0, 86, 140), ((30, 0, 0), (0, 0, 34)), m['Gold'], .8, 18)


def flank_bard(m):
    """Pointed scalloped bard panels with rune studs over each flank."""
    for side in (1, -1):
        for index, (bone, y_center, top, bottom, half, x_out) in enumerate(
                ((RUMP_BONE, 64, 172, 126, 25, 36.5),
                 (BARREL_BONE, 14, 166, 112, 23, 35.5),
                 (CHEST_BONE, -48, 162, 120, 18, 30))):
            section(bone, lambda yc=y_center, t=top, b=bottom, h=half, x=x_out, s=side, i=index:
                    flank_panel(f'Mount_flank_panel_{i}_{s}', h, t, b, yc, x, s, m))
        section(BARREL_BONE, lambda s=side: flank_runes(m, s))
    section(BARREL_BONE, lambda: flank_scallops(m))


def flank_scallops(m):
    """Gold scale row layered over the bard panel seam, both flanks."""
    profiles = []
    for row, y in enumerate((28, 40, 52)):
        top = 158 - row * 4
        profiles.append([(y - 6, top), (y + 6, top), (y + 3.6, top - 16), (y, top - 19),
                         (y - 3.6, top - 16)])
    for side in (1, -1):
        side_plates(f'Mount_flank_scales_{side}', profiles, 0, 36.4, side, m['Gold'])


def flank_runes(m, side):
    """Ocean-blue rune studs along the flank seam (native barrel band)."""
    for index, (y, z) in enumerate(((34, 152), (12, 146), (-10, 140))):
        gem(f'Mount_flank_rune_gem_{index}_{side}', (side * (35.8 + index * .3), y, z),
            (1.1, .7, 2.6), m['Blue'])


def breastplate(m):
    """Triple-point sea breastplate on the chest front (native chest bone)."""
    section(CHEST_BONE, lambda: chest_front(m))


def chest_front(m):
    outline = [(-24, 168), (-13, 174), (0, 176), (13, 174), (24, 168),
               (20, 140), (10, 128), (0, 124), (-10, 128), (-20, 140)]
    curved_shell('Mount_breast_black', outline, -80, 4, m['Black'], crown_rows=5)
    rim('Mount_breast_gold_edge', outline, -79.2, m['Gold'], .6)
    for offset in (-9, 0, 9):
        mounted_blade(f'Mount_breast_point_gold_{offset}',
                      [(offset, 166 - abs(offset) * .8), (offset + 2.6, 156 - abs(offset) * .8),
                       (offset, 138 - abs(offset)), (offset - 2.6, 156 - abs(offset) * .8)],
                      -81.5, m, 1.4)
    gem('Mount_breast_ocean_gem', (0, -83, 148), (2.2, 1.3, 4.6), m['Blue'])
    wave('Mount_breast_wave_gold', [(-18, -80, 136), (-8, -82.5, 130), (8, -82.5, 130),
                                    (18, -80, 136)], .5, m['Gold'])


def neck_and_crest(m):
    """Black neck tube, gold crest fins and side scale rows (native 21/22)."""
    section(NECK_BONE, lambda: sweep('Mount_neck_black',
            [(Vector((0, -50, 150)), Vector((20, 0, 0)), Vector((0, 2, 24))),
             (Vector((0, -58, 164)), Vector((17.5, 0, 0)), Vector((0, 3, 22))),
             (Vector((0, -66, 180)), Vector((15.5, 0, 0)), Vector((0, 4, 19)))], m['Black'],
            segments=22))
    section(NECK1_BONE, lambda: sweep('Mount_upper_neck_black',
            [(Vector((0, -66, 180)), Vector((15.5, 0, 0)), Vector((0, 4, 19))),
             (Vector((0, -74, 198)), Vector((13.5, 0, 1)), Vector((0, 5, 16))),
             (Vector((0, -81, 213)), Vector((11.5, 0, 2)), Vector((0, 5, 13)))], m['Black'],
            segments=22))
    section(BASE_CREST_BONE, lambda: withers_crest(m))
    section(CREST_BONE, lambda: mane_crest(m))
    # The neck leans forward as it rises (dy/dz ~ -0.49); scale rows follow.
    for bone, z_top, x_out in ((NECK_BONE, 180, 17.4), (NECK1_BONE, 196, 14.2)):
        section(bone, lambda z=z_top, x=x_out, b=bone: neck_scales(m, b, z, x))


def neck_y(z):
    """Neck axis y at height z (bind pose lean of the native neck chain)."""
    return -50 - 0.492 * (z - 150)


def neck_scales(m, bone, z_top, x_out):
    """Pointed gold scale rows hugging the sides of one leaning neck band."""
    shrink = .82 if bone == NECK1_BONE else 1
    for row in range(2):
        top = z_top - row * 16
        center_y = neck_y(top - 8)
        profiles = []
        for along, rise in ((-9, 0), (0, 2), (9, 0)):
            width = (5.5 if along else 6.5) * shrink
            low = top + rise - 18
            profiles.append([(along - width, top + rise), (along + width, top + rise),
                             (along + width * .6, low + 3), (along, low),
                             (along - width * .6, low + 3)])
        side_plates(f'Mount_neck_scales_{bone}_{row}', profiles, center_y, x_out, 1, m['Gold'])
        side_plates(f'Mount_neck_scales_{bone}_{row}_m', profiles, center_y, x_out, -1, m['Gold'])


def withers_crest(m):
    """Gold triple fin rising at the withers (native Bone06 region)."""
    for index, (y, height) in enumerate(((-26, 11), (-16, 16), (-6, 11))):
        blade(f'Mount_withers_fin_gold_{index}',
              [(y - 3.2, 186), (y, 186 + height), (y + 3.2, 186), (y + 2, 180), (y - 2, 180)],
              0, m['Gold'])
    gem('Mount_withers_ocean_gem', (0, -10, 190), (1.3, .8, 3), m['Blue'])


def mane_crest(m):
    """Crest spike row following the head crown (native Bone04 plume bone)."""
    for index, (y, z, height) in enumerate(((-92, 228, 9), (-101, 235, 11), (-109, 241, 8))):
        blade(f'Mount_mane_fin_gold_{index}',
              [(y - 3, z), (y, z + height), (y + 3, z), (y + 2, z - 5), (y - 2, z - 5)],
              0, m['Gold'])


def side_plates(name, profiles, y_center, x_out, side, material, thickness=1.1):
    """Closed pointed plates on a +-X surface; profiles are (y, z) pentagons."""
    for index, profile in enumerate(profiles):
        p = plate(f'{name}_{index}', profile, (0, thickness), material)
        rotate_z(p, 90 * side)
        p.data.transform(Matrix.Translation((side * x_out, y_center, 0)))


def head_bard(m):
    """Chamfron band hugging the raised native head, plus chin and ears."""
    section(HEAD_BONE, lambda: sweep('Mount_head_black',
            [(Vector((0, -82, 214)), Vector((11, 0, 0)), Vector((0, -2, 12))),
             (Vector((0, -92, 224)), Vector((9.5, 0, 0)), Vector((0, -2, 10.5))),
             (Vector((0, -102, 234)), Vector((7.5, 0, 0)), Vector((0, -1.5, 8))),
             (Vector((0, -110, 242)), Vector((5, 0, 0)), Vector((0, -1, 5.5)))], m['Black'],
            segments=16))
    section(HEAD_BONE, lambda: chamfron(m))
    section(HEAD_BONE, lambda: bridle(m))
    section(HEAD_BONE, lambda: chin_medallion(m))
    for side in (1, -1):
        section(HEAD_BONE, lambda s=side: ear_fin(m, s))


def bridle(m):
    """Cheek discs and bit rings of the bridle, hugging the head sweep."""
    for side in (-1, 1):
        ellipse(f'Mount_bridle_cheek_{side}', (side * 9.9, -92, 223.5),
                ((0, 2.6, 0), (0, 0, 2.6)), m['Gold'], .5, 12)
        ellipse(f'Mount_bit_ring_{side}', (side * 5.2, -111, 240.5),
                ((0, 2, 0), (0, 0, 2)), m['Gold'], .45, 10)


def chamfron(m):
    """Face plate wrapping the head axis; front arc with a nose blade."""
    axis = Vector((0, -1, 1)).normalized()
    centers = ((0, -84, 216), (0, -93, 226), (0, -102, 235), (0, -109, 241))
    radii = (12.2, 10.8, 8.6, 6.2)
    u_axis = Vector((1, 0, 0))
    # Arc from the left side (180 degrees) through the muzzle (270) to the
    # right side (360); the crown stays open for the mane crest fins.
    arc_sweep('Mount_chamfron_black', centers, radii, u_axis, axis,
              PI, TAU, m['Black'], steps=10)
    arc_sweep('Mount_chamfron_gold_edge_l', centers, radii, u_axis, axis,
              PI * .98, PI * 1.06, m['Gold'], steps=2)
    arc_sweep('Mount_chamfron_gold_edge_r', centers, radii, u_axis, axis,
              PI * 1.94, TAU * 1.02, m['Gold'], steps=2)
    tube('Mount_nose_blade_gold', [(0, -108, 240), (0, -118, 250)], [3.2, .4],
         m['Gold'], sides=6)
    for side in (-1, 1):
        gem(f'Mount_brow_ocean_gem_{side}', (side * 8.6, -96, 226), (1.1, .7, 2.2), m['Blue'])


def chin_medallion(m):
    """Gorget medal under the jaw of the raised head (head bone)."""
    ellipse('Mount_chin_gold_ring', (0, -95, 215), ((4.5, 0, 0), (0, 0, 4.5)), m['Gold'], .6, 12)
    gem('Mount_chin_ocean_gem', (0, -96.5, 211), (1.2, .9, 2.4), m['Blue'])


def ear_fin(m, side):
    """Gilded pointed ear guard on the upper head side (head bone)."""
    blade(f'Mount_ear_fin_gold_{side}',
          [(-100, 228), (-105, 243), (-110, 228), (-109, 222), (-101, 222)],
          6.8 * side, m['Gold'])


def saddle(m):
    """Rider saddle with scalloped side caparisons on the native saddle band."""
    section(SADDLE_BONE, lambda: saddle_body(m))
    for side in (1, -1):
        section(SADDLE_BONE, lambda s=side: caparison_side(m, s))


def saddle_body(m):
    pad = [(-26, 182), (0, 188), (26, 182), (22, 174), (0, 170), (-22, 174)]
    curved_shell('Mount_saddle_pad_black', pad, -14, 3, m['Black'], crown_rows=4)
    rim('Mount_saddle_gold_border', pad, -13, m['Gold'], .55)
    cantle = [(-14, 180), (0, 184), (14, 180), (10, 198), (0, 202), (-10, 198)]
    curved_shell('Mount_cantle_black', cantle, 16, 3, m['Black'], crown_rows=4)
    rim('Mount_cantle_gold_edge', cantle, 17, m['Gold'], .5)
    ellipse('Mount_pommel_gold_arc', (0, -25, 186), ((9, 0, 0), (0, 0, 8)), m['Gold'], .8, 14)
    gem('Mount_saddle_ocean_gem', (0, -15.5, 188), (1.5, 1, 3.2), m['Blue'])
    saddle_fringe(m)
    # Girth strap closing under the belly, same native band as the saddle.
    sweep('Mount_girth_black',
          [y_ring(4, z=141, width=34.6, depth=41.4), y_ring(-4, z=142, width=34.2, depth=41)],
          m['Black'], segments=22)
    ellipse('Mount_girth_gold_ring', (0, 0, 141.5), ((34.7, 0, 0), (0, 0, 41.5)),
            m['Gold'], .7, 22)


def saddle_fringe(m):
    """Gold fringe plates along the saddle pad rim, hanging over the bard."""
    for index, x in enumerate((-16, 0, 16)):
        plate(f'Mount_saddle_fringe_front_{index}',
              [(x - 4.4, 184), (x + 4.4, 184), (x + 2.6, 178), (x, 175), (x - 2.6, 178)],
              (-15, 1.1), m['Gold'])
    for side in (1, -1):
        for index, z in enumerate((176, 168)):
            p = plate(f'Mount_saddle_fringe_side_{side}_{index}',
                      [(-4.4, z + 4), (4.4, z + 4), (2.6, z - 8), (0, z - 12), (-2.6, z - 8)],
                      (0, 1.1), m['Gold'])
            rotate_z(p, 90 * side)
            p.data.transform(Matrix.Translation((side * 27.5, 2, 0)))


def caparison_side(m, side):
    """Scalloped riding cloth over the flank bard, hanging below the belly."""
    flank_panel(f'Mount_caparison_{side}', 26, 180, 108, -2, 37.8, side, m)
    for index, (y, z) in enumerate(((-14, 150), (4, 146))):
        gem(f'Mount_caparison_rune_gem_{index}_{side}', (side * 38.4, y, z),
            (1.1, .7, 2.4), m['Blue'])


def armored_tail(m):
    """Banded armored tail flowing back, one link per native tail bone."""
    links = ((TAIL_BONES[0], (0, 90, 162), (-9, 105, 136), 7.5, 6),
             (TAIL_BONES[1], (-9, 105, 136), (-38, 106, 110), 6, 4.4),
             (TAIL_BONES[2], (-38, 106, 110), (-70, 101, 98), 4.4, 1.6))
    for index, (bone, start, end, r0, r1) in enumerate(links):
        section(bone, lambda s=start, e=end, a=r0, b=r1, i=index: tail_link(m, i, s, e, a, b))


def tail_link(m, index, start, end, r0, r1):
    """One armored tail segment with gold rings; the last ends in a blade."""
    start, end = Vector(start), Vector(end)
    direction = (end - start).normalized()
    side = direction.cross(Vector((0, 0, 1)))
    if side.length < .3:
        side = Vector((1, 0, 0))
    side.normalize()
    up = direction.cross(side).normalized()
    tube(f'Mount_tail_link_{index}', [tuple(start), tuple(end)], [r0, r1], m['Black'], sides=12)
    mid = start + (end - start) * .5
    tube(f'Mount_tail_sheath_{index}', [tuple(start + (end - start) * .34),
                                        tuple(start + (end - start) * .66)],
         [r0 + (r1 - r0) * .34 + 1, r0 + (r1 - r0) * .66 + 1], m['Gold'], sides=12)
    del mid
    for fraction in (.3, .62):
        center = start + (end - start) * fraction
        radius = r0 + (r1 - r0) * fraction
        ellipse(f'Mount_tail_gold_ring_{index}_{fraction}', tuple(center),
                (side * radius, up * radius), m['Gold'], .55, 12)
    if index == 2:
        tip = end + direction * 14
        tube('Mount_tail_blade_gold', [tuple(end), tuple(tip)], [r1 + .8, .3], m['Gold'], sides=6)
        gem('Mount_tail_ocean_gem', tuple(end + direction * 4 - up * (r1 + 1)),
            (1, .7, 2.2), m['Blue'])


def front_leg(m, side):
    """Barded foreleg: pauldron, sleeved cannon, metal shoe on Toe0."""
    upper, fore, cannon, shoe = FRONT_LEG_BONES[side]
    section(upper, lambda s=side: limb_sleeve(m, f'Mount_front_upper_{s}',
            (side * 13, -62, 168), (side * 15.5, -56, 112), 12.5, 9))
    section(fore, lambda s=side: limb_sleeve(m, f'Mount_front_fore_{s}',
            (side * 15.5, -56, 112), (side * 17, -50, 66), 9, 6.8))
    section(cannon, lambda s=side: limb_sleeve(m, f'Mount_front_cannon_{s}',
            (side * 17, -50, 66), (side * 17.2, -32, 36), 6.8, 5.2))
    section(cannon, lambda s=side: knee_cop(m, f'Mount_front_knee_{s}',
            (side * 16.6, -53, 108), 10.2))
    section(shoe, lambda s=side: hoof(m, f'Mount_front_hoof_{s}',
            (side * 17.2, -32, 36), (side * 17.1, -28, 0), 5.2))
    section(upper, lambda s=side: pauldron(m, s))


def rear_leg(m, side):
    """Barded hind leg: tasset haunch, sleeved gaskin/cannon, metal shoe."""
    thigh, calf, link, foot, toe = REAR_LEG_BONES[side]
    section(thigh, lambda s=side: haunch(m, s))
    section(calf, lambda s=side: limb_sleeve(m, f'Mount_rear_calf_{s}',
            (side * 17, 55, 98), (side * 18, 69, 58), 9.5, 6.8))
    section(link, lambda s=side: limb_sleeve(m, f'Mount_rear_cannon_{s}',
            (side * 18, 69, 58), (side * 19, 72, 26), 6.8, 5))
    section(link, lambda s=side: knee_cop(m, f'Mount_rear_hock_{s}',
            (side * 18.4, 70, 54), 8))
    section(foot, lambda s=side: limb_sleeve(m, f'Mount_rear_pastern_{s}',
            (side * 19, 72, 26), (side * 19, 52, 8), 5, 4.4))
    section(toe, lambda s=side: hoof(m, f'Mount_rear_hoof_{s}',
            (side * 19, 52, 8), (side * 19, 47, -1), 4.4))


def pauldron(m, side):
    """Flared pointed shoulder cop over the native foreleg upper bone."""
    limb_sleeve(m, f'Mount_pauldron_{side}', (side * 12.6, -62, 172),
                (side * 13.4, -60, 138), 15.5, 11.5, rings=(1,), sides=16)
    gem(f'Mount_pauldron_ocean_gem_{side}', (side * 19, -64, 156), (1.2, .8, 2.6), m['Blue'])


def haunch(m, side):
    """Tasset shell over the haunch on the native thigh bone."""
    limb_sleeve(m, f'Mount_haunch_{side}', (side * 16.5, 58, 146), (side * 18.5, 66, 96),
                16, 10, rings=(0.35,), sides=16)
    for index, (y, z, x_out) in enumerate(((70, 128, 24), (58, 118, 27))):
        gem(f'Mount_haunch_ocean_gem_{index}_{side}', (side * x_out, y, z),
            (1.2, .8, 2.8), m['Blue'])


def knee_cop(m, name, center, radius):
    """Gold joint cop ring over knee or hock."""
    ellipse(name, center, ((radius, 0, 0), (0, radius, 0)), m['Gold'], .8, 14)


def hoof(m, name, start, end, radius):
    """Flared metal shoe with a gold fetlock crown on the native Toe0 bones."""
    tube(name, [start, end], [radius, radius * 1.18], m['Black'], sides=12)
    ellipse(name + '_gold_crown', start, ((radius + 1, 0, 0), (0, radius + 1, 0)),
            m['Gold'], .6, 12)
