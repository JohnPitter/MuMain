"""Read exact native references and record evidence; never modify/extract game assets in place."""
import argparse
import hashlib
import json
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent.parent
sys.path.insert(0, str(ROOT.parent / 'celestial'))
from inspect_bmd_rig import inspect

REFERENCE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/concept-herdeiro-de-zeus-mg-20260911.png')
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/celestial-ultimate-20260911/ultimate-base-downloaded.zip')
# Duel Master = CLASS_DUELMASTER, whose GetBaseClass() is CLASS_DARK (index 3 in
# CLASS_TYPE). OpenPlayers() loads Class3 sets with suffix i + 1, so the Duel
# Master visual set is the Class3XX family with suffix 304.
MEMBERS = {
    'owner_rig': 'Data/Player/player.bmd',
    'duel_master_helm': 'Data/Player/HelmClass304.bmd',
    'duel_master_armor': 'Data/Player/ArmorClass304.bmd',
    'duel_master_pants': 'Data/Player/PantClass304.bmd',
    'duel_master_gloves': 'Data/Player/GloveClass304.bmd',
    'duel_master_boots': 'Data/Player/BootClass304.bmd',
    'native_sword_orientation_only': 'Data/Item/Sword15.bmd',
    'native_staff_orientation_only': 'Data/Item/Staff06.bmd',
    'wing_skeleton_reference': 'Data/Item/Wing44.bmd',
    'native_pendant_reference': 'Data/Item/Necklace02.bmd',
    'native_ring_reference': 'Data/Item/Ring02.bmd',
    # Cape of the Emperor: the audited cloth precedent (one "collar" bone,
    # rigid ferragem only; fabric is procedural cloth in ZzzCharacter.cpp).
    'emperor_cape': 'Data/Item/DarkLordRobe02.bmd',
}
SOURCE_EVIDENCE = {
    'src/source/Core/Globals/_enum.h': ['CLASS_DUELMASTER,', 'SKIN_CLASS_DUELMASTER,',
        'DuelMaster = 13,', 'MODEL_LEGENDARY_STAFF = MODEL_STAFF + 5,',
        'MODEL_LIGHTING_SWORD = MODEL_SWORD + 14,', 'MODEL_15GRADE_ARMOR_OBJ_HEAD,'],
    'src/source/Core/Globals/_define.h': ['#define MAX_CLASS', '#define MAX_CLASS_STAGES'],
    'src/source/Character/CharacterManager.cpp': ['case CLASS_DUELMASTER:', 'return CLASS_DARK;',
        'case DuelMaster:', 'case MagicGladiator:', 'IsThirdClass'],
    'src/source/Engine/Object/ZzzOpenData.cpp': ['L"Player")', 'L"HelmClass3"',
        '(MAX_CLASS * 2) + i'],
    'src/source/Engine/Object/ZzzCharacter.cpp': ['c->Weapon[0].LinkBone = 33;',
        'c->Weapon[1].LinkBone = 42;', 'case CLASS_DARK:    c->Object.Scale = 0.95f; break;',
        'GetBaseClass(c->Class) == CLASS_DARK)'],
    'src/source/Engine/Object/ZzzInventory.cpp': ['DualMaster',
        'GetBaseClass(Hero->Class) == CLASS_KNIGHT || gCharacterManager.GetBaseClass(Hero->Class) == CLASS_DARK'],
    'src/source/Engine/Object/ZzzObject.cpp': ['case 3:Vector(Bright * 0.0f, Bright * 0.5f, Bright * 1.0f',
        'MODEL_LEGENDARY_STAFF)', 'TIER_FULL_SPECULAR_V2 = {',
        'vec3_t specularTint', 'MODEL_15GRADE_ARMOR_OBJ_HEAD;',
        'Luminosity, Luminosity * 0.3f, 1.f - Luminosity'],
    'src/source/Render/Shaders/BMDMeshShader.cpp': ['SHADER_VARIANT_FULL_SPECULAR_V2',
        'u_SpecularTint'],
}


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def source_evidence():
    report = {}
    for filename, needles in SOURCE_EVIDENCE.items():
        path = REPO / filename
        lines = path.read_text(encoding='utf-8-sig').splitlines()
        hits = {needle: [dict(line=i + 1, text=line.strip()) for i, line in enumerate(lines)
                         if needle in line] for needle in needles}
        if any(not items for items in hits.values()):
            raise ValueError(f'Source contract moved: {filename} needle {needles}')
        report[filename] = dict(sha256=sha256(path), matches=hits)
    return report


def model_summary(archive, name, member, directory):
    path = directory / (name + '.bmd')
    path.write_bytes(archive.read(member))
    model = inspect(path, True)
    bones = [dict(index=i, name=bone.get('name'), parent=bone.get('parent'),
                  dummy=bone.get('dummy', False),
                  animated_actions=[index for index, clip in enumerate(bone.get('clips', []))
                                    if clip['moves'] or clip['rotates']])
             for i, bone in enumerate(model['bones'])]
    return dict(member=member, sha256=model['sha256'], model_name=model['name'],
                meshes=len(model['meshes']), triangles=sum(m['triangles'] for m in model['meshes']),
                textures=sorted({m['texture'] for m in model['meshes']}), bones=bones,
                action_frames=model['action_frames'], action_locks=model['action_locks'],
                used_bones=sorted({n for m in model['meshes'] for n in m['used_bones']}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=ARCHIVE)
    args = parser.parse_args()
    with zipfile.ZipFile(args.archive) as archive, tempfile.TemporaryDirectory(prefix='zeus-native-') as temp:
        models = {name: model_summary(archive, name, member, Path(temp)) for name, member in MEMBERS.items()}
    report = dict(archive=str(args.archive), archive_sha256=sha256(args.archive),
                  concept_reference=str(REFERENCE), concept_sha256=sha256(REFERENCE),
                  white_platina_masters=dict(
                      directory='C:/_wt-platina-client/art-source/celestial/textures/masters/white-platina',
                      Gold_png_sha256=sha256(Path('C:/_wt-platina-client/art-source/celestial/textures/masters/white-platina/Gold.png')),
                      Ivory_png_sha256=sha256(Path('C:/_wt-platina-client/art-source/celestial/textures/masters/white-platina/Ivory.png'))),
                  purpose='Rig/animation/source compatibility evidence, not shape reuse or new gameplay approval',
                  sources=source_evidence(), models=models)
    (ROOT / 'native-reference-audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    for name, model in models.items():
        print(name, model['member'], 'bones', len(model['bones']), 'actions', len(model['action_frames']),
              'meshes', model['meshes'], 'triangles', model['triangles'], 'sha256', model['sha256'])


if __name__ == '__main__':
    main()
