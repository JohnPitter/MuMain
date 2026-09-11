"""Load the frozen DarkHorse mount rig and prove it against the audit.

Same discipline as poseidon_armor_rig.py: the native BMD is read from the
immutable base archive into a temporary directory, never extracted into the
client. The authored black mount binds to this 60-bone / 7-action skeleton
with single full influences; the used-bone contract below extends the native
distribution only on animated bones the horse already carries (ear bones
25/26/27 for the head fins, Toe0 48/54 for the metal hooves).
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
MEMBER = 'Data/Skill/DarkHorse.bmd'
AUDIT_KEY = 'dark_horse'
MODEL_NAME = 'Poseidon_Black_Mount'
# Exactly the native mesh distribution (see native-reference-audit.json)
# plus the animated Toe0 hooves 48/54 for the metal shoes. The ear/chin bones
# 25/26/27 are native effect anchors (no native mesh binds to them and their
# clips swing widely), so authored geometry stays off them - no renumbering.
USED_BONE_CONTRACT = (17, 18, 19, 20, 21, 22, 23, 28,
                      31, 32, 33, 34, 37, 38, 39, 40, 42,
                      44, 45, 46, 47, 48, 50, 51, 52, 53, 54,
                      56, 57, 58)
# Evidence: src/source/Engine/AI/GOBoid.cpp, MODEL_DARK_HORSE branch
# (PLAYER_ATTACK_DARKHORSE -> 3, PLAYER_RUN_RIDE_HORSE -> 1,
# PLAYER_ATTACK_RIDE_STRIKE..RIDE_ATTACK_MAGIC -> 2, PLAYER_IDLE1_DARKHORSE
# -> 5, PLAYER_IDLE2_DARKHORSE -> 6, default -> 0). Action 4 is not
# referenced by the mount branch and stays reserved.
ACTION_NAMES = {0: 'stand', 1: 'gallop', 2: 'rider_strike', 3: 'earthshake',
                4: 'reserved', 5: 'idle1', 6: 'idle2'}
ACTION_EVIDENCE = ('src/source/Engine/AI/GOBoid.cpp MODEL_DARK_HORSE branch: '
                   'PLAYER_ATTACK_DARKHORSE->3, PLAYER_RUN_RIDE_HORSE->1, '
                   'PLAYER_ATTACK_RIDE_STRIKE..RIDE_ATTACK_MAGIC->2, '
                   'PLAYER_IDLE1_DARKHORSE->5, PLAYER_IDLE2_DARKHORSE->6, default->0; '
                   'action 4 is not referenced by the mount branch.')


def load_native_rig():
    """Return the native mount model, its bind matrices and audit agreement."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='poseidon-mount-') as temp:
        path = Path(temp) / 'DarkHorse.bmd'
        path.write_bytes(archive.read(MEMBER))
        model = inspect(path, True)
        verify_audit_contract('DarkHorse', model, audit['models'][AUDIT_KEY])
    # The contract may only use real bones: dummies carry no clips, while
    # non-animated bones still pose correctly through their animated parents
    # (the native mesh itself binds to such bones, e.g. the eagle's pelvis).
    dummies = [bone for bone in USED_BONE_CONTRACT if model['bones'][bone].get('dummy')]
    if dummies:
        raise ValueError(f'Mount contract uses dummy bones: {dummies}')
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                model=model, binds=world_matrices(model, 0),
                used_bone_contract=USED_BONE_CONTRACT,
                action_names=ACTION_NAMES, action_evidence=ACTION_EVIDENCE)
