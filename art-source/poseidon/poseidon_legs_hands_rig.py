"""Load the frozen Pant/Glove Class305 rigs and prove them against the audit.

Same discipline as poseidon_armor_rig.py: native BMDs are read from the
immutable base archive into a temporary directory, never extracted into the
client. The used-bone contracts below were measured from the audited native
meshes (vertex bindings, not assumptions) and must match exactly.
"""
import hashlib
import json
import tempfile
import zipfile
from pathlib import Path

from import_native_reference import world_matrices
from inspect_bmd_rig import inspect
from poseidon_armor_rig import digest, verify_audit_contract

ROOT = Path(__file__).resolve().parent
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/'
               'celestial-ultimate-20260911/ultimate-base-downloaded.zip')
PIECE_MEMBERS = {'Pant': 'Data/Player/PantClass305.bmd',
                 'Glove': 'Data/Player/GloveClass305.bmd'}
AUDIT_KEYS = {'Pant': 'lord_emperor_pants', 'Glove': 'lord_emperor_gloves'}
# Measured native vertex bindings: Pant hangs on pelvis, both thighs/calves,
# spine (waistband) and Bone02 (free front skirt); gloves use forearm, hand
# and Finger0 of each arm - Finger01/02 exist in the skeleton but stay unused.
USED_BONE_CONTRACT = {'Pant': (2, 3, 4, 10, 11, 17, 44),
                      'Glove': (27, 28, 29, 36, 37, 38)}


def load_native_rig():
    """Return per-piece native models, bind matrices and audit agreement proof."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    models, binds = {}, {}
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='poseidon-legs-') as temp:
        for name, member in PIECE_MEMBERS.items():
            path = Path(temp) / (name + '.bmd')
            path.write_bytes(archive.read(member))
            model = inspect(path, True)
            contract = audit['models'][AUDIT_KEYS[name]]
            verify_audit_contract(name, model, contract)
            models[name] = model
            binds[name] = world_matrices(model, 0)
    skeleton = models['Pant']['bones']
    for name, model in models.items():
        if model['bones'] != skeleton:
            raise ValueError(f'{name} skeleton diverges from the audited 51-bone contract')
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                models=models, binds=binds, skeleton=skeleton,
                used_bone_contract=USED_BONE_CONTRACT)
