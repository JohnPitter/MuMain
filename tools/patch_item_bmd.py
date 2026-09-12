"""Write the authored-set rows into a client item table (``item*.bmd``).

The client keeps one fixed-size record per item index (``group * 512 + number``)
in ``Data/Local/**/item*.bmd``. Every Celestial row is filled in; the Poseidon
and Zeus rows shipped blank, which means ``Width`` and ``Height`` are zero - and
an item with a zero footprint cannot be placed in any storage, so the client
refuses to even pick it up. Nothing about that reaches the server, which is why
the logs showed no rejection at all.

    python tools/patch_item_bmd.py --spec tools/authored_item_rows.json \
        --verify Data/Local/Por/item_por.bmd
    python tools/patch_item_bmd.py --spec tools/authored_item_rows.json \
        --write Data/Local/Por/item_por.bmd

The record layout mirrors ``ITEM_ATTRIBUTE_FILE_LEGACY`` (ItemStructs.h plus the
X-macro in ItemFieldDefs.h) as MSVC lays it out: 30-byte name, then the simple
fields with their natural alignment, then ``RequireClass[7]`` and
``Resistance[8]`` - 84 bytes. Encryption is the per-record 3-byte Bux xor
(``_crypt.h``) and the trailing DWORD is ``GenerateCheckSum2`` over the
*encrypted* buffer with key 0xE2F1 (ZzzInfomation.h, ItemDataLoader.cpp).

Never touches an installed client: it rewrites the file given on the command
line, and only the indices named in the spec.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

BUX_CODE = (0xFC, 0xCF, 0xAB)
RECORD_SIZE = 84
ITEM_TYPES = 16
ITEM_INDEX = 512
MAX_ITEM = ITEM_TYPES * ITEM_INDEX
CHECKSUM_KEY = 0xE2F1
NAME_SIZE = 30
MAX_CLASS = 7

# field -> (offset, struct format). Offsets follow MSVC's padding of the
# legacy record; the file size is the proof they are right.
FIELDS = {
    'two_hand': (30, '<B'),
    'level': (32, '<H'),
    'slot': (34, '<B'),
    'skill': (36, '<H'),
    'width': (38, '<B'),
    'height': (39, '<B'),
    'damage_min': (40, '<B'),
    'damage_max': (41, '<B'),
    'blocking': (42, '<B'),
    'defense': (43, '<B'),
    'magic_defense': (44, '<B'),
    'weapon_speed': (45, '<B'),
    'walk_speed': (46, '<B'),
    'durability': (47, '<B'),
    'magic_durability': (48, '<B'),
    'magic_power': (49, '<B'),
    'require_strength': (50, '<H'),
    'require_dexterity': (52, '<H'),
    'require_energy': (54, '<H'),
    'require_vitality': (56, '<H'),
    'require_charisma': (58, '<H'),
    'require_level': (60, '<H'),
    'value': (62, '<B'),
    'zen': (64, '<i'),
    'attack_type': (68, '<B'),
}
REQUIRE_CLASS_OFFSET = 69
RESISTANCE_OFFSET = 76


def bux(record: bytearray) -> bytearray:
    """The xor is its own inverse and restarts at every record boundary."""
    return bytearray(b ^ BUX_CODE[i % 3] for i, b in enumerate(record))


def checksum(buffer: bytes, key: int = CHECKSUM_KEY) -> int:
    result = (key << 9) & 0xFFFFFFFF
    for offset in range(0, len(buffer) - 3, 4):
        (word,) = struct.unpack_from('<I', buffer, offset)
        if (offset // 4 + key) % 2 == 0:
            result ^= word
        else:
            result = (result + word) & 0xFFFFFFFF
        if offset % 16 == 0:
            result ^= ((key + result) & 0xFFFFFFFF) >> ((offset // 4) % 8 + 1)
        result &= 0xFFFFFFFF
    return result


def read(path: Path):
    raw = path.read_bytes()
    expected = RECORD_SIZE * MAX_ITEM + 4
    if len(raw) != expected:
        raise SystemExit(f'{path}: {len(raw)} bytes, expected {expected} '
                         f'({RECORD_SIZE}-byte records x {MAX_ITEM} + checksum)')
    body, stored = raw[:-4], struct.unpack('<I', raw[-4:])[0]
    if checksum(body) != stored:
        raise SystemExit(f'{path}: checksum {stored:#010x} does not match '
                         f'{checksum(body):#010x} - wrong key or corrupt file')
    return bytearray(body)


def record(body: bytearray, index: int) -> bytearray:
    return bux(body[index * RECORD_SIZE:(index + 1) * RECORD_SIZE])


def describe(plain: bytearray) -> dict:
    out = {'name': plain[:NAME_SIZE].split(b'\x00')[0].decode('latin-1')}
    for field, (offset, fmt) in FIELDS.items():
        out[field] = struct.unpack_from(fmt, plain, offset)[0]
    out['require_class'] = list(plain[REQUIRE_CLASS_OFFSET:REQUIRE_CLASS_OFFSET + MAX_CLASS])
    return out


def build(row: dict) -> bytearray:
    plain = bytearray(RECORD_SIZE)
    name = row['name'].encode('latin-1')
    if len(name) >= NAME_SIZE:
        raise SystemExit(f'name too long for a {NAME_SIZE}-byte field: {row["name"]!r}')
    plain[:len(name)] = name
    for field, (offset, fmt) in FIELDS.items():
        struct.pack_into(fmt, plain, offset, row.get(field, 0))
    index = row['require_class_index']
    if not 0 <= index < MAX_CLASS:
        raise SystemExit(f'require_class_index {index} out of range for {row["name"]!r}')
    plain[REQUIRE_CLASS_OFFSET + index] = row['require_class_step']
    return plain


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--spec', type=Path, required=True)
    parser.add_argument('--write', type=Path)
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    if bool(args.write) == bool(args.verify):
        raise SystemExit('pass exactly one of --write or --verify')

    rows = json.loads(args.spec.read_text(encoding='utf-8'))
    path = args.write or args.verify
    body = read(path)

    if args.verify:
        missing = 0
        for row in rows:
            index = row['group'] * ITEM_INDEX + row['number']
            have = describe(record(body, index))
            want = build(row)
            ok = have == describe(want)
            missing += 0 if ok else 1
            print(f'{"ok " if ok else "BAD"} {index:5} {row["name"]:26} '
                  f'W={have["width"]} H={have["height"]} slot={have["slot"]} '
                  f'lvl={have["require_level"]} class={have["require_class"]}')
        print(f'{len(rows) - missing}/{len(rows)} rows match the spec')
        return 0 if missing == 0 else 1

    for row in rows:
        index = row['group'] * ITEM_INDEX + row['number']
        body[index * RECORD_SIZE:(index + 1) * RECORD_SIZE] = bux(build(row))
    out = bytes(body) + struct.pack('<I', checksum(bytes(body)))
    path.write_bytes(out)
    print(f'{path}: {len(rows)} rows written, {len(out)} bytes')
    return 0


if __name__ == '__main__':
    sys.exit(main())
