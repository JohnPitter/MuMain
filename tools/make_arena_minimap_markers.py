"""Arena (World7) TAB-map markers - the green NPC dots the other maps show.

`CNewUIMiniMap::LoadImages` reads `Data\\Local\\<ML>\\Minimap\\Minimap_<World>_<ML>.bmd`
alongside the sheet and draws one icon per record: `Kind` 1 is the NPC dot
(`mini_map_ui_npc`, 15 px) and `Kind` 2 the portal arrow (`mini_map_ui_portal`,
30 px, oriented by `Rotation`). Hovering a marker shows `Name`. World7 has never
had one - the retail client ships no Arena minimap at all - so the Stadium's
cage doors, its warp pad and its plaza were unlabelled.

File layout (matching every shipped Minimap_*.bmd, 11 649 bytes):
  100 x MINI_MAP_FILE (116 B: BYTE Kind + 3 pad, int Location[2], int Rotation,
  char Name[100] UTF-8), + 45 trailing bytes, all XOR'd with BuxConvert's
  {0xFC, 0xCF, 0xAB}, then a 4-byte GenerateCheckSum2(buffer, size, 0x2BC1).
  A record with Kind 0 ends the list.

Coordinates are (map X, map Y) - `Location[0]` is X, verified against
Lorencia's shipped file (the Guardsman at 96,129 is the server's spawn at
x=96 y=129).

Positions come from OpenMU `src/GameLogic/ArenaCageDoors.cs` (the 13 cage
`Gates` and the plaza safezone rect) and `VersionSeasonSix/Gates.cs` (exit gate
50, the warp landing); the monster pairs come from
`Version075/Maps/Arena.cs` `HuntingSpawnSpecs`. Re-run after changing any of
those.

Run with `py -3.12 tools/make_arena_minimap_markers.py`.
"""
from __future__ import annotations

import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCAL = ROOT / "src" / "bin" / "Data" / "Local"

RECORD_SIZE = 116
RECORD_COUNT = 100
TRAILER_SIZE = 45
CHECKSUM_KEY = 0x2BC1
BUX_CODE = (0xFC, 0xCF, 0xAB)
NAME_SIZE = 100

KIND_NPC = 1
KIND_PORTAL = 2

# (kind, x, y, rotation, {language: name})
MARKERS = [
    (KIND_PORTAL, 102, 116, 45, {
        "eng": "Arena Warp Pad",
        "por": "Plataforma de teleporte",
        "spn": "Plataforma de teletransporte"}),
    (KIND_NPC, 66, 45, 0, {
        "eng": "Plaza - Safe Zone",
        "por": "Praca - Zona Segura",
        "spn": "Plaza - Zona Segura"}),
    (KIND_NPC, 16, 38, 0, {"eng": "Cage: Yeti / Ice Monster",
                           "por": "Jaula: Yeti / Ice Monster",
                           "spn": "Jaula: Yeti / Ice Monster"}),
    (KIND_NPC, 16, 56, 0, {"eng": "Cage: Elite Yeti / Hommerd",
                           "por": "Jaula: Elite Yeti / Hommerd",
                           "spn": "Jaula: Elite Yeti / Hommerd"}),
    (KIND_NPC, 16, 74, 0, {"eng": "Cage: Cyclops / Hell Spider",
                           "por": "Jaula: Cyclops / Hell Spider",
                           "spn": "Jaula: Cyclops / Hell Spider"}),
    (KIND_NPC, 16, 87, 0, {"eng": "Cage: Poison Bull / Dark Knight",
                           "por": "Jaula: Poison Bull / Dark Knight",
                           "spn": "Jaula: Poison Bull / Dark Knight"}),
    (KIND_NPC, 35, 38, 0, {"eng": "Cage: Gorgon / Thunder Lich",
                           "por": "Jaula: Gorgon / Thunder Lich",
                           "spn": "Jaula: Gorgon / Thunder Lich"}),
    (KIND_NPC, 35, 56, 0, {"eng": "Cage: Shadow / Cursed Wizard",
                           "por": "Jaula: Shadow / Cursed Wizard",
                           "spn": "Jaula: Shadow / Cursed Wizard"}),
    (KIND_NPC, 35, 74, 0, {"eng": "Cage: Devil / Poison Shadow",
                           "por": "Jaula: Devil / Poison Shadow",
                           "spn": "Jaula: Devil / Poison Shadow"}),
    (KIND_NPC, 29, 86, 0, {"eng": "Cage: Death Cow / Death Knight",
                           "por": "Jaula: Death Cow / Death Knight",
                           "spn": "Jaula: Death Cow / Death Knight"}),
    (KIND_NPC, 53, 38, 0, {"eng": "Cage: Bahamut / Vepar",
                           "por": "Jaula: Bahamut / Vepar",
                           "spn": "Jaula: Bahamut / Vepar"}),
    (KIND_NPC, 53, 56, 0, {"eng": "Cage: Lizard King / Hydra",
                           "por": "Jaula: Lizard King / Hydra",
                           "spn": "Jaula: Lizard King / Hydra"}),
    # One door serves both the Iron Wheel and the Mutant pen (gates 410/411).
    (KIND_NPC, 53, 81, 0, {"eng": "Cages: Iron Wheel / Mutant",
                           "por": "Jaulas: Iron Wheel / Mutant",
                           "spn": "Jaulas: Iron Wheel / Mutant"}),
    (KIND_NPC, 64, 74, 0, {"eng": "Cage: Alquamos / Drakan",
                           "por": "Jaula: Alquamos / Drakan",
                           "spn": "Jaula: Alquamos / Drakan"}),
]


def generate_checksum(buffer: bytes, key: int) -> int:
    """ZzzInfomation.h GenerateCheckSum2."""
    result = (key << 9) & 0xFFFFFFFF
    for offset in range(0, len(buffer) - 3, 4):
        (word,) = struct.unpack_from("<I", buffer, offset)
        if ((offset // 4 + key) % 2) == 0:
            result ^= word
        else:
            result = (result + word) & 0xFFFFFFFF
        if offset % 16 == 0:
            result ^= ((key + result) & 0xFFFFFFFF) >> ((offset // 4) % 8 + 1)
        result &= 0xFFFFFFFF
    return result


def bux_convert(buffer: bytearray, start: int, size: int) -> None:
    """_crypt.h BuxConvert, in place.

    The loader calls it once per record with the pointer advanced, so the key
    phase restarts at every record - and 116 is not a multiple of 3, so writing
    one continuous pass over the whole buffer decodes as garbage from record 1
    onwards.
    """
    for i in range(size):
        buffer[start + i] ^= BUX_CODE[i % 3]


def build(language: str) -> bytes:
    body = bytearray(RECORD_SIZE * RECORD_COUNT + TRAILER_SIZE)
    for index, (kind, x, y, rotation, names) in enumerate(MARKERS):
        name = names[language].encode("utf-8")
        if len(name) >= NAME_SIZE:
            raise SystemExit(f"name too long: {names[language]!r}")
        offset = index * RECORD_SIZE
        struct.pack_into("<B3xiii", body, offset, kind, x, y, rotation)
        body[offset + 16:offset + 16 + len(name)] = name
    for index in range(RECORD_COUNT):
        bux_convert(body, index * RECORD_SIZE, RECORD_SIZE)
    return bytes(body) + struct.pack("<I", generate_checksum(body, CHECKSUM_KEY))


def main() -> None:
    for folder, language in (("Eng", "eng"), ("Por", "por"), ("Spn", "spn")):
        directory = LOCAL / folder / "Minimap"
        if not directory.is_dir():
            raise SystemExit(f"missing {directory}")
        blob = build(language)
        path = directory / f"Minimap_World7_{language}.bmd"
        path.write_bytes(blob)
        print(f"wrote {path} ({len(blob)} bytes, {len(MARKERS)} markers)")


if __name__ == "__main__":
    main()
