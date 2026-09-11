"""Load the frozen Class305 equipment rigs and prove them against the audit.

The native BMDs are read from the immutable base archive into a temporary
directory, never extracted into the client. Bones, hierarchy and the bind pose
are preserved exactly; nothing native is reexported as Poseidon geometry.
"""
import hashlib
import json
import tempfile
import zipfile
from collections import defaultdict
from pathlib import Path

from import_native_reference import world_matrices
from inspect_bmd_rig import inspect

ROOT = Path(__file__).resolve().parent
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/'
               'celestial-ultimate-20260911/ultimate-base-downloaded.zip')
PIECE_MEMBERS = {'Helm': 'Data/Player/HelmClass305.bmd',
                 'Armor': 'Data/Player/ArmorClass305.bmd',
                 'Boot': 'Data/Player/BootClass305.bmd'}
AUDIT_KEYS = {'Helm': 'lord_emperor_helm', 'Armor': 'lord_emperor_armor',
              'Boot': 'lord_emperor_boots'}
USED_BONE_CONTRACT = {'Helm': (18, 20), 'Armor': (17, 18, 25, 26, 27, 34, 35, 36),
                      'Boot': (4, 5, 11, 12)}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_native_rig():
    """Return per-piece native models, bind matrices and audit agreement proof."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    models, binds = {}, {}
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='poseidon-rig-') as temp:
        for name, member in PIECE_MEMBERS.items():
            path = Path(temp) / (name + '.bmd')
            path.write_bytes(archive.read(member))
            model = inspect(path, True)
            contract = audit['models'][AUDIT_KEYS[name]]
            verify_audit_contract(name, model, contract)
            models[name] = model
            binds[name] = world_matrices(model, 0)
    skeleton = models['Helm']['bones']
    for name, model in models.items():
        if model['bones'] != skeleton:
            raise ValueError(f'{name} skeleton diverges from the audited 51-bone contract')
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                models=models, binds=binds, skeleton=skeleton,
                used_bone_contract=USED_BONE_CONTRACT)


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
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='poseidon-player-') as temp:
        path = Path(temp) / 'player.bmd'
        path.write_bytes(archive.read('Data/Player/player.bmd'))
        return inspect(path, True)
