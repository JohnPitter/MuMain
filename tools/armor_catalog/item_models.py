"""Resolve non-armor equipment from literal native loads, then known loader loops."""
import re
from pathlib import PurePosixPath


GROUPS = {'SWORD': 0, 'AXE': 1, 'MACE': 2, 'SPEAR': 3, 'BOW': 4, 'STAFF': 5,
          'SHIELD': 6, 'HELM': 7, 'ARMOR': 8, 'PANTS': 9, 'GLOVES': 10,
          'BOOTS': 11, 'WING': 12, 'HELPER': 13, 'POTION': 14, 'ETC': 15}
LOADER_FILES = ('src/source/Engine/Object/ZzzOpenData.cpp',
                'src/source/GameLogic/Social/MonkSystem.cpp',
                'src/source/GameLogic/Items/ChangeRingManager.cpp')


def model_symbols(client_root):
    definitions = (client_root / 'src/source/Core/Globals/_enum.h').read_text(encoding='utf-8')
    symbols = {'MODEL_' + group: (number, 0) for group, number in GROUPS.items()}
    expression = re.compile(r'\b(MODEL_\w+)\s*=\s*(MODEL_\w+)\s*\+\s*(\d+)')
    for _ in range(3):
        for name, parent, offset in expression.findall(definitions):
            if parent in symbols:
                group, number = symbols[parent]
                symbols[name] = group, number + int(offset)
    monk = (client_root / LOADER_FILES[1]).read_text(encoding='utf-8')
    for parent, left, right in re.findall(r'SetModelType\((MODEL_\w+), (MODEL_\w+), (MODEL_\w+)\)', monk):
        if parent in symbols:
            symbols[left] = symbols[right] = symbols[parent]
    return symbols


def literal_item_loads(client_root):
    symbols, result = model_symbols(client_root), {}
    for relative in LOADER_FILES:
        parse_loader(client_root / relative, symbols, result)
    return result


def parse_loader(source, symbols, result):
    pattern = re.compile(r'AccessModel\((MODEL_\w+)(?:\s*\+\s*(\d+))?,\s*(?:L"([^"]+)"|directory),\s*L"([^"]+)"(?:,\s*(-?\d+))?\)')
    text = source.read_text(encoding='utf-8')
    textures = {}
    for symbol, directory in re.findall(r'OpenTexture\((MODEL_\w+),\s*L"([^"]+)"', text):
        textures.setdefault(symbol, set()).add('Data/' + directory.replace('\\', '/').replace('//', '/'))
    for line_number, line in enumerate(text.splitlines(), 1):
        match = pattern.search(line)
        if not match or match[1] not in symbols:
            continue
        group, number = symbols[match[1]]
        folder = (match[3] or 'Data/Item/').replace('\\', '/').replace('//', '/')
        suffix = f'{int(match[5]):02}' if match[5] and int(match[5]) >= 0 else ''
        filename = (folder + match[4] + suffix + '.bmd').lower()
        result.setdefault(filename, []).append(dict(group=group, item_number=number + int(match[2] or '0'),
            model_symbol=match[1], source=str(source), line=line_number, mapping_evidence='literal_native_load',
            texture_directories=sorted(textures.get(match[1], []))))


def loop_identity(stem):
    match = re.fullmatch(r'(sword|axe|mace|spear|bow|crossbow|staff|shield|wing|ring|necklace)_?(\d+)', stem)
    if not match:
        return None
    prefix, number = match[1], int(match[2])
    if prefix == 'wing':
        if 1 <= number <= 7:
            return 12, number - 1
        if 8 <= number <= 11:
            return 12, number + 28
        if 42 <= number <= 44:
            return 12, number - 1
        return None
    if prefix == 'crossbow':
        return (4, number + 7) if number <= 7 else None
    if prefix in ('ring', 'necklace'):
        return (13, number + (7 if prefix == 'ring' else 11)) if number in (1, 2) else None
    allowed = {'sword': set(range(1, 18)), 'axe': set(range(1, 10)), 'mace': set(range(1, 14)),
               'spear': set(range(1, 11)), 'bow': set(range(1, 8)), 'staff': set(range(1, 10)) | set(range(15, 22)),
               'shield': set(range(1, 16))}
    return (GROUPS[prefix.upper()], number - 1) if number in allowed.get(prefix, set()) else None


def equipment_category(group, name):
    if group <= 5:
        return 'weapon'
    if group == 6:
        return 'shield'
    if 7 <= group <= 11:
        return 'armor_attachment'
    if group == 12 and any(word in name.lower() for word in ('wing', 'cape', 'robe')):
        return 'wings_or_cape'
    if group == 13 and any(word in name.lower() for word in ('ring', 'pendant', 'necklace')):
        return 'accessory'
    if group == 13:
        return 'helper_pet_or_other'
    return 'consumable_quest_or_other'


def item_mapping(relative, loads, names):
    identities = loads.get(relative.lower(), [])
    stem = PurePosixPath(relative).stem.lower()
    if not identities:
        identity = loop_identity(stem)
        if identity:
            identities = [dict(group=identity[0], item_number=identity[1], mapping_evidence='native_prefix_loop')]
    if not identities:
        return dict(category='unmapped_item_resource', part=None, identities=[], name='Name unresolved')
    enriched = []
    for item in identities:
        identity = f"{item['group']}:{item['item_number']}"
        local = {locale: records[identity] for locale, records in names['client'].items() if identity in records}
        name = (local.get('Eng') or local.get('default') or {}).get('name', 'Name unresolved')
        record = local.get('Eng') or local.get('default') or {}
        reliability = 'client_cp932_inferred' if record.get('name_encoding') == 'cp932-fallback' else 'client_declared'
        if '\ufffd' in name or name == 'Name unresolved':
            reliability = 'unresolved_or_lossy_decode'
        enriched.append({**item, 'identity': identity, 'name': name, 'name_reliability': reliability, 'client_localizations': local})
    primary = enriched[0]
    category = equipment_category(primary['group'], primary['name'] + ' ' + stem)
    return {**primary, 'category': category, 'part': None, 'identities': enriched}
