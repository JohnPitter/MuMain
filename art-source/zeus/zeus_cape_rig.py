"""Load the frozen cape rig and reconstruct the runtime link math.

The native cape (Data/Item/DarkLordRobe02.bmd, audited as `emperor_cape`) is
the cloth precedent of the client: one bone ("collar", index 0) and one pose;
the visible fabric is procedural cloth created by ZzzCharacter.cpp (see
cape-cloth-contract.md). The rigid ferragem of every cape in that family is
linked to player bone 19: RenderLinkObject composes

    M1 = AngleMatrix(0, 90, 0) + T(-47, -7, 0)      # cape branch (~line 6845)
    M2 = AngleMatrix(145, 0, 275) + T(0, 10, -30)   # non-right-hand concat
    ParentMatrix = BoneTransform[19] . M1 . M2      # R_ConcatTransforms

and BMD::Animation appends the model's own root-bone matrix. This module
rebuilds that product from the frozen player.bmd and the audited cape, so
authored geometry in player bind space can be exported into cape-bone-local
space and later posed exactly where the runtime would put it. The Zeus cape
reuses this audited link unchanged (same slot, same branch); its class
difference lives in the cloth gate, not in the ferragem link. No native mesh
is reexported and no runtime code changes here.
"""
import hashlib
import json
import tempfile
import zipfile
from pathlib import Path

from import_native_reference import world_matrices
from inspect_bmd_rig import inspect

ROOT = Path(__file__).resolve().parent
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/'
               'celestial-ultimate-20260911/ultimate-base-downloaded.zip')
CAPE_MEMBER = 'Data/Item/DarkLordRobe02.bmd'
PLAYER_MEMBER = 'Data/Player/player.bmd'
AUDIT_KEY = 'emperor_cape'
CAPE_BONE_CONTRACT = (0,)
LINK_BONE = 19


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def angle_matrix(roll, pitch, yaw):
    """Engine AngleMatrix (ZzzMathLib.cpp): degrees, rows of Rz(yaw)Ry(pitch)Rx(roll)."""
    from math import cos, radians, sin
    sr, cr = sin(radians(roll)), cos(radians(roll))
    sp, cp = sin(radians(pitch)), cos(radians(pitch))
    sy, cy = sin(radians(yaw)), cos(radians(yaw))
    from mathutils import Matrix
    return Matrix((
        (cp * cy, sr * sp * cy - cr * sy, cr * sp * cy + sr * sy, 0.0),
        (cp * sy, sr * sp * sy + cr * cy, cr * sp * sy - sr * cy, 0.0),
        (-sp, sr * cp, cr * cp, 0.0),
        (0.0, 0.0, 0.0, 1.0)))


def link_matrix():
    """M1 . M2 exactly as RenderLinkObject builds it for MODEL_CAPE_OF_EMPEROR."""
    from mathutils import Matrix
    m1 = angle_matrix(0, 90, 0)
    m1[0][3], m1[1][3], m1[2][3] = -47.0, -7.0, 0.0
    m2 = angle_matrix(145, 0, 275)
    m2[0][3], m2[1][3], m2[2][3] = 0.0, 10.0, -30.0
    return m1 @ m2


def verify_audit_contract(model, contract):
    if model['sha256'] != contract['sha256']:
        raise ValueError('native cape differs from the audited reference')
    bones = model['bones']
    if len(bones) != len(contract['bones']):
        raise ValueError('cape bone count diverges from the audited contract')
    for index, (bone, recorded) in enumerate(zip(bones, contract['bones'])):
        if (bone.get('name'), bone.get('parent'), bone.get('dummy', False)) != \
                (recorded.get('name'), recorded.get('parent'), recorded.get('dummy', False)):
            raise ValueError(f'cape bone {index} no longer matches the audited contract')
    if model['action_frames'] != contract['action_frames'] or \
            model['action_locks'] != contract['action_locks']:
        raise ValueError('cape pose contract changed')
    if len(model['meshes']) != contract['meshes'] or \
            sum(m['triangles'] for m in model['meshes']) != contract['triangles']:
        raise ValueError('cape mesh census changed')


def load_cape_rig():
    """Return the audited native cape, its bind (runtime link) matrix and proof."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    contract = audit['models'][AUDIT_KEY]
    if contract['member'] != CAPE_MEMBER:
        raise ValueError('audit member moved')
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='zeus-cape-') as temp:
        cape_path = Path(temp) / 'cape.bmd'
        cape_path.write_bytes(archive.read(CAPE_MEMBER))
        native = inspect(cape_path, True)
        player_path = Path(temp) / 'player.bmd'
        player_path.write_bytes(archive.read(PLAYER_MEMBER))
        player = inspect(player_path, True)
    verify_audit_contract(native, contract)
    bone = native['bones'][0]
    if bone['parent'] != -1 or bone.get('dummy', False):
        raise ValueError('cape root bone must be a real root')
    clip = bone['clips'][0]
    from mathutils import Euler, Matrix, Vector
    bone0 = Euler(clip['rotations'][0], 'XYZ').to_matrix().to_4x4()
    bone0.translation = Vector(clip['positions'][0])
    bind_world = world_matrices(player, 0, 0)
    cape_root = bind_world[LINK_BONE] @ link_matrix() @ bone0
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                native=native, player_bones=len(player['bones']),
                player_anchor=dict(bone=LINK_BONE,
                                   translation=[round(v, 4) for v in bind_world[LINK_BONE].translation]),
                audit=audit, contract=contract,
                link=dict(bone=LINK_BONE,
                          m1_rotation=(0, 90, 0), m1_translation=(-47, -7, 0),
                          m2_rotation=(145, 0, 275), m2_translation=(0, 10, -30),
                          source='ZzzCharacter.cpp RenderLinkObject MODEL_CAPE_OF_EMPEROR branch'),
                bone0=dict(name=bone['name'], parent=bone['parent'],
                           position=clip['positions'][0], rotation=clip['rotations'][0]),
                cape_root=cape_root,
                cape_root_translation=[round(v, 4) for v in cape_root.translation])


def cape_bind(rig):
    """Single-influence bind map: authored world space -> cape bone-local space."""
    return {0: rig['cape_root']}


def decoded_skinned_triangles(model):
    """Bone-local corner tuples (position, normal, uv, bone) with binding checks."""
    from collections import defaultdict
    grouped = defaultdict(list)
    for mesh in model['meshes']:
        for (vertices, uv_ids), normals in zip(mesh['faces'], mesh['normal_indices']):
            corners = []
            for vertex, normal, uv in zip(vertices, normals, uv_ids):
                bone, position = mesh['points'][vertex]
                record = mesh['normal_vectors'][normal]
                if record[0] != bone or record[-1] != vertex:
                    raise ValueError('Normal bound to the wrong bone or vertex')
                corners.append((position, record[1:4], mesh['texcoords'][uv], (bone,)))
            grouped[mesh['texture']].append(corners)
    return grouped
