"""Load the frozen DarkSpirit eagle rig and prove it against the audit.

Contract decision (design-spec.md, "Águia Imperial" row): the eagle follows the
animated DarkSpirit pet rig - "Rig DarkSpirit separado do personagem; voo,
retorno e ataque em prova futura" - not a shoulder ornament. The audit table
records Data/Skill/darkspirit.bmd as the animated eagle reference (77 bones /
4 actions); DarkHorseHorn/DarkHorseSoul style item shells are explicitly not
the pet contract. The authored Imperial Eagle binds to this skeleton with
single full influences on the animated pet bones only.
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
MEMBER = 'Data/Skill/darkspirit.bmd'
AUDIT_KEY = 'dark_spirit'
MODEL_NAME = 'Poseidon_Imperial_Eagle'
# Symmetric pet contract: body 2/3/4, collar on the static neck 5, head 6;
# wings arm 10/11/12 (L) / 37/38/39 (R) with layered feather rows on the
# finger chains 14/15, 18/19, 22, 26, 30 + coverts 34/35/36 (L) and mirrors
# 41/42, 45/46, 49, 53, 57 + 61/62/63 (R); legs 64-67 / 69-72; tail 74/75.
# The native-unused but animated crest 7/8/9 and tail tip 76 follow the
# effect-bone pattern (wide clip swings, no native mesh) and stay unused,
# exactly like the native mesh distribution treats them.
USED_BONE_CONTRACT = (2, 3, 4, 5, 6,
                      10, 11, 12, 14, 15, 18, 19, 22, 26, 30, 34, 35, 36,
                      37, 38, 39, 41, 42, 45, 46, 49, 53, 57, 61, 62, 63,
                      64, 65, 66, 67, 69, 70, 71, 72,
                      74, 75)
# Evidence: src/source/GameLogic/Pets/CSPetSystem.cpp CSPetAction branch -
# PET_FLY -> 0, PET_FLYING/PET_STAND_START -> 1, PET_STAND -> 2 (the locked
# static stand, matching action_locks[2]), PET_ESCAPE -> 3.
ACTION_NAMES = {0: 'fly', 1: 'flying', 2: 'stand', 3: 'escape'}
ACTION_EVIDENCE = ('src/source/GameLogic/Pets/CSPetSystem.cpp: PET_FLY->0, '
                   'PET_FLYING/PET_STAND_START->1, PET_STAND->2 (locked), '
                   'PET_ESCAPE->3.')


def load_native_rig():
    """Return the native eagle model, its bind matrices and audit agreement."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='poseidon-eagle-') as temp:
        path = Path(temp) / 'darkspirit.bmd'
        path.write_bytes(archive.read(MEMBER))
        model = inspect(path, True)
        verify_audit_contract('darkspirit', model, audit['models'][AUDIT_KEY])
    # The contract may only use real bones: dummies carry no clips, while
    # non-animated bones (the eagle's pelvis and legs) still pose correctly
    # through their animated parents - the native mesh binds to them as well.
    dummies = [bone for bone in USED_BONE_CONTRACT if model['bones'][bone].get('dummy')]
    if dummies:
        raise ValueError(f'Eagle contract uses dummy bones: {dummies}')
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                model=model, binds=world_matrices(model, 0),
                used_bone_contract=USED_BONE_CONTRACT,
                action_names=ACTION_NAMES, action_evidence=ACTION_EVIDENCE)
