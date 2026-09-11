"""Load the frozen Class304 Duel Master equipment rigs and prove them against the audit.

Same discipline as the audited foundation: native BMDs are read from the
immutable base archive into a temporary directory, never extracted into the
client. The Class304 skeletons carry 51 bones and a single static action per
piece; their frame-0 bind clips are identical to the audited Class305 family,
so authored world space is shared. The used-bone contracts below were measured
from the audited native meshes (vertex bindings, not assumptions) and must
match exactly.
"""
import hashlib
import json
import tempfile
import zipfile
from collections import defaultdict
from pathlib import Path

from inspect_bmd_rig import inspect

ROOT = Path(__file__).resolve().parent
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/'
               'celestial-ultimate-20260911/ultimate-base-downloaded.zip')
PIECE_MEMBERS = {'Armor': 'Data/Player/ArmorClass304.bmd',
                 'Pant': 'Data/Player/PantClass304.bmd',
                 'Glove': 'Data/Player/GloveClass304.bmd',
                 'Boot': 'Data/Player/BootClass304.bmd'}
AUDIT_KEYS = {'Armor': 'duel_master_armor', 'Pant': 'duel_master_pants',
              'Glove': 'duel_master_gloves', 'Boot': 'duel_master_boots'}
# Measured native vertex bindings: armor dresses spine/spine1, both arm chains
# (clavicle+upper arm+forearm) and, unlike most sets, the pelvis and both
# thighs (native hip tassets); pants hang on the root, pelvis, spine, thighs
# and calves - the native Duel Master skirt follows the root (bone 0), not a
# Bone02 cloth chain; gloves use forearm, hand and Finger0 of each arm
# (Finger01/02 exist but stay unused); boots use calf and foot only.
USED_BONE_CONTRACT = {'Armor': (2, 3, 10, 17, 18, 25, 26, 27, 34, 35, 36),
                      'Pant': (0, 2, 3, 4, 10, 11, 17),
                      'Glove': (27, 28, 29, 36, 37, 38),
                      'Boot': (4, 5, 11, 12)}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_audit_contract(name, model, contract):
    bones = model['bones']
    if len(bones) != len(contract['bones']) or model['sha256'] != contract['sha256']:
        raise ValueError(f'{name}: native piece differs from the audited reference')
    for index, (bone, recorded) in enumerate(zip(bones, contract['bones'])):
        if (bone.get('name'), bone.get('parent'), bone.get('dummy', False)) != \
                (recorded.get('name'), recorded.get('parent'), recorded.get('dummy', False)):
            raise ValueError(f'{name}: bone {index} no longer matches the audited contract')
    if model['action_frames'] != contract['action_frames']:
        raise ValueError(f'{name}: action contract changed')


def load_native_rig():
    """Return per-piece native models, bind matrices and audit agreement proof."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    models, binds = {}, {}
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='zeus-rig-') as temp:
        for name, member in PIECE_MEMBERS.items():
            path = Path(temp) / (name + '.bmd')
            path.write_bytes(archive.read(member))
            model = inspect(path, True)
            verify_audit_contract(name, model, audit['models'][AUDIT_KEYS[name]])
            models[name] = model
            binds[name] = world_matrices(model, 0)
    skeleton = models['Armor']['bones']
    for name, model in models.items():
        if model['bones'] != skeleton:
            raise ValueError(f'{name} skeleton diverges from the audited 51-bone contract')
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                models=models, binds=binds, skeleton=skeleton,
                used_bone_contract=USED_BONE_CONTRACT)


def world_matrices(model, frame):
    """Bind-pose world matrices per bone (frame `frame` of the first action)."""
    from mathutils import Euler, Matrix, Vector
    matrices = []
    for index, bone in enumerate(model['bones']):
        if bone.get('dummy'):
            matrices.append(Matrix.Identity(4))
            continue
        clip = bone['clips'][0]
        local = Euler(clip['rotations'][frame], 'XYZ').to_matrix().to_4x4()
        local.translation = Vector(clip['positions'][frame])
        parent = bone['parent']
        if parent >= index:
            raise ValueError('Unsupported bone ordering')
        matrices.append(matrices[parent] @ local if parent >= 0 else local)
    return matrices


def decoded_skinned_triangles(model):
    """Bone-local corner tuples (position, normal, uv, bone) with binding checks."""
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


def load_player_rig():
    """Read the master 60-bone player skeleton from the same frozen archive."""
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='zeus-player-') as temp:
        path = Path(temp) / 'player.bmd'
        path.write_bytes(archive.read('Data/Player/player.bmd'))
        return inspect(path, True)
