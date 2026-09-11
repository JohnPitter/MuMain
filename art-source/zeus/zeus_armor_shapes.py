"""Original Zeus Armor, Pant, Glove and Boot silhouettes on the audited Class304 rig.

Continues the weapon prototypes' language from zeus_shapes.py onto the body:
celestial-blue faceted mass, platinado structure lines and electric channels,
emissive blue restricted to gems and energy grooves. Family motifs are angular:
"V" chest facets, lightning zigzags (sharp polyline bolts - never the Poseidon
wave), multi-point stars as axial ornaments and bolt-tip wedges on shoulders
and knees. Shapes are authored in the native bind-pose world space (Z-up,
frame 0 of the frozen *Class304.bmd skeletons - the same frame-0 bind as the
audited Class305 family) with single full influences, and the binding bones
are exactly the native distribution measured in the audit:
Armor -> 2/3/10/17/18/25/26/27/34/35/36, Pant -> 0/2/3/4/10/11/17,
Glove -> 27/28/29/36/37/38, Boot -> 4/5/11/12.

Rear surfaces are authored facing front (-Y) and mirrored by rear_facing(), so
their recorded depths are negative like any front relief.
"""
import bmesh
import bpy
from math import cos, sin
from math import tau as TAU

from mathutils import Matrix

from armor_surfaces import loft, section, segment_shell
from celestial_geometry import ellipse, feather, gem, mesh, plate, tube
from zeus_shapes import edged_blade, star_outline

SPINE_BONE, CHEST_BONE, PELVIS_BONE, ROOT_BONE = 17, 18, 2, 0
CLAVICLE_BONES = {1: 34, -1: 25}
ARM_BONES = {1: 35, -1: 26}      # upper arms carry the integrated pauldrons
FOREARM_BONES = {1: 36, -1: 27}
THIGH_BONES = {1: 3, -1: 10}     # +X is the left leg in the native bind pose
CALF_BONES = {1: 4, -1: 11}
FOOT_BONES = {1: 5, -1: 12}
HAND_BONES = {1: 37, -1: 28}
FINGER_BONES = {1: 38, -1: 29}


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
        obj['zeus_facing'] = 'rear'


def dome_shell(name, center, levels, segments, material):
    """Closed faceted dome around center=(x, y)."""
    vertices, faces = [], []
    for z, radius, depth in levels:
        for i in range(segments):
            angle = TAU * i / segments
            vertices.append((center[0] + radius * cos(angle), center[1] + depth * sin(angle), z))
    for row in range(len(levels) - 1):
        for i in range(segments):
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
    """Closed angular shell with a faceted crown; outline is a list of (x, z)."""
    points = [point for point in outline]
    center = [sum(point[axis] for point in points) / len(points) for axis in range(2)]
    count = len(points)
    vertices, faces = [], []
    for row in range(crown_rows):
        radius = 1 - row / crown_rows
        crown = rise * (1 - radius * radius) ** .5
        vertices.extend((point[0], depth - crown, point[1])
                        for point in ((center[0] + (point[0] - center[0]) * radius,
                                       center[1] + (point[1] - center[1]) * radius)
                                      for point in points))
    for row in range(crown_rows - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    tip = len(vertices)
    vertices.append((center[0], depth - rise, center[1]))
    last = (crown_rows - 1) * count
    faces.extend((last + index, last + (index + 1) % count, tip) for index in range(count))
    front_faces = len(faces)
    back = len(vertices)
    vertices.extend((point[0], depth + back_gap, point[1]) for point in points)
    back_center = len(vertices)
    vertices.append((center[0], depth + back_gap, center[1]))
    for index in range(count):
        following = (index + 1) % count
        faces.append((index, back + index, back + following, following))
        faces.append((back + following, back + index, back_center))
    result = mesh(name, vertices, faces, material)
    for face in result.data.polygons[:front_faces]:
        face.use_smooth = True
    return result


def mounted_outline(name, outline, y, materials, thickness=1.5):
    """Beveled blade (star, fan petal, bolt tip) placed at depth y on a surface."""
    previous = set(bpy.data.objects)
    edged_blade(name, outline, materials, thickness)
    for obj in set(bpy.data.objects) - previous:
        shift(obj, (0, y, 0))


def mounted_star(name, center_x, center_z, outer, inner, points, y, materials,
                 thickness=1.6):
    """Beveled Zeus star ornament centered at (center_x, center_z), depth y."""
    outline = [(center_x + x, z) for x, z in star_outline(center_z, outer, inner, points)]
    mounted_outline(name, outline, y, materials, thickness)
    return outline


def bolt(name, path, radii, material):
    """Electric zigzag: sharp straight polyline tube, the Zeus family line."""
    return tube(name, path, radii, material, sides=6)


def trim(name, outline, depth, material, width=.5):
    """Square-profile platinado edge; the angular trim of the Zeus family."""
    path = [(x, depth, z) for x, z in outline]
    return tube(name, path + [path[0]], width, material, sides=4)


def zigzag(x, z_low, z_high, swings, y):
    """Vertical lightning polyline alternating across x with fixed depth y."""
    points = []
    for index in range(len(swings) * 2):
        offset = swings[index // 2] * (-1 if index % 2 == 0 else 1)
        points.append((x + offset, y,
                       z_low + (z_high - z_low) * index / (len(swings) * 2 - 1)))
    return points


# ----------------------------------------------------------------- armor ----
def armor_torso_bone(obj, position):
    return CHEST_BONE if position.z >= 133 else SPINE_BONE


def armor(m):
    section(armor_torso_bone, lambda: cuirass(m))
    section(armor_torso_bone, lambda: rear_facing(lambda: back_armor(m)))
    section(PELVIS_BONE, lambda: hip_guards(m))
    for side in (1, -1):
        section(THIGH_BONES[side], lambda s=side: tasset(m, s))
        section(CLAVICLE_BONES[side], lambda s=side: clavicle_trim(m, s))
        section(ARM_BONES[side], lambda s=side: upper_arm_sleeve(m, s))
        section(ARM_BONES[side], lambda s=side: pauldron(m, s))
        section(FOREARM_BONES[side], lambda s=side: elbow_cop(m, s))
        section(FOREARM_BONES[side], lambda s=side: forearm_fin(m, s))


def cuirass(m):
    loft('Armor_faced_cuirass_blue',
         [((0, 0, z), width, depth) for z, width, depth in
          ((106, 11.5, 9), (116, 12, 9.2), (128, 16.5, 11.8),
           (140, 19.5, 12.8), (150, 17.5, 11), (156.5, 12.5, 9))], m['Blue'])
    ellipse('Armor_standing_collar_platina', (0, .5, 157.8), ((10.6, 0, 0), (0, 8.6, 0)),
            m['Platina'], 1.1, 14)
    ellipse('Armor_storm_belt_platina', (0, .3, 108.5), ((12, 0, 0), (0, 9.5, 0)),
            m['Platina'], 1.2, 14)
    mounted_star('Armor_belt_star_buckle', 0, 108, 4.8, 2, 6, -11.2, m, 1.5)
    gem('Armor_bolt_core_gem', (0, -12.4, 108), (1.6, 1, 3.2), m['Emissive'])
    for side in (-1, 1):
        chest = [(side * 2, 152.5), (side * 13.8, 156.5), (side * 19.2, 148),
                 (side * 14.8, 136.5), (side * 4.4, 133.8), (side * 2, 142)]
        curved_shell(f'Armor_chest_v_facet_{side}', chest, -13.6, 3, m['Blue'])
        trim(f'Armor_chest_platina_seam_{side}', chest, -14.3, m['Platina'], .6)
        bolt(f'Armor_chest_energy_channel_{side}',
             zigzag(side * 8.6, 137, 152, (2.6, 1.4, 2.2), -14.8),
             [.5, .42, .44, .38, .4, .08], m['Emissive'])
        bolt(f'Armor_abdomen_energy_channel_{side}',
             zigzag(side * 6, 111, 126, (1.8, 1.1, 1.4), -12.4),
             [.42, .36, .38, .32, .34, .08], m['Emissive'])
    trim('Armor_sternum_star_setting', star_outline(146, 4.6, 1.9, 8), -15.6,
        m['Platina'], .65)
    gem('Armor_sternum_celestial_star', (0, -16.6, 146), (2.7, 1.6, 6), m['Emissive'])
    feather('Armor_sternum_platina_keel', [(0, -14.4, 133), (0, -15.1, 123),
            (0, -13.2, 114), (0, -11.9, 106)], (2.6, .9), m['Platina'])
    for index, (low, width) in enumerate(((124.5, 10.2), (116.5, 9.8))):
        outline = [(-width, low), (-width * .3, low + 1.6), (width * .3, low + 1.6),
                   (width, low), (width * .65, low - 3.2), (0, low - 4.2), (-width * .65, low - 3.2)]
        plate(f'Armor_abdominal_v_lamella_{index}', outline, (-(11.9 - index * .5), 1.2), m['Blue'])
        trim(f'Armor_lamella_platina_edge_{index}', outline, -(12.5 - index * .5), m['Platina'], .5)


def back_armor(m):
    """Authored facing front; rear_facing() mirrors it onto the back."""
    for side in (-1, 1):
        mounted_star(f'Armor_scapula_storm_star_{side}', side * 9.2, 146.5, 8.4, 3.3, 5,
                     -12.8, m, 1.5)
        chevron = [(side * 2.4, 138.5), (side * 10.4, 141), (side * 14.6, 133.5),
                   (side * 10.8, 126), (side * 3.4, 124.5), (side * 2.4, 131)]
        curved_shell(f'Armor_back_v_facet_{side}', chevron, -12.6, 2.2, m['Blue'])
        trim(f'Armor_back_platina_seam_{side}', chevron, -13.3, m['Platina'], .55)
    bolt('Armor_spine_energy_channel',
         [(0, -12.6, 156), (.9, -12.9, 149), (-.9, -13.1, 142), (.9, -13.3, 135),
          (-.9, -13.5, 128), (0, -13.7, 121)], [.6, .5, .52, .46, .42, .1], m['Emissive'])
    for index, (low, width) in enumerate(((119.5, 11.5), (114.5, 11), (109.5, 10.5))):
        outline = [(-width, low), (-width * .35, low + .9), (width * .35, low + .9),
                   (width, low), (width * .7, low - 3.4), (0, low - 4), (-width * .7, low - 3.4)]
        plate(f'Armor_lower_back_lamella_{index}', outline, (-(11.2 - index * .8), 1.1), m['Blue'])
        trim(f'Armor_back_lamella_platina_edge_{index}', outline, -(11.8 - index * .8),
            m['Platina'], .45)


def hip_guards(m):
    """Pelvis-bound angular hip bells flanking the storm belt."""
    for side in (-1, 1):
        guard = [(side * 4.5, 104.5), (side * 12.6, 105.8), (side * 15.2, 100.2),
                 (side * 11.4, 94.6), (side * 4.6, 95.4), (side * 4.5, 100)]
        curved_shell(f'Armor_hip_guard_{side}', guard, -10.4, 2, m['Blue'])
        trim(f'Armor_hip_platina_edge_{side}', guard, -11.1, m['Platina'], .5)
        gem(f'Armor_hip_energy_pin_{side}', (side * 11.2, -11.6, 100.2), (1.1, .7, 2.4),
            m['Emissive'])


def tasset(m, side):
    """Small faceted plate strapped over the native thigh top (bone 3/10)."""
    axis = side * 10.8
    outline = [(axis - side * 3.4, 107.5), (axis + side * 1.4, 108.6),
               (axis + side * 5.4, 105.4), (axis + side * 4.4, 100.6),
               (axis + side * .6, 99.2), (axis - side * 3, 101.8)]
    curved_shell(f'Armor_tasset_plate_{side}', outline, -9.8, 1.8, m['Blue'])
    trim(f'Armor_tasset_platina_edge_{side}', outline, -10.5, m['Platina'], .45)
    gem(f'Armor_tasset_energy_pin_{side}', (axis + side * 1.4, -10.9, 104.2), (.9, .6, 1.8),
        m['Emissive'])


def clavicle_trim(m, side):
    ellipse(f'Armor_clavicle_platina_stud_{side}', (side * 12.5, -8.6, 152.5),
            ((2.2, 0, 0), (0, 0, 2.2)), m['Platina'], .45, 10)


def pauldron(m, side):
    """High angular pauldron: faceted dome crowned by layered bolt-tip fans."""
    dome_shell(f'Armor_pauldron_faceted_dome_{side}', (side * 20.4, 0),
               [(148.5, 7.6, 7), (153, 9.9, 8.8), (156.8, 8.8, 7.8),
                (159.4, 5.2, 4.6), (161, 1.7, 1.5)], 12, m['Blue'])
    ellipse(f'Armor_pauldron_platina_halo_{side}', (side * 20.4, 0, 149.3),
            ((8.4, 0, 0), (0, 8, 0)), m['Platina'], .7, 12)
    for index in range(3):
        spread = 3.6 + index * 1.35
        outline = [(side * (spread - 2.6), 154.5 + index * .8),
                   (side * (spread + 2.2), 158.5 + index * 1.4),
                   (side * (spread + 4.6), 165.5 + index * 2.2),
                   (side * (spread + 5.6), 174.5 + index * 3.2),
                   (side * (spread + 3.4), 172.5 + index * 3),
                   (side * (spread + .8), 164 + index * 2.2),
                   (side * (spread - 2), 157.5 + index * 1.2)]
        mounted_outline(f'Armor_pauldron_bolt_fan_{index}_{side}', outline,
                        -1.4 + index * 1.15, m, 1.5)
        bolt(f'Armor_fan_spine_platina_{index}_{side}',
             [(side * (spread + 2.2), -1.4 + index * 1.15, 157.5 + index * 1.4),
              (side * (spread + 4.4), -1.4 + index * 1.15, 165.5 + index * 2.4),
              (side * (spread + 5.2), -1.4 + index * 1.15, 173.5 + index * 3)],
             [.3, .34, .08], m['Platina'])
    gem(f'Armor_pauldron_celestial_gem_{side}', (side * 21.6, -6.8, 154.5), (2, 1.1, 4.4),
        m['Emissive'])


def upper_arm_sleeve(m, side):
    segment_shell(f'Armor_upper_arm_sleeve_{side}',
                  ((side * 20.2, 0, 152), (side * 25.4, -1.5, 126)),
                  [(0, 8.3, 7.8), (.35, 8.6, 7.8), (.7, 6.5, 6.3), (1, 5.7, 5.5)], m['Blue'])
    bolt(f'Armor_sleeve_energy_line_{side}',
         [(side * 22.6, -7.4, 146), (side * 26.6, -8.2, 139),
          (side * 26.2, -7, 133), (side * 24, -6.2, 128.5)], [.4, .44, .36, .1], m['Emissive'])


def elbow_cop(m, side):
    dome_shell(f'Armor_elbow_cop_{side}', (side * 26, -.5),
               [(116.5, 5, 4.8), (119.5, 6.2, 6), (122, 5, 4.9)], 12, m['Blue'])
    ellipse(f'Armor_elbow_platina_ring_{side}', (side * 26, -.5, 118.2),
            ((5.9, 0, 0), (0, 5.9, 0)), m['Platina'], .5, 10)


def forearm_fin(m, side):
    for index in range(1):
        outline = [(side * (27 + index), 116 - index * 3), (side * (29.4 + index), 108 - index * 3),
                   (side * (30 + index), 100 - index * 3), (side * (28.4 + index), 95 - index * 3),
                   (side * (27.4 + index), 100 - index * 3), (side * (27.6 + index), 108 - index * 3)]
        mounted_outline(f'Armor_forearm_bolt_fin_{index}_{side}', outline, -5.8 - index, m, 1.2)


# ----------------------------------------------------------------- pants ----
def pants(m):
    section(SPINE_BONE, lambda: waistband(m))
    section(PELVIS_BONE, lambda: fauld(m))
    section(PELVIS_BONE, lambda: rear_facing(lambda: seat_plates(m)))
    for side in (1, -1):
        section(THIGH_BONES[side], lambda s=side: thigh_plates(m, s))
        section(THIGH_BONES[side], lambda s=side: rear_facing(lambda: rear_thigh(m, s)))
        section(CALF_BONES[side], lambda s=side: knee_cop(m, s))
    section(ROOT_BONE, lambda: faldon(m))


def waistband(m):
    """Waist line that tucks under the authored cuirass (bottom ring at z=106)."""
    loft('Pants_waistline_blue',
         [((0, 0, z), width, depth) for z, width, depth in
          ((100.8, 12.7, 9.95), (103.6, 12.2, 9.6), (106, 11.3, 8.95))], m['Blue'])
    ellipse('Pants_waistline_platina_hem', (0, 0, 100.8), ((12.9, 0, 0), (0, 10.1, 0)),
            m['Platina'], .8, 16)
    mounted_star('Pants_buckle_star', 0, 103.4, 3.8, 1.6, 8, -13.8, m, 1.3)
    gem('Pants_buckle_energy_core', (0, -14.7, 103.4), (1.1, .65, 2.2), m['Emissive'])
    for side in (-1, 1):
        bolt(f'Pants_waist_energy_line_{side}',
             [(side * 10.2, -10.9, 102.6), (side * 11.6, -7, 103.4), (side * 11.2, -2.4, 102.8),
              (side * 11.8, 2.2, 102.2), (side * 10.4, 6.4, 101.6)], [.42, .4, .42, .38, .1],
             m['Emissive'])


def fauld(m):
    """Hip bell flaring below the waistband, bound to the pelvis only."""
    loft('Pants_hip_fauld_blue',
         [((0, 0, z), width, depth) for z, width, depth in
          ((95.4, 13.6, 10.8), (98.2, 12.8, 10.2), (101, 12, 9.6))], m['Blue'])
    ellipse('Pants_fauld_platina_hem', (0, 0, 95.4), ((13.7, 0, 0), (0, 10.9, 0)),
            m['Platina'], .8, 16)
    for side in (-1, 1):
        ellipse(f'Pants_hip_platina_disc_{side}', (side * 12.9, -2.5, 98.6),
                ((0, 0, 2.4), (0, 2.4, 0)), m['Platina'], .5, 10)
        gem(f'Pants_hip_energy_pin_{side}', (side * 12.9, -5.1, 98.6), (.8, .55, 1.6),
            m['Emissive'])


def seat_plates(m):
    """Authored facing front; rear_facing() mirrors them onto the seat bell."""
    for index, (low, width, depth) in enumerate(((99.8, 5.6, -10.1), (95.8, 5.2, -10.9))):
        outline = [(-width, low + 1.4), (0, low + 2.2), (width, low + 1.4),
                   (width * .8, low - 2.4), (0, low - 3.2), (-width * .8, low - 2.4)]
        plate(f'Pants_seat_lamella_{index}', outline, (depth, 1.2), m['Blue'])
        trim(f'Pants_seat_platina_edge_{index}', outline, depth - .5, m['Platina'], .45)
    bolt('Pants_seat_energy_spine',
         [(0, -10.1, 101.8), (.7, -10.9, 99.2), (-.7, -11.1, 96.6), (0, -11.4, 94.2)],
         [.5, .42, .42, .1], m['Platina'])


def thigh_plates(m, side):
    """Angular lamellas over a blue under-sleeve, per native thigh bone."""
    axis = side * 10.45
    segment_shell(f'Pants_thigh_sleeve_blue_{side}',
                  ((axis, -1.4, 108), (axis, -3.4, 68)),
                  [(0, 7.2, 6.9), (.35, 8.2, 7.7), (.7, 7.3, 6.9), (1, 5.8, 5.6)], m['Blue'])
    for index, (low, width) in enumerate(((95.2, 8.6), (86.8, 8.2), (78.6, 7.6))):
        band = [(axis - side * 2.6, low + 3.6), (axis + side * 1.2, low + 4.8),
                (axis + side * width, low + 3), (axis + side * (width + 1), low + .4),
                (axis + side * (width * .8), low - 1.8), (axis - side * 1.6, low - 2.2)]
        curved_shell(f'Pants_thigh_lamella_{index}_{side}', band, -9.9, 2.2, m['Blue'])
        trim(f'Pants_lamella_platina_edge_{index}_{side}', band, -10.6, m['Platina'], .5)
    bolt(f'Pants_thigh_energy_line_{side}',
         [(axis + side * 2.6, -10, 104), (axis + side * 6.4, -9.6, 98),
          (axis + side * 5.2, -9, 92), (axis + side * 7.2, -8.2, 86),
          (axis + side * 5.4, -7, 80.5)], [.4, .42, .36, .42, .1], m['Emissive'])
    trim(f'Pants_thigh_star_{side}',
        [(axis + side * 5.2 + x, z) for x, z in star_outline(89.6, 3.2, 1.3, 6)],
        -10, m['Platina'], .4)
    gem(f'Pants_thigh_energy_pin_{side}', (axis + side * 5.2, -10.8, 84.6), (1.1, .7, 2.4),
        m['Emissive'])


def rear_thigh(m, side):
    """Authored facing front; rear_facing() mirrors it onto the back of the thigh."""
    axis = side * 10.45
    for index, (low, width, depth) in enumerate(((99.6, 3.2, -6.1), (92.4, 2.8, -5.8))):
        outline = [(axis - width, low + 1.2), (axis, low + 2), (axis + width, low + 1.2),
                   (axis + width * .8, low - 2.2), (axis, low - 3), (axis - width * .8, low - 2.2)]
        plate(f'Pants_thigh_rear_plate_{index}_{side}', outline, (depth, 1.1), m['Blue'])
        trim(f'Pants_thigh_rear_platina_edge_{index}_{side}', outline, depth - .5,
            m['Platina'], .4)


def knee_cop(m, side):
    """Bolt-tip cop over the boot greave top (z=57), on the native calf bone."""
    center = side * 10.35
    dome_shell(f'Pants_knee_cop_{side}', (center, -1),
               [(57.6, 4.6, 4.4), (60.6, 5.6, 5.4), (63.6, 4.4, 4.2)], 12, m['Blue'])
    outline = [(center - 3.4, 58.2), (center, 66.5), (center + 3.4, 58.2),
               (center + 2.4, 55.4), (center, 57), (center - 2.4, 55.4)]
    mounted_outline(f'Pants_knee_bolt_tip_{side}', outline, -6.8, m, 1.4)
    ellipse(f'Pants_knee_platina_ring_{side}', (center, -1, 58.4), ((5.5, 0, 0), (0, 5.3, 0)),
            m['Platina'], .5, 12)
    gem(f'Pants_knee_energy_gem_{side}', (center, -6.8, 61), (.9, .6, 1.8), m['Emissive'])


def faldon(m):
    """Front skirt in double-pointed flaps, hanging free on the native root bone.

    The audited native Duel Master pant hangs its skirt panel from Bip01 (bone
    0); it must never weld to the thigh lamellas.
    """
    for index, (x, width, low, high) in enumerate(((-7.6, 3.4, 88.5, 103), (0, 4.6, 86.5, 104.5),
                                                   (7.6, 3.4, 88.5, 103))):
        outline = [(x - width, high), (x + width, high), (x + width * .82, low + 6),
                   (x + width * .5, low + 1.2), (x + width * .22, low + 4.4),
                   (x, low), (x - width * .22, low + 4.4), (x - width * .5, low + 1.2),
                   (x - width * .82, low + 6)]
        curved_shell(f'Pants_faldon_double_point_{index}', outline, -11.6, 1.4, m['Blue'],
                     crown_rows=3, back_gap=.5)
        trim(f'Pants_faldon_platina_edge_{index}', outline, -12.4, m['Platina'], .45)
    for side in (-1, 1):
        bolt(f'Pants_faldon_energy_line_{side}',
             [(side * 4.6, -12.9, 101.6), (side * 5.8, -13.4, 98), (side * 5, -13.2, 94.6),
              (side * 6.2, -13.6, 91)], [.36, .38, .34, .1], m['Emissive'])
    trim('Pants_faldon_star_setting', star_outline(97.6, 2.8, 1.15, 8), -13.8,
        m['Platina'], .4)
    gem('Pants_faldon_energy_core', (0, -14.5, 97.6), (.9, .6, 2), m['Emissive'])


# ----------------------------------------------------------------- gloves ---
def gloves(m):
    for side in (1, -1):
        section(FOREARM_BONES[side], lambda s=side: gauntlet(m, s))
        section(FOREARM_BONES[side], lambda s=side: rear_facing(lambda: palm_guard(m, s)))
        section(HAND_BONES[side], lambda s=side: hand_guard(m, s))
        section(FINGER_BONES[side], lambda s=side: claws(m, s))


def gauntlet(m, side):
    """Flared cuff over the elbow, tapering shell, dorsal plates and crystal."""
    segment_shell(f'Glove_gauntlet_shell_blue_{side}',
                  ((side * 26.2, .2, 122.5), (side * 31.2, -.9, 89.5)),
                  [(0, 4.9, 4.6), (.3, 5.8, 5.4), (.65, 6.2, 5.8), (1, 5.2, 4.9)], m['Blue'])
    segment_shell(f'Glove_cuff_flare_blue_{side}',
                  ((side * 25.8, .3, 120.2), (side * 25, .5, 126.8)),
                  [(0, 5.3, 5), (1, 7, 6.6)], m['Blue'])
    ellipse(f'Glove_cuff_platina_rim_{side}', (side * 25, .5, 126.8), ((7.1, 0, 0), (0, 6.7, 0)),
            m['Platina'], .7, 14)
    for index, (high, low, depth) in enumerate(((117.5, 108.5, -7.2), (109.5, 100, -7.8),
                                                (101, 92, -8.2))):
        outline = [(side * 27.2, high), (side * 30.4, high - 1.4), (side * 31.9, (high + low) / 2),
                   (side * 30.2, low + 1.8), (side * 27, low)]
        plate(f'Glove_dorsal_plate_{index}_{side}', outline, (depth, 1), m['Blue'])
        bolt(f'Glove_plate_energy_chevron_{index}_{side}',
             [(side * 28.4, depth - .9, low + 2.2), (side * 29.8, depth - .9, low + 3.4),
              (side * 31, depth - .9, low + 2.2)], [.34, .36, .1], m['Platina'])
    bolt(f'Glove_outer_energy_line_{side}',
         [(side * 27.6, -5.6, 118.5), (side * 31.4, -6.3, 112), (side * 30.2, -5.8, 106),
          (side * 33, -5, 99.5), (side * 31.8, -4.2, 93.5)], [.4, .42, .36, .42, .1],
         m['Emissive'])
    trim(f'Glove_crystal_star_setting_{side}',
        [(side * 28.7 + x, z) for x, z in star_outline(104.6, 2.1, .85, 6)],
        -8.9, m['Platina'], .4)
    gem(f'Glove_dorsal_celestial_crystal_{side}', (side * 28.7, -9.6, 104.6), (1.1, .7, 2.4),
        m['Emissive'])


def palm_guard(m, side):
    """Authored facing front; rear_facing() mirrors it onto the palm side."""
    outline = [(side * 28.4, 87.2), (side * 31, 86.4), (side * 31.8, 83.4),
               (side * 30.6, 80.6), (side * 28.2, 80.2), (side * 27.2, 83.8)]
    plate(f'Glove_palm_guard_blue_{side}', outline, (-6.4, 1), m['Blue'])
    trim(f'Glove_palm_platina_border_{side}', outline, -6.9, m['Platina'], .4)


def hand_guard(m, side):
    """Segmented hand shell with a raised dorsal plate; palm stays usable."""
    center = side * 33.4
    profile = ((88.6, 4.4, 4.2), (85.5, 5, 4.7), (82.5, 4.7, 4.5), (79.8, 3.6, 3.5))
    sections = [hand_section(side, center, row) for row in profile]
    vertices = [point for points in sections for point in points]
    count = len(sections[0])
    faces = []
    for row in range(len(sections) - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    faces.append(tuple(range(count - 1, -1, -1)))
    base = (len(sections) - 1) * count
    faces.append(tuple(base + index for index in range(count)))
    result = mesh(f'Glove_hand_shell_blue_{side}', vertices, faces, m['Blue'])
    for face in result.data.polygons:
        face.use_smooth = True
    guard = [(center - 2.6, 87.2), (center, 87.8), (center + 2.6, 87.2),
             (center + 2.2, 82.8), (center, 81.2), (center - 2.2, 82.8)]
    plate(f'Glove_hand_dorsal_guard_{side}', guard, (-5.6, 1), m['Blue'])
    trim(f'Glove_guard_platina_border_{side}', guard, -6.3, m['Platina'], .45)
    star = [(center - 1.3, 85.4), (center - .4, 86.3), (center + .7, 86.1), (center + 1.3, 85.2),
            (center + .6, 84.5), (center - .5, 84.7)]
    trim(f'Glove_hand_star_setting_{side}', star, -6.1, m['Platina'], .35)
    gem(f'Glove_hand_celestial_gem_{side}', (center, -6.8, 85.4), (.9, .6, 1.7), m['Emissive'])
    bolt(f'Glove_knuckle_energy_bar_{side}',
         [(center - 3, -5, 80.6), (center - 1.4, -5.3, 80.1), (center + .2, -5.4, 80.5),
          (center + 1.8, -5.3, 80), (center + 3, -5, 80.4)], [.4, .36, .38, .36, .1],
         m['Platina'])
    ellipse(f'Glove_wrist_platina_collar_{side}', (side * 31.5, -.8, 89.4),
            ((5.5, 0, 0), (0, 5.2, 0)), m['Platina'], .7, 12)


def hand_section(side, center, row):
    z, width, depth = row
    return [(center - width * .9, -depth * .55, z), (center - width, -depth * .2, z),
            (center - width * .62, depth * .8, z), (center, depth, z),
            (center + width * .62, depth * .8, z), (center + width, -depth * .2, z),
            (center + width * .9, -depth * .55, z)]


def claws(m, side):
    """Segmented fingers ending in platinado storm-tips, on native Finger0."""
    for index, (x, y, top, length) in enumerate(
            ((side * 34.9, -2.3, 79.4, 8.2), (side * 36.9, -1.7, 78.8, 9),
             (side * 38.7, -.7, 77.6, 8.4))):
        mid = (x + side * .5, y - .3, top - length * .55)
        tip = (x + side * 1.1, y - .9, top - length)
        tube(f'Glove_finger_segment_{index}_{side}', [(x, y, top), mid], [1.05, .8],
             m['Blue'], sides=6)
        tube(f'Glove_claw_tip_platina_{index}_{side}', [mid, tip], [.6, .06], m['Platina'], sides=5)
    tube(f'Glove_thumb_segment_{side}', [(side * 32.4, -3.5, 82.6), (side * 33.6, -4.7, 78.8)],
         [1.05, .8], m['Blue'], sides=6)
    tube(f'Glove_thumb_tip_platina_{side}', [(side * 33.6, -4.7, 78.8), (side * 34.6, -5.7, 75.6)],
         [.6, .06], m['Platina'], sides=5)


# ----------------------------------------------------------------- boots ----
def boots(m):
    for side, calf, foot in ((1, 4, 5), (-1, 11, 12)):
        section(calf, lambda s=side: greave(m, s))
        section(foot, lambda s=side: sabaton(m, s))
        section(calf, lambda s=side: rear_facing(lambda: rear_calf(m, s)))


def greave(m, side):
    center = side * 10.3
    segment_shell(f'Boots_faceted_greave_{side}',
                  ((center, -.6, 15.5), (center, -4.6, 57)),
                  [(0, 4.3, 4.6), (.3, 5.4, 5.6), (.65, 7.3, 7), (1, 6.4, 6.2)], m['Blue'])
    shin = [(center, 55.5), (side * 17.6, 46), (side * 16, 31),
            (side * 12.4, 22), (side * 8.4, 24.5), (side * 7.6, 47)]
    curved_shell(f'Boots_shin_v_plate_{side}', shin, -9.6, 2.6, m['Blue'])
    trim(f'Boots_shin_platina_frame_{side}', shin, -10.3, m['Platina'], .55)
    bolt(f'Boots_shin_outer_energy_{side}',
         [(center, -10, 26), (side * 14, -9.6, 33), (side * 12.6, -9, 40),
          (side * 14.6, -8.6, 47), (side * 12, -8.4, 52)], [.42, .44, .38, .44, .1],
         m['Emissive'])
    bolt(f'Boots_shin_inner_energy_{side}',
         [(center, -10, 28), (side * 6.8, -9.6, 36), (side * 7.8, -8.8, 43),
          (side * 6.4, -8.4, 49)], [.36, .4, .36, .1], m['Emissive'])
    trim(f'Boots_shin_star_setting_{side}',
        [(center + x, z) for x, z in star_outline(45.5, 3.4, 1.35, 6)],
        -9.4, m['Platina'], .45)
    gem(f'Boots_shin_celestial_star_{side}', (center, -10.2, 45.5), (1.6, .9, 3.6),
        m['Emissive'])
    outline = [(center - 4.2, 56.8), (center, 65.6), (center + 4.2, 56.8),
               (center + 3, 53.6), (center, 55.4), (center - 3, 53.6)]
    mounted_outline(f'Boots_knee_bolt_point_{side}', outline, -9.8, m, 1.6)


def sabaton(m, side):
    center = side * 10.28
    foot_shell(f'Boots_sabaton_shell_{side}', center, m['Blue'])
    sole(m, side, center)
    for index in range(3):
        instep_scale(f'Boots_instep_lamella_{index}', m, side, center,
                     -4.5 - index * 4.6, 3.2 + index * .5, 10.6 - index * 1.2)
    toe = [(center - 2.6, 3.4), (center, 2.8), (center + 2.6, 3.4),
           (center + 2.2, 6.6), (center - 2.2, 6.6)]
    curved_shell(f'Boots_toe_guard_{side}', toe, -15.5, 1.8, m['Blue'], crown_rows=4, back_gap=.5)
    trim(f'Boots_toe_platina_edge_{side}', toe, -16, m['Platina'], .4)
    ellipse(f'Boots_ankle_platina_collar_{side}', (center, -.5, 15),
            ((4.6, 0, 0), (0, 5.6, 0)), m['Platina'], .9, 16)
    rear_facing(lambda: heel_counter(m, side, center))


def foot_shell(name, center, material):
    profile = ((9, 4.6, 1.2, 10.5), (5.5, 5.4, .9, 12.4), (3, 4.9, 1, 13.4),
               (1, 6.2, .8, 13),
               (-4, 6.4, .8, 12.4), (-9.5, 5.8, 1, 10.2), (-14.5, 3.6, 1.4, 6.6),
               (-18, 1.2, 2.2, 4))
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
    mesh(f'Boots_platina_sole_{side}', vertices, faces, m['Platina'])


def instep_scale(name, m, side, center, y, width, height):
    outline = [(center - width, y + 1.5, height - 2.2), (center, y + 2.4, height),
               (center + width, y + 1.5, height - 2.2), (center + width * .8, y - 1.8, height - 2.6),
               (center, y - 2.8, height - 1.2), (center - width * .8, y - 1.8, height - 2.6)]
    count = len(outline)
    vertices = outline + [(x, depth - .6, z) for x, depth, z in outline]
    faces = [(0, 1, 2, 3, 4, 5), (9, 8, 7, 6, 11, 10)]
    faces.extend((n, (n + 1) % count, count + (n + 1) % count, count + n) for n in range(count))
    mesh(name, vertices, faces, m['Blue'])
    chevron = [(outline[4][0], outline[4][1] + .12, outline[4][2]),
               (outline[3][0], outline[3][1] + .12, outline[3][2]),
               (outline[2][0], outline[2][1] + .12, outline[2][2])]
    bolt(name + '_platina_chevron', chevron, .35, m['Platina'])


def heel_counter(m, side, center):
    """Authored facing front; rear_facing() mirrors it onto the heel."""
    outline = [(center - 4.4, 8), (center, 10.8), (center + 4.4, 8), (center + 4.2, 1.5),
               (center + 1.6, 5.2), (center - 1.6, 5.2), (center - 4.2, 1.5), (center, .8)]
    plate(f'Boots_faceted_heel_counter_{side}', outline, (-9.6, 1.2), m['Blue'])
    trim(f'Boots_heel_platina_edge_{side}', outline, -10.1, m['Platina'], .45)


def rear_calf(m, side):
    """Authored facing front; rear_facing() mirrors it onto the calf."""
    center = side * 10.3
    for direction in (-1, 1):
        for index in range(2):
            outline = [(center + direction * 3.6, 20 + index * 9),
                       (center + direction * 6.2, 27 + index * 9),
                       (center + direction * 5.4, 35 + index * 9),
                       (center + direction * 3.2, 42 + index * 8),
                       (center + direction * 4.6, 33 + index * 9),
                       (center + direction * 4.4, 25 + index * 9)]
            plate(f'Boots_rear_calf_lamella_{side}_{direction}_{index}', outline,
                  (-6.8 - index, 1), m['Blue'])
            trim(f'Boots_rear_lamella_platina_edge_{side}_{direction}_{index}', outline,
                -7.4 - index, m['Platina'], .35)
    bolt(f'Boots_achilles_energy_spine_{side}',
         [(center, -7.2, 18), (center, -8.4, 25), (center, -8, 32), (center, -8.8, 39),
          (center, -8.2, 46), (center, -6.4, 52)], [.44, .4, .42, .4, .42, .1], m['Platina'])
    star = [(center, 52.4), (center + 1.9, 50), (center + .8, 47.6), (center, 46.2),
            (center - .8, 47.6), (center - 1.9, 50)]
    trim(f'Boots_rear_star_setting_{side}', star, -8, m['Platina'], .4)
    gem(f'Boots_rear_celestial_gem_{side}', (center, -8.6, 49.6), (1.3, .7, 2.8), m['Emissive'])
