"""Read equipment names and classify model filenames using the native loaders."""
import csv
import re
from pathlib import PurePosixPath


PARTS = {'Helm': 7, 'Armor': 8, 'Pants': 9, 'Gloves': 10, 'Boots': 11}
CLASSES = ('Dark Wizard', 'Dark Knight', 'Fairy Elf', 'Magic Gladiator',
           'Dark Lord', 'Summoner', 'Rage Fighter')
PART_PATTERN = re.compile(r'(Helm|Armor|Pants?|Gloves?|Boots?)', re.IGNORECASE)
BUX_KEY = bytes.fromhex('fc cf ab')
ITEM_COUNT = 16 * 512
INVENTORY_ARMORS = {'armor_inventory60': 59, 'armormale61_inventory': 60,
                    'armormale62_inventory': 61, 'armormale74_inven': 73}


def part_of(filename):
    match = PART_PATTERN.search(PurePosixPath(filename).stem)
    if not match:
        return None
    value = match[1].lower()
    return next(part for part in PARTS if part.lower().startswith(value.rstrip('s')))


def is_candidate(filename):
    stem = PurePosixPath(filename).stem.lower()
    return filename.lower().endswith('.bmd') and bool(part_of(filename) or stem.startswith(('head', 'face', 'player')))


def class_body(stem):
    match = re.search(r'class([23]?)(\d{2})$', stem, re.IGNORECASE)
    if not match:
        return {}
    index = int(match[2]) - 1
    return {'class': CLASSES[index] if 0 <= index < len(CLASSES) else 'unknown',
            'evolution_mesh': int(match[1] or '1')}


def filename_mapping(filename):
    path = PurePosixPath(filename)
    stem, part = path.stem.lower(), part_of(filename)
    result = {'part': part, 'group': PARTS.get(part), 'category': 'unmapped_equipment_candidate'}
    if 'class' in stem or stem.startswith(('head', 'face')):
        return {**result, 'category': 'class_body_or_head', **class_body(stem)}
    if stem.startswith('player'):
        return {**result, 'category': 'player_animation_reference'}
    if 'celestial' in stem and part:
        return {**result, 'category': 'equipment', 'item_number': 74, 'variant': 'primary'}
    if stem in INVENTORY_ARMORS:
        return {**result, 'category': 'equipment', 'item_number': INVENTORY_ARMORS[stem],
                'variant': 'inventory_only', 'mapping_source': 'MonkSystem.cpp:109-112; ZzzInventory.cpp:10699-10714'}
    number_match = re.search(r'(\d+)$', stem)
    if not number_match or not part:
        return result
    number = int(number_match[1])
    index, variant = None, 'primary'
    if 'luckyitem' in filename.lower() and stem.startswith('new_'):
        index = int(path.parent.name)
    elif stem.startswith('hdk_'):
        index = 28 + number
    elif stem.startswith('cw_'):
        index = 33 + number
    elif 'elfc' in stem:
        index, variant = 9 + number, 'male_elf_common'
    elif 'elf' in stem:
        index = 9 + number
    elif 'monk' in stem and 1 <= number <= 4:
        index, variant = (5, 6, 8, 9)[number - 1], 'rage_fighter_common'
    elif 'male' in stem:
        index = number - 1
        if stem.startswith('mask'):
            variant = 'helmet_face_mask'
        elif 'test' in stem:
            variant = 'primary' if number == 20 else 'test_named_unconfirmed'
        elif stem.startswith('t_'):
            variant = 'primary' if number == 19 else 't_named_unconfirmed'
    if index is None:
        return result
    return {**result, 'category': 'equipment', 'item_number': index, 'variant': variant}


def server_names(server_root):
    source = server_root / 'src/Persistence/Initialization/VersionSeasonSix/Items/Armors.cs'
    result = {}
    for line_number, line in enumerate(source.read_text(encoding='utf-8').splitlines(), 1):
        match = re.search(r'^\s*this\.Create(Armor|Gloves|Boots)\(([^;]+)\);', line)
        if not match:
            continue
        cells = next(csv.reader([match[2]], skipinitialspace=True))
        group = int(cells[1]) + 5 if match[1] == 'Armor' else PARTS[match[1]]
        name_index, level_index = (4, 8) if match[1] == 'Armor' else (1, 6)
        number = int(cells[0])
        class_start = 14 if match[1] == 'Armor' else (9 if match[1] == 'Gloves' else 12)
        levels = [int(c) for c in cells[class_start:class_start + len(CLASSES)]]
        levels += [0] * (len(CLASSES) - len(levels))
        result[f'{group}:{number}'] = dict(name=cells[name_index], minimum_level=int(cells[level_index]),
            classes=[dict(name=name, evolution_requirement=level) for name, level in zip(CLASSES, levels) if level],
            source=str(source), line=line_number)
    return result


def item_layout(header, name_length):
    widths = {'Bool': 1, 'Byte': 1, 'Word': 2, 'Int': 4, 'DWord': 4}
    fields, offset = {}, name_length
    for name, kind in re.findall(r'\bX\((\w+), (Bool|Byte|Word|Int|DWord), 1,', header):
        width = widths[kind]
        offset = (offset + width - 1) // width * width
        fields[name] = (offset, width)
        offset += width
    fields['RequireClass'] = (offset, len(CLASSES))
    return fields, (offset + len(CLASSES) + 8 + 3) // 4 * 4


def client_names(raw, client_root):
    header = (client_root / 'src/source/Data/GameData/ItemData/ItemFieldDefs.h').read_text()
    formats = [(size, *item_layout(header, size)) for size in (30, 50)]
    name_length, fields, stride = next((entry for entry in formats if len(raw) == ITEM_COUNT * entry[2] + 4), (None, None, None))
    if stride is None:
        raise ValueError(f'Unsupported Item.bmd byte length: {len(raw)}')
    result = {}
    for group in range(16):
        for number in range(512):
            start = (group * 512 + number) * stride
            record = bytes(value ^ BUX_KEY[index % len(BUX_KEY)] for index, value in enumerate(raw[start:start + stride]))
            name_bytes = record[:name_length].split(b'\0', 1)[0]
            try:
                name = name_bytes.decode('utf-8')
                encoding = 'utf-8'
            except UnicodeDecodeError:
                name = name_bytes.decode('cp932', errors='replace')
                encoding = 'cp932-fallback'
            if not name:
                continue
            offset, width = fields['RequireLevel']
            class_offset, class_count = fields['RequireClass']
            levels = record[class_offset:class_offset + class_count]
            result[f'{group}:{number}'] = dict(name=name, name_encoding=encoding, name_bytes_hex=name_bytes.hex(), minimum_level=int.from_bytes(record[offset:offset + width], 'little'),
                classes=[dict(name=cls, evolution_requirement=level) for cls, level in zip(CLASSES, levels) if level])
    return result


def runtime_primary_paths():
    result = set()
    stems = {'Helm': 'Helm', 'Armor': 'Armor', 'Pants': 'Pant', 'Gloves': 'Glove', 'Boots': 'Boot'}
    for number in range(75):
        if 54 <= number <= 58:
            continue
        for part, stem in stems.items():
            if part == 'Helm' and number in (15, 20, 23, 32, 37, 47, 48, 71):
                continue
            if part == 'Gloves' and number in (59, 60, 61, 72, 73):
                continue
            prefix, suffix, directory = '', f'Male{number + 1:02}', 'Data/Player'
            if 10 <= number <= 14:
                suffix = f'Elf{number - 9:02}'
            elif number == 19:
                suffix = 'MaleTest20'
            elif number == 18 and part == 'Pants':
                prefix = 't_'
            elif 29 <= number <= 38:
                prefix, suffix = ('HDK_', f'Male{number - 28:02}') if number <= 33 else ('CW_', f'Male{number - 33:02}')
            elif 62 <= number <= 72:
                directory, prefix, suffix = f'Data/Player/LuckyItem/{number}', 'new_', f'{number - 61:02}'
            elif number == 74:
                prefix, stem, suffix = 'Celestial_', part, ''
            result.add(f'{directory}/{prefix}{stem}{suffix}.bmd'.lower())
    return result


def enrich_mapping(mapping, names):
    if 'item_number' not in mapping:
        return mapping
    identity = f"{mapping['group']}:{mapping['item_number']}"
    result = {**mapping, 'identity': identity, 'server': names['server'].get(identity),
              'client_localizations': {locale: entries[identity] for locale, entries in names['client'].items() if identity in entries}}
    local = result['client_localizations'].get('Eng') or result['client_localizations'].get('default') or {}
    result['name'] = (result['server'] or local).get('name', 'Name unresolved')
    result['name_reliability'] = 'server_declared' if result['server'] else ('client_cp932_inferred' if local.get('name_encoding') == 'cp932-fallback' else 'client_declared')
    if '\ufffd' in result['name'] or result['name'] == 'Name unresolved':
        result['name_reliability'] = 'unresolved_or_lossy_decode'
    result['defined_on_server'] = result['server'] is not None
    if mapping['item_number'] == 74:
        result['effective_override'] = {'category': 'Ultimate', 'minimum_level': 400, 'class': 'Grand Master',
            'source': 'CelestialUltimateConfiguration + client Ultimate requirements; original initializer/class bytes are preserved above.'}
    return result
