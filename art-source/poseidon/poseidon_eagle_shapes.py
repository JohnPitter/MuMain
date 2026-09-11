"""Original Imperial Eagle silhouette on the audited DarkSpirit pet rig.

Design-spec.md "Águia Imperial": layered black feathers with gold edges,
ocean-blue breast crystal and gaze, real talons - on the DarkSpirit pet rig
"separado do personagem" (77 bones / 4 actions kept intact). Shapes are
authored in the native bind-pose world space (Z-up, frame 0 of the frozen
darkspirit.bmd skeleton, muzzle facing -Y, wings spread along +-X) with
single full influences.

Bone-contract decisions (see poseidon_eagle_rig.py): the native feather
distribution is mirrored per wing; the animated-but-native-unused crest
bones 7/8/9 and tail tip 76 follow the effect-bone pattern (wide clip
swings, no native mesh) so authored geometry stays off them - crest feathers
bind to the head and the tail fan to the native tail pair 74/75. The neck
(5), spine (3) and leg chains never animate and are safe, like the native
pelvis; the forearms 12/39 carry the real wing joint.
"""
from mathutils import Vector

from armor_surfaces import section
from celestial_geometry import ellipse, feather, gem, rim, tube
from poseidon_mount_shapes import sweep

BODY_BONES = (2, 3, 4)
COLLAR_BONE = 5
HEAD_BONE = 6
WING_ARMS = {1: (10, 11, 12), -1: (37, 38, 39)}
WING_MIRROR = {41: 14, 42: 15, 45: 18, 46: 19, 49: 22, 53: 26, 57: 30,
               61: 34, 62: 35, 63: 36}
# L-side feather row spec: bone -> (start_x, reach_x, base_z, base_y, count).
# Extents follow the native per-bone mesh spans (audit + measured bind data).
WING_ROWS = {
    14: (58, 96, 71, -16, 3), 15: (92, 150, 74, -19, 3),
    18: (58, 94, 66, -14, 2), 19: (90, 144, 69, -17, 2),
    22: (56, 100, 58, -12, 2), 26: (54, 90, 51, -7, 2),
    30: (40, 72, 46, -10, 2),
    34: (24, 46, 40, -14, 2), 35: (22, 42, 36, -4, 2), 36: (20, 38, 31, 8, 2),
}
GOLD_COVERTS = (34, 35, 36)
TAIL_BONES = (74, 75)
LEG_BONES = {1: (64, 65, 66, 67), -1: (69, 70, 71, 72)}


def y_ring(y, x=0, z=0, width=0, depth=0):
    return (Vector((x, y, z)), Vector((width, 0, 0)), Vector((0, 0, depth)))


# ------------------------------------------------------------------ eagle ----
def eagle(m):
    body(m)
    head(m)
    for side in (1, -1):
        wing(m, side)
        leg(m, side)
    tail(m)


def body(m):
    """Black feathered body with a gold breast edge and the ocean crystal."""
    section(BODY_BONES[0], lambda: sweep('Eagle_body_rump_black',
            [y_ring(30, z=22, width=6.5, depth=8.5), y_ring(20, z=24.5, width=9, depth=11.5),
             y_ring(8, z=25.5, width=10.5, depth=13)], m['Black'], segments=16))
    section(BODY_BONES[1], lambda: sweep('Eagle_body_mid_black',
            [y_ring(8, z=25.5, width=10.5, depth=13), y_ring(-6, z=26, width=11, depth=13.5)],
            m['Black'], segments=16))
    section(BODY_BONES[2], lambda: chest(m))


def chest(m):
    sweep('Eagle_body_chest_black',
          [y_ring(-6, z=26, width=11, depth=13.5), y_ring(-18, z=24.5, width=10, depth=12.5),
           y_ring(-28, z=22.5, width=7, depth=9.5)], m['Black'], segments=16)
    rim('Eagle_chest_gold_edge', [(-7.5, 28), (0, 31), (7.5, 28), (5.5, 16), (0, 13.5), (-5.5, 16)],
        -26.5, m['Gold'], .55)
    gem('Eagle_breast_ocean_crystal', (0, -30, 21), (2.1, 1.3, 4), m['Blue'])


def collar(m):
    """Gold collar ring at the native neck bone."""
    ellipse('Eagle_collar_gold_ring', (0, -24, 24.6), ((7.6, 0, 0), (0, 0, 7.2)),
            m['Gold'], .7, 16)


def head(m):
    """Black eagle head, gold hooked beak, blue gaze and crest feathers."""
    section(COLLAR_BONE, lambda: collar(m))
    section(HEAD_BONE, lambda: sweep('Eagle_head_black',
            [(Vector((0, -28, 27)), Vector((6.5, 0, 0)), Vector((0, 0, 6.5))),
             (Vector((0, -34, 25.5)), Vector((5.5, 0, 0)), Vector((0, 0, 5.5))),
             (Vector((0, -38.5, 24)), Vector((4, 0, 0)), Vector((0, 0, 4)))],
            m['Black'], segments=14))
    section(HEAD_BONE, lambda: beak(m))
    for index, (offset, lean) in enumerate(((0, 0), (1.6, .9), (-1.6, .9))):
        controls = [(offset, -38.5, 25), (offset + lean, -39.5, 28.5),
                    (offset + lean * 1.4, -38, 31.5), (offset + lean, -35.5, 33.5)]
        section(HEAD_BONE, lambda c=controls, i=index:
                feather(f'Eagle_crest_feather_{i}', c, (1.6, .5, 8), m['Black']))
    for side in (1, -1):
        section(HEAD_BONE, lambda s=side: eye(m, s))
        section(HEAD_BONE, lambda s=side: cheek_tufts(m, s))


def beak(m):
    tube('Eagle_beak_gold', [(0, -39.5, 23.5), (0, -44, 22.5), (0, -47.5, 21.5)],
         [2.1, 1.3, .35], m['Gold'], sides=10)
    tube('Eagle_beak_hook_gold', [(0, -47.5, 21.5), (0, -48.6, 20.2)], [.5, .1],
         m['Gold'], sides=6)


def eye(m, side):
    gem(f'Eagle_eye_ocean_gem_{side}', (side * 3.3, -33.5, 26.6), (.9, .6, .9), m['Blue'])


def cheek_tufts(m, side):
    for index, lift in enumerate((0, 2.4)):
        feather(f'Eagle_cheek_tuft_{index}_{side}',
                [(side * 4.4, -37, 25.5 + lift), (side * 7.5, -31, 27.5 + lift),
                 (side * 10.5, -25, 28.5 + lift), (side * 13, -20, 29 + lift)],
                (1.9, .5, 8), m['Black'])


def wing(m, side):
    """Layered black wing with gold coverts on the native finger chains."""
    arm = WING_ARMS[side]
    section(arm[0], lambda s=side: tube(f'Eagle_wing_arm_{s}',
            [(s * 8, -22, 31), (s * 24, -22, 37)], [5.5, 4.8], m['Black'], sides=12))
    section(arm[1], lambda s=side: tube(f'Eagle_wing_upper_{s}',
            [(s * 24, -22, 37), (s * 45, -21, 52)], [4.8, 4.2], m['Black'], sides=12))
    section(arm[2], lambda s=side: wing_forearm(m, s))
    for bone in WING_ROWS:
        native = bone if side == 1 else next(k for k, v in WING_MIRROR.items() if v == bone)
        section(native, lambda b=native, s=side: wing_rows(m, b, s))


def wing_forearm(m, side):
    tube(f'Eagle_wing_fore_{side}', [(side * 45, -21, 52), (side * 57, -20, 67)],
         [4.2, 3.4], m['Black'], sides=12)
    ellipse(f'Eagle_wrist_gold_ring_{side}', (side * 52, -20.5, 61),
            ((4.4, 0, 0), (0, 4, 0)), m['Gold'], .5, 12)


def wing_rows(m, bone, side):
    """One feather row; spec keys are L-side bones, mirrored for the right."""
    spec_bone = WING_MIRROR.get(bone, bone)
    start, reach, z0, y0, count = WING_ROWS[spec_bone]
    gold = spec_bone in GOLD_COVERTS
    material = m['Gold'] if gold else m['Black']
    for index in range(count):
        controls = [(side * start, y0 - index * 2.4, z0 + index * .8),
                    (side * (start + (reach - start) * .45), y0 - 2 - index * 2.4,
                     z0 - .8 + index * .6),
                    (side * (start + (reach - start) * .82), y0 - 4 - index * 2.4,
                     z0 - 1.6 + index * .4),
                    (side * reach, y0 - 6 - index * 2.4, z0 - 2.6)]
        width = 4.6 if reach > 100 else 3.6 if reach > 60 else 2.6
        feather(f'Eagle_wing_feather_{bone}_{index}', controls,
                (width, .5, 8), material)


def leg(m, side):
    """Feathered thigh, gold ring and real talons on the native leg chain."""
    thigh, calf, foot, toe = LEG_BONES[side]
    section(thigh, lambda s=side: tube(f'Eagle_leg_thigh_{s}',
            [(s * 3.5, 8, 20), (s * 4, 0, 17)], [3.6, 2.8], m['Black'], sides=10))
    section(calf, lambda s=side: ellipse(f'Eagle_leg_gold_ring_{s}',
            (s * 4.2, -4, 16.4), ((2.7, 0, 0), (0, 0, 2.7)), m['Gold'], .5, 10))
    section(foot, lambda s=side: tube(f'Eagle_leg_foot_{s}',
            [(s * 4.4, -2, 14.6), (s * 4.8, -4.5, 13.6)], [2.2, 1.9], m['Gold'], sides=8))
    section(toe, lambda s=side: talons(m, s))


def talons(m, side):
    for index, spread in enumerate((-1.6, 0, 1.6)):
        tube(f'Eagle_talon_{index}_{side}',
             [(side * 4.8, -4.5 + spread * .4, 13.4),
              (side * (5.6 + index * .7), -8.5 + spread * 1.6, 11),
              (side * (6 + index), -11 + spread * 2.4, 9.2)],
             [.75, .45, .08], m['Gold'], sides=6)


def tail(m):
    """Black tail fan with a gold vane pair on the native tail chain."""
    fans = (
        (TAIL_BONES[0], [(0, 24, 28), (0, 42, 30), (0, 56, 31), (0, 66, 31.2)], 5.5),
        (TAIL_BONES[1], [(0, 54, 31), (0, 78, 31.5), (0, 98, 31), (0, 112, 30)], 4.8),
    )
    for index, (bone, controls, width) in enumerate(fans):
        section(bone, lambda c=controls, w=width, i=index:
                feather(f'Eagle_tail_center_{i}', c, (w, .6, 10), m['Black']))
    for side in (1, -1):
        section(TAIL_BONES[1], lambda s=side: tail_tip(m, s))


def tail_tip(m, side):
    """Long tail feathers; the +-14 degree pair is gold-edged."""
    for index, spread in enumerate((0, 14, 26)):
        controls = [(side * spread * .25, 40, 29.5),
                    (side * spread * .8, 66, 30 - spread * .02),
                    (side * spread * 1.5, 92, 29.5 - spread * .05),
                    (side * spread * 2, 112, 28.5 - spread * .08)]
        material = m['Gold'] if spread == 14 else m['Black']
        feather(f'Eagle_tail_vane_{index}_{side}', controls, (3, .5, 9), material)
