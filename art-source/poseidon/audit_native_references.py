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

REFERENCE = Path('C:/Users/joaop/AppData/Local/Temp/codex-clipboard-04794f6e-f53a-4555-a521-d0a673490353.png')
ARCHIVE = Path('C:/Users/joaop/Desenvolvimento/openmu/scratchpad/celestial-ultimate-20260911/ultimate-base-downloaded.zip')
MEMBERS = {
    'owner_rig': 'Data/Player/player.bmd',
    'lord_emperor_helm': 'Data/Player/HelmClass305.bmd',
    'lord_emperor_armor': 'Data/Player/ArmorClass305.bmd',
    'lord_emperor_pants': 'Data/Player/PantClass305.bmd',
    'lord_emperor_gloves': 'Data/Player/GloveClass305.bmd',
    'lord_emperor_boots': 'Data/Player/BootClass305.bmd',
    'emperor_cape': 'Data/Item/DarkLordRobe02.bmd',
    'dark_horse': 'Data/Skill/DarkHorse.bmd',
    'dark_spirit': 'Data/Skill/darkspirit.bmd',
    'native_scepter_orientation_only': 'Data/Item/saint.bmd',
}
SOURCE_EVIDENCE = {
    'src/source/Core/Globals/_enum.h': ['CLASS_DARK_LORD,', 'CLASS_LORDEMPEROR,',
                                     'LordEmperor = 17', 'MODEL_CAPE_OF_EMPEROR =',
                                     'PLAYER_DARKLORD_STAND', 'PLAYER_ATTACK_DARKHORSE'],
    'src/source/Engine/Object/ZzzOpenData.cpp': ['L"Player")', 'L"HelmClass3"',
        'L"DarkLordRobe02"', 'L"DarkHorse")', 'L"DarkSpirit")', 'L"Saint")'],
    'src/source/Engine/Object/ZzzCharacter.cpp': ['c->Weapon[0].LinkBone = 33;',
        'c->Weapon[1].LinkBone = 42;', 'w->LinkBone = 19;', 'MODEL_CAPE_OF_EMPEROR)',
        'CreateMount(MODEL_DARK_HORSE', 'giPetManager::CreatePetDarkSpirit'],
    'src/source/GameLogic/Pets/GIPetManager.cpp': ['new CSPetDarkSpirit'],
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
            raise ValueError(f'Source contract moved: {filename}')
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
    with zipfile.ZipFile(args.archive) as archive, tempfile.TemporaryDirectory(prefix='poseidon-native-') as temp:
        models = {name: model_summary(archive, name, member, Path(temp)) for name, member in MEMBERS.items()}
    report = dict(archive=str(args.archive), archive_sha256=sha256(args.archive),
                  concept_reference=str(REFERENCE), concept_sha256=sha256(REFERENCE),
                  purpose='Rig/animation/source compatibility evidence, not shape reuse or new gameplay approval',
                  sources=source_evidence(), models=models)
    (ROOT / 'native-reference-audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    for name, model in models.items():
        print(name, model['member'], 'bones', len(model['bones']), 'actions', len(model['action_frames']),
              'meshes', model['meshes'], 'sha256', model['sha256'])


if __name__ == '__main__':
    main()
