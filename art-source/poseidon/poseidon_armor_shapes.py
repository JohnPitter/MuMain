"""Original Poseidon Helm, Armor and Boots silhouettes bound to the audited Class305 rig.

Shapes are authored in the native bind-pose world space (Z-up, frame 0 of the
frozen Helm/Armor/BootClass305.bmd skeletons) and skinned with a single full
influence per vertex, matching the MU equipment renderer contract. The visual
language follows design-spec.md: polished black mass, antique gold structure
lines, ocean-blue restricted to jewels; waves, triple points and lozenges tie
the pieces to the weapon prototypes in poseidon_shapes.py.

Rear surfaces are authored facing front (-Y) and mirrored by rear_facing(), so
their recorded depths are negative like any front relief.
"""
import bmesh
import bpy
from math import cos, sin
from math import tau as TAU

from mathutils import Matrix, Vector

from armor_surfaces import loft, section, segment_shell
from celestial_geometry import bezier, ellipse, feather, gem, mesh, plate, rim, tube
from poseidon_shapes import edged_blade

CROWN_BONE, GORGET_BONE = 20, 18


def shift(obj, offset):
    obj.data.transform(Matrix.Translation(offset))
    return obj


def rear_facing(builder):
    """Reflect front-authored reliefs toward +Y while preserving outward winding."""
    previous = set(bpy.data.objects)
    builder()
    for obj in set(bpy.data.objects) - previous:
        obj.data.transform(Matrix.Diagonal((1, -1, 1, 1)))
        topology = bmesh.new()
        topology.from_mesh(obj.data)
        bmesh.ops.reverse_faces(topology, faces=list(topology.faces))
        topology.to_mesh(obj.data)
        topology.free()
        obj.data.update()
        obj['poseidon_facing'] = 'rear'


def dome_shell(name, center, levels, segments, material, open_rows=(), front_limit=0):
    """Closed ellipsoidal dome around center=(x, y); open_rows face windows."""
    vertices, faces = [], []
    for z, radius, depth in levels:
        for i in range(segments):
            angle = TAU * i / segments
            vertices.append((center[0] + radius * cos(angle), center[1] + depth * sin(angle), z))
    for row in range(len(levels) - 1):
        for i in range(segments):
            if row in open_rows and sin(TAU * (i + .5) / segments) < front_limit:
                continue
            a, b = row * segments + i, row * segments + (i + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    apex = len(vertices)
    vertices.append((center[0], center[1], levels[-1][0] + levels[-1][1] * .55))
    last = (len(levels) - 1) * segments
    for i in range(segments):
        faces.append((last + i, last + (i + 1) % segments, apex))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons:
        face.use_smooth = True
    return result


def curved_shell(name, outline, depth, rise, material, crown_rows=6, back_gap=.7):
    """Closed rounded shell with a curved crown; outline is a list of (x, z)."""
    points = [Vector(point) for point in outline]
    center = sum(points, Vector((0, 0))) / len(points)
    count = len(points)
    vertices, faces = [], []
    for row in range(crown_rows):
        radius = 1 - row / crown_rows
        crown = rise * (1 - radius * radius) ** .5
        vertices.extend((point.x, depth - crown, point.y)
                        for point in (center.lerp(point, radius) for point in points))
    for row in range(crown_rows - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    tip = len(vertices)
    vertices.append((center.x, depth - rise, center.y))
    last = (crown_rows - 1) * count
    faces.extend((last + index, last + (index + 1) % count, tip) for index in range(count))
    front_faces = len(faces)
    back = len(vertices)
    vertices.extend((point.x, depth + back_gap, point.y) for point in points)
    back_center = len(vertices)
    vertices.append((center.x, depth + back_gap, center.y))
    for index in range(count):
        following = (index + 1) % count
        faces.append((index, back + index, back + following, following))
        faces.append((back + following, back + index, back_center))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons[:front_faces]:
        face.use_smooth = True
    return result


def rounded(outline, corner_fraction=.16):
    points = [Vector(point) for point in outline]
    result = []
    for index, corner in enumerate(points):
        previous, following = points[index - 1], points[(index + 1) % len(points)]
        start = corner.lerp(previous, corner_fraction)
        end = corner.lerp(following, corner_fraction)
        for t in (0, .5, 1):
            result.append((1 - t) ** 2 * start + 2 * (1 - t) * t * corner + t * t * end)
    return result


def mounted_blade(name, outline, y, materials, thickness=2.0):
    """Weapon-family beveled blade, placed at depth y on a worn surface."""
    previous = set(bpy.data.objects)
    edged_blade(name, outline, materials, thickness)
    return [shift(obj, (0, y, 0)) for obj in set(bpy.data.objects) - previous]


def wave(name, controls, width, material):
    """Gold wave fillet: four bezier controls, a Poseidon signature."""
    return tube(name, bezier(controls, 10), width, material, sides=6)


# ---------------------------------------------------------------- helmet ----
def helm(m):
    section(CROWN_BONE, lambda: crown(m))
    section(GORGET_BONE, lambda: gorget(m))


def crown(m):
    dome_shell('Helm_dome_black_crown', (0, 0),
               [(160.5, 10.2, 10.6), (166.5, 12.1, 12.5), (173, 12.5, 12.9),
                (179.2, 10.8, 11.2), (183.6, 6.6, 7), (185.6, 2.2, 2.4)],
               24, m['Black'], open_rows=(0, 1, 2), front_limit=-.3)
    brow = [(-10.6, 168.5), (-4.6, 172.2), (0, 173.2), (4.6, 172.2),
            (10.6, 168.5), (4.4, 167.4), (0, 167.9), (-4.4, 167.4)]
    plate('Helm_brow_black_band', brow, (-11.9, 1.2), m['Black'])
    rim('Helm_brow_gold_line', [(-9.8, 170.6), (-4, 174), (0, 174.8), (4, 174),
                                (9.8, 170.6), (0, 171.6)], -13, m['Gold'], .5)
    for side in (-1, 1):
        cheek = [(side * 8.2, 157.5), (side * 11.9, 162.5), (side * 12.6, 168.5),
                 (side * 10.2, 173.5), (side * 7.4, 168.2), (side * 6.9, 160.8)]
        curved_shell(f'Helm_cheek_guard_{side}', cheek, -8.4, 2.2, m['Black'])
        rim(f'Helm_cheek_gold_rim_{side}', cheek, -9, m['Gold'], .55)
        wave(f'Helm_temple_wave_{side}',
             [(side * 8.6, -1.5, 166.5), (side * 14.2, -1.5, 170.5),
              (side * 15.4, -1.5, 176.5), (side * 11.8, -1.5, 181.5)], .55, m['Gold'])
        tine = [(side * 6.8, 171.5), (side * 10.8, 179.5), (side * 12.8, 188.5),
                (side * 11.2, 196.5), (side * 8.7, 190.5), (side * 9.5, 182.5),
                (side * 7.6, 175.5)]
        mounted_blade(f'Helm_crown_outer_tine_{side}', tine, -1.2, m, 1.5)
        tube(f'Helm_tine_gold_fluting_{side}',
             [(side * 8.2, -2.4, 174), (side * 11.6, -2.4, 183), (side * 11.2, -2.4, 191)],
             [.32, .42, .1], m['Gold'], sides=6)
        ellipse(f'Helm_ear_disc_{side}', (side * 11.6, -.6, 167),
                ((0, 3.1, 0), (0, 0, 3.1)), m['Gold'], .45, 16)
        gem(f'Helm_temple_jewel_{side}', (side * 10.6, -5.6, 176.5), (1, .6, 2.4), m['Blue'])
    mounted_blade('Helm_crown_trident_blade',
                  [(0, 175.5), (2.7, 182.5), (1.9, 190.5), (3.2, 202), (2.3, 211),
                   (0, 217.5), (-2.3, 211), (-3.2, 202), (-1.9, 190.5), (-2.7, 182.5)],
                  -.6, m, 2.0)
    gem('Helm_axial_ocean_crystal', (0, -3.4, 188), (1.5, .75, 6.5), m['Blue'])
    gem('Helm_axial_rear_seal', (0, 1.9, 188), (.8, .5, 3.5), m['Blue'])
    lozenge = [(0, 178.5), (3.4, 174.6), (0, 170.8), (-3.4, 174.6)]
    rim('Helm_forehead_diamond_setting', lozenge, -12.6, m['Gold'], .6)
    gem('Helm_forehead_ocean_diamond', (0, -13.6, 174.6), (2.1, 1.2, 4.8), m['Blue'])


def gorget(m):
    for index, (low, high, width) in enumerate(((149, 152.6, 8.8), (153, 156, 8.2),
                                                (156.6, 159.2, 7.2))):
        outline = [(-width, low + .8), (-width * .55, high), (0, high + .5),
                   (width * .55, high), (width, low + .8), (0, low)]
        plate(f'Helm_nape_lamella_{index}', outline, (9.2 - index * .4, 1.2), m['Black'])
        rim(f'Helm_nape_gold_edge_{index}', outline, 9.7 - index * .4, m['Gold'], .5)
    occipital = mounted_blade('Helm_occipital_diamond',
                              [(0, 158.2), (2.4, 154.6), (0, 151), (-2.4, 154.6)], 10.2, m, 1.3)
    throat = [(-8, 153.5), (0, 154.6), (8, 153.5), (6, 150.4), (0, 149.9), (-6, 150.4)]
    gorget_parts = occipital + [
        tube('Helm_throat_gold_band', [(-7.5, -6.8, 151.5), (-3.5, -9.8, 150.8),
             (3.5, -9.8, 150.8), (7.5, -6.8, 151.5)], 1.05, m['Gold'], sides=8),
        plate('Helm_throat_black_plate', throat, (-9.2, 1.6), m['Black']),
        rim('Helm_throat_gold_edge', throat, -9.7, m['Gold'], .5)]
    for obj in gorget_parts:
        obj['poseidon_part'] = 'gorget'


# ----------------------------------------------------------------- armor ----
def armor_torso_bone(obj, position):
    return 18 if position.z >= 133 else 17


def armor(m):
    section(armor_torso_bone, lambda: cuirass(m))
    section(armor_torso_bone, lambda: rear_facing(lambda: back_armor(m)))
    for side, clavicle, arm, forearm in ((1, 34, 35, 36), (-1, 25, 26, 27)):
        section(clavicle, lambda s=side: clavicle_trim(m, s))
        section(arm, lambda s=side: pauldron(m, s, s > 0))
        section(arm, lambda s=side: upper_arm_sleeve(m, s))
        section(arm, lambda s=side: elbow_cop(m, s))
        section(forearm, lambda s=side: forearm_fin(m, s))


def cuirass(m):
    loft('Armor_anatomical_cuirass_black',
         [((0, 0, z), width, depth) for z, width, depth in
          ((106, 11.5, 9), (116, 12, 9.2), (128, 16.5, 11.8),
           (140, 19.5, 12.8), (150, 17.5, 11), (156.5, 12.5, 9))], m['Black'])
    ellipse('Armor_standing_collar_gold', (0, .5, 157.8), ((10.6, 0, 0), (0, 8.6, 0)),
            m['Gold'], 1.1, 18)
    ellipse('Armor_belt_gold', (0, .3, 108.5), ((12, 0, 0), (0, 9.5, 0)), m['Gold'], 1.2, 18)
    mounted_blade('Armor_belt_lozenge', [(0, 111.5), (3, 108), (0, 104.5), (-3, 108)],
                  -10.6, m, 1.5)
    gem('Armor_belt_ocean_gem', (0, -11.9, 108), (1.6, 1, 3.4), m['Blue'])
    for side in (-1, 1):
        chest = rounded([(side * 2.2, 151.5), (side * 13.5, 157), (side * 19.5, 148.5),
                         (side * 15, 136.5), (side * 4.5, 133.5)])
        curved_shell(f'Armor_sculpted_pectoral_{side}', chest, -13.6, 3, m['Black'])
        rim(f'Armor_pectoral_gold_border_{side}', chest, -14.3, m['Gold'], .6)
        wave(f'Armor_chest_wave_gold_{side}',
             [(side * 5, -14.6, 151), (side * 12, -15.6, 146),
              (side * 17, -14, 140), (side * 13, -12.9, 133)], .55, m['Gold'])
        wave(f'Armor_abdomen_wave_gold_{side}',
             [(side * 4, -12.6, 124), (side * 9.5, -13.1, 119),
              (side * 12, -11.6, 113), (side * 7, -10.9, 108)], .5, m['Gold'])
    lozenge = [(0, 155), (4.2, 146), (0, 137), (-4.2, 146)]
    rim('Armor_sternum_diamond_setting', lozenge, -15.7, m['Gold'], .65)
    gem('Armor_sternum_ocean_diamond', (0, -16.7, 146), (2.9, 1.7, 6.6), m['Blue'])
    feather('Armor_abdominal_spear_gold', [(0, -14.2, 133), (0, -14.9, 123),
            (0, -13, 114), (0, -11.7, 106)], (3.1, .95), m['Gold'])


def back_armor(m):
    """Authored facing front; rear_facing() mirrors it onto the back."""
    for side in (-1, 1):
        scapula = rounded([(side * 3, 152), (side * 11, 154.5), (side * 15.5, 147),
                           (side * 12, 138.5), (side * 4, 140)])
        curved_shell(f'Armor_scapula_shell_{side}', scapula, -13.4, 2.4, m['Black'])
        rim(f'Armor_scapula_gold_border_{side}', scapula, -14.1, m['Gold'], .55)
        wave(f'Armor_back_wave_gold_{side}',
             [(side * 4, -13.2, 149), (side * 9.5, -13.6, 143),
              (side * 11, -13.2, 136), (side * 6, -12.8, 130)], .5, m['Gold'])
    mounted_blade('Armor_back_trident_brand',
                  [(0, 156), (2.2, 150), (1.4, 144.5), (2.6, 138), (0, 133.5),
                   (-2.6, 138), (-1.4, 144.5), (-2.2, 150)], -13.2, m, 1.6)
    for index, (low, width) in enumerate(((119.5, 11.5), (114.5, 11), (109.5, 10.5))):
        outline = [(-width, low), (-width * .35, low + .9), (width * .35, low + .9),
                   (width, low), (width * .7, low - 3.4), (0, low - 4), (-width * .7, low - 3.4)]
        plate(f'Armor_lower_back_lamella_{index}', outline, (-(11.2 - index * .8), 1.1), m['Black'])
        rim(f'Armor_lamella_gold_edge_{index}', outline, -(11.8 - index * .8), m['Gold'], .45)


def clavicle_trim(m, side):
    ellipse(f'Armor_clavicle_stud_{side}', (side * 12.5, -8.6, 152.5),
            ((2.2, 0, 0), (0, 0, 2.2)), m['Gold'], .45, 16)


def pauldron(m, side, fan_shoulder):
    center = (side * 20, 0)
    if fan_shoulder:
        dome_shell(f'Armor_pauldron_fan_dome_{side}', center,
                   [(150, 7.8, 7.2), (154.5, 10.2, 9), (158, 9.2, 8.2),
                    (160.5, 5.4, 4.8), (162, 1.8, 1.6)], 16, m['Black'])
        for index in range(5):
            controls = [(side * (19.5 + index * .4), 1.5 + index * .8, 156.5 - index * 1.2),
                        (side * (24 + index * 1.4), 4 + index * 1.5, 162 - index * 1.5),
                        (side * (27.5 + index * 1.6), 6.5 + index * 1.6, 172.5 - index * 2.2),
                        (side * (28.5 + index * 1.2), 7.5 + index * 1.5, 184 - index * 4)]
            feather(f'Armor_pauldron_fan_plume_{index}', controls,
                    (2.6 - index * .25, .75, 9), m['Black'])
            if index % 2 == 0:
                tube(f'Armor_plume_gold_spine_{index}',
                     [(side * (20.5 + index * .9), 2.5 + index * 1.2, 157.5 - index * 1.4),
                      (side * (25.5 + index * 1.5), 5 + index * 1.5, 166 - index * 1.8),
                      (side * (28.2 + index * 1.4), 7 + index * 1.5, 179 - index * 3)],
                     [.3, .34, .06], m['Gold'], sides=6)
        gem(f'Armor_pauldron_ocean_gem_{side}', (side * 22.5, -6.5, 156), (2.2, 1.1, 5), m['Blue'])
    else:
        dome_shell(f'Armor_pauldron_curved_guard_{side}', (side * 22, 1.5),
                   [(146, 8, 6.5), (151, 12, 10), (155.5, 12.5, 11),
                    (159, 9.5, 8.5), (161.5, 4.5, 4)], 20, m['Black'])
        ellipse(f'Armor_guard_gold_halo_{side}', (side * 22, 1.5, 149.5),
                ((9.5, 0, 0), (0, 9.5, 0)), m['Gold'], .7, 20)
        wave(f'Armor_guard_wave_undercurl_{side}',
             [(side * 16, -9, 146), (side * 22, -11, 143),
              (side * 28, -9.5, 145), (side * 31, -6, 149)], .6, m['Gold'])
        for index in range(2):
            feather(f'Armor_guard_front_spire_{index}',
                    [(side * (20 + index * 1.6), -4 - index, 158 + index),
                     (side * (24 + index * 1.6), -9 - index, 164 + index),
                     (side * (25.5 + index * 1.6), -13 - index, 170 + index),
                     (side * (24 + index * 1.6), -15 - index, 175 + index)],
                    (2.2 - index * .4, .7), m['Gold'])
        gem(f'Armor_guard_ocean_gem_{side}', (side * 21, -12.4, 152), (2, 1, 4.6), m['Blue'])


def upper_arm_sleeve(m, side):
    segment_shell(f'Armor_upper_arm_sleeve_{side}',
                  ((side * 20.2, 0, 152), (side * 25.4, -1.5, 124)),
                  [(0, 8.5, 8), (.35, 8.8, 8), (.7, 6.6, 6.4), (1, 5.8, 5.6)], m['Black'])
    feather(f'Armor_sleeve_wave_gold_{side}',
            [(side * 22, -7.5, 146), (side * 27, -8.5, 139),
             (side * 27.5, -7, 131), (side * 24.5, -6, 126)], (1.4, .5), m['Gold'])


def elbow_cop(m, side):
    dome_shell(f'Armor_elbow_cop_{side}', (side * 26, -.5),
               [(116.5, 5, 4.8), (119.5, 6.2, 6), (122, 5, 4.9)], 12, m['Black'])
    ellipse(f'Armor_elbow_gold_ring_{side}', (side * 26, -.5, 118.2),
            ((5.9, 0, 0), (0, 5.9, 0)), m['Gold'], .5, 14)


def forearm_fin(m, side):
    for index in range(2):
        feather(f'Armor_forearm_fin_{index}',
                [(side * (27 + index), -6, 116 - index * 3),
                 (side * (29.5 + index), -7, 108 - index * 3),
                 (side * (30 + index), -6.5, 100 - index * 3),
                 (side * (28.5 + index), -5.5, 95 - index * 3)], (1.9 - index * .3, .6), m['Black'])


# ----------------------------------------------------------------- boots ----
def boots(m):
    for side, calf, foot in ((1, 4, 5), (-1, 11, 12)):
        section(calf, lambda s=side: greave(m, s))
        section(foot, lambda s=side: sabaton(m, s))
        section(calf, lambda s=side: rear_facing(lambda: rear_calf(m, s)))


def greave(m, side):
    center = side * 10.3
    segment_shell(f'Boots_curved_greave_{side}',
                  ((center, -.6, 15.5), (center, -4.6, 57)),
                  [(0, 4.3, 4.6), (.3, 5.4, 5.6), (.65, 7.3, 7), (1, 6.4, 6.2)], m['Black'])
    shin = [(center, 55), (side * 17.8, 46), (side * 16.2, 31),
            (side * 12.6, 22), (side * 8.4, 24.5), (side * 7.6, 47)]
    curved_shell(f'Boots_shin_plate_{side}', shin, -9.6, 2.6, m['Black'])
    rim(f'Boots_shin_gold_frame_{side}', shin, -10.3, m['Gold'], .55)
    wave(f'Boots_shin_wave_outer_{side}',
         [(center, -10, 26), (side * 14.2, -9.6, 36),
          (side * 12.4, -8.8, 46), (side * 10.6, -8.4, 53)], .45, m['Gold'])
    wave(f'Boots_shin_wave_inner_{side}',
         [(center, -10, 28), (side * 6.6, -9.6, 38),
          (side * 7.4, -8.8, 45), (side * 9.6, -8.4, 50)], .4, m['Gold'])
    lozenge = [(center, 50), (side * 13, 45.5), (center, 41), (side * 7.6, 45.5)]
    rim(f'Boots_shin_diamond_setting_{side}', lozenge, -9.4, m['Gold'], .45)
    gem(f'Boots_shin_ocean_diamond_{side}', (center, -10.2, 45.5), (1.8, .9, 4), m['Blue'])


def sabaton(m, side):
    center = side * 10.28
    foot_shell(f'Boots_sabaton_shell_{side}', center, m['Black'])
    sole(m, side, center)
    for index in range(3):
        instep_scale(f'Boots_instep_lamella_{index}', m, side, center,
                     -4.5 - index * 4.6, 3.2 + index * .5, 10.6 - index * 1.2)
    toe = [(center - 2.6, 3.4), (center, 2.8), (center + 2.6, 3.4),
           (center + 2.2, 6.6), (center - 2.2, 6.6)]
    curved_shell(f'Boots_toe_guard_{side}', toe, -15.5, 1.8, m['Black'], crown_rows=4, back_gap=.5)
    rim(f'Boots_toe_gold_edge_{side}', toe, -16, m['Gold'], .4)
    ellipse(f'Boots_ankle_gold_collar_{side}', (center, -.5, 15),
            ((4.6, 0, 0), (0, 5.6, 0)), m['Gold'], .9, 16)
    rear_facing(lambda: heel_counter(m, side, center))


def foot_shell(name, center, material):
    profile = ((9, 4.6, 1.2, 10.5), (5.5, 5.4, .9, 12.6), (3, 4.9, 1, 13.6),
               (1, 6.2, .8, 13.2),
               (-4, 6.4, .8, 12.6), (-9.5, 5.8, 1, 10.4), (-14.5, 3.6, 1.4, 6.8),
               (-18, 1.2, 2.2, 4.2))
    sections = [foot_section(center, row) for row in profile]
    vertices = [point for section in sections for point in section]
    count = len(sections[0])
    faces = []
    for row in range(len(sections) - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    faces.append(tuple(range(count - 1, -1, -1)))
    base = (len(sections) - 1) * count
    faces.append(tuple(base + index for index in range(count)))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons:
        face.use_smooth = True
    return result


def foot_section(center, row):
    y, width, bottom, top = row
    shoulder = bottom + (top - bottom) * .64
    return [(center - width * .85, y, bottom), (center - width, y, bottom + .65),
            (center - width, y, shoulder), (center - width * .58, y, top - .3),
            (center, y, top), (center + width * .58, y, top - .3),
            (center + width, y, shoulder), (center + width, y, bottom + .65),
            (center + width * .85, y, bottom)]


def sole(m, side, center):
    profile = ((9, 4.6, 1.2), (5.5, 5.4, .9), (3, 4.9, 1), (1, 6.2, .8), (-4, 6.4, .8),
               (-9.5, 5.8, 1), (-14.5, 3.6, 1.4), (-18, 1.2, 2.2))
    boundary = [(center - w * .91, y, bottom) for y, w, bottom in profile]
    boundary += [(center + w * .91, y, bottom) for y, w, bottom in reversed(profile)]
    count = len(boundary)
    vertices = boundary + [(x, y, z - .8) for x, y, z in boundary]
    faces = [(index, (index + 1) % count, count + (index + 1) % count, count + index)
             for index in range(count)]
    faces.extend((tuple(range(count)), tuple(reversed(range(count, count * 2)))))
    mesh(f'Boots_gilded_sole_{side}', vertices, faces, m['Gold'])


def instep_scale(name, m, side, center, y, width, height):
    outline = [(center - width, y + 1.5, height - 2.2), (center, y + 2.4, height),
               (center + width, y + 1.5, height - 2.2), (center + width * .8, y - 1.8, height - 2.6),
               (center, y - 2.8, height - 1.2), (center - width * .8, y - 1.8, height - 2.6)]
    count = len(outline)
    vertices = outline + [(x, depth - .6, z) for x, depth, z in outline]
    faces = [(0, 1, 2, 3, 4, 5), (9, 8, 7, 6, 11, 10)]
    faces.extend((n, (n + 1) % count, count + (n + 1) % count, count + n) for n in range(count))
    mesh(name, vertices, faces, m['Black'])
    chevron = [(outline[4][0], outline[4][1] + .12, outline[4][2]),
               (outline[3][0], outline[3][1] + .12, outline[3][2]),
               (outline[2][0], outline[2][1] + .12, outline[2][2])]
    tube(name + '_gold_chevron', chevron, .35, m['Gold'], sides=6)


def heel_counter(m, side, center):
    """Authored facing front; rear_facing() mirrors it onto the heel."""
    outline = [(center - 4.4, 8), (center, 10.8), (center + 4.4, 8), (center + 4.2, 1.5),
               (center + 1.6, 5.2), (center - 1.6, 5.2), (center - 4.2, 1.5), (center, .8)]
    plate(f'Boots_notched_heel_counter_{side}', outline, (-9.6, 1.2), m['Black'])
    rim(f'Boots_heel_gold_edge_{side}', outline, -10.1, m['Gold'], .45)


def rear_calf(m, side):
    """Authored facing front; rear_facing() mirrors it onto the calf."""
    center = side * 10.3
    for direction in (-1, 1):
        for index in range(2):
            feather(f'Boots_rear_calf_lamella_{side}_{direction}_{index}',
                    [(center + direction * 4.2, -6.5, 20 + index * 9),
                     (center + direction * 6.4, -8, 27 + index * 9),
                     (center + direction * 5.8, -6.5, 35 + index * 9),
                     (center + direction * 3.6, -5, 43 + index * 8)], (1.5, .45), m['Black'])
    feather(f'Boots_achilles_spine_gold_{side}',
            [(center, -7.2, 18), (center, -8.6, 28), (center, -8.2, 40), (center, -6.4, 52)],
            (1.9, .5), m['Gold'])
    lozenge = [(center, 52), (center + 2.2, 48.5), (center, 45), (center - 2.2, 48.5)]
    rim(f'Boots_rear_diamond_setting_{side}', lozenge, -8, m['Gold'], .4)
    gem(f'Boots_rear_ocean_diamond_{side}', (center, -8.6, 48.5), (1.4, .7, 3), m['Blue'])
