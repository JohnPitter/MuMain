"""Load the frozen Wing44 wing rig and prove it against the audit.

Same discipline as the armor lane: the native wing BMD is read from the
immutable base archive into a temporary directory, never extracted into the
client. The contract is exact - 47 bones with the audited names, indices and
hierarchy (no renumbering, no extra halo bone), one 9-frame flap action with
locked root positions. Geometry authored on this rig must use the frame-0
bind matrices computed with the root XY lock applied, exactly like the
runtime does for locked actions.
"""
import hashlib
import json
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/'
               'celestial-ultimate-20260911/ultimate-base-downloaded.zip')
WING_MEMBER = 'Data/Item/Wing44.bmd'
WING_AUDIT_KEY = 'wing_skeleton_reference'
# Bones the authored Zeus wings bind to: the root ornament rides the static
# root, each side fans over its five-bone arm chain (mirrored pairs 1-5 and
# 24-28), mirroring how the authored Celestial wings ride the same chains.
FLAP_CHAINS = {1: (1, 2, 3, 4, 5), -1: (24, 25, 26, 27, 28)}
USED_BONES = (0, 1, 2, 3, 4, 5, 24, 25, 26, 27, 28)
FLAP_FRAMES = 9


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_wing_contract(model, contract):
    bones = model['bones']
    if model['sha256'] != contract['sha256']:
        raise ValueError('Native Wing44 differs from the audited reference')
    if len(bones) != 47 or model['action_frames'] != [FLAP_FRAMES] \
            or model['action_locks'] != [True]:
        raise ValueError('Wing44 animation contract changed')
    for index, (bone, recorded) in enumerate(zip(bones, contract['bones'])):
        if (bone.get('name'), bone.get('parent'), bone.get('dummy', False)) != \
                (recorded.get('name'), recorded.get('parent'), recorded.get('dummy', False)):
            raise ValueError(f'Wing44 bone {index} no longer matches the audit')


def load_wing_rig():
    """Return the Wing44 model plus lock-aware frame-0 bind matrices."""
    audit = json.loads((ROOT / 'native-reference-audit.json').read_text(encoding='utf-8'))
    if audit['archive_sha256'] != hashlib.sha256(ARCHIVE.read_bytes()).hexdigest():
        raise ValueError('Frozen base archive changed; audit no longer applies')
    from import_native_reference import world_matrices
    with zipfile.ZipFile(ARCHIVE) as archive, tempfile.TemporaryDirectory(prefix='zeus-wing-') as temp:
        path = Path(temp) / 'Wing44.bmd'
        path.write_bytes(archive.read(WING_MEMBER))
        model = inspect_full(path)
    verify_wing_contract(model, audit['models'][WING_AUDIT_KEY])
    return dict(archive=str(ARCHIVE), archive_sha256=digest(ARCHIVE),
                model=model, bind=world_matrices(model, 0),
                used_bones=USED_BONES, flap_frames=FLAP_FRAMES)


def inspect_full(path):
    from inspect_bmd_rig import inspect
    return inspect(path, True)


def decoded_wing_triangles(model):
    """Bone-local corner tuples (position, normal, uv, bone) with binding checks."""
    from zeus_armor_rig import decoded_skinned_triangles
    return decoded_skinned_triangles(model)
