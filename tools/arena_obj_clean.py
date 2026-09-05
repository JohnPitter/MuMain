"""Drop the corrupt object records from Data/World7/EncTerrain7.obj.

The base client's Stadium object list carries three garbage records at index
18-20 that the Season 8 reference client does not have:

    [18] type=-515 pos=(0, 0, -5.4e8) scale=0   raw fdfdfdfd...
    [19] type=0    pos=(0, 0, 0)      scale=0   (all zero)
    [20] type=0    pos=(0, 0, 0)      scale=0   (all zero)

``OpenObjectsEnc`` feeds every record straight into ``CreateObject`` without
validating the type, and object rendering resolves the model with
``BMD* b = &Models[o->Type]`` (``MODEL_WORLD_OBJECT`` is 0). A type of -515
therefore reads 515 ``BMD`` structs *before* the array - a latent out-of-bounds
read that only stays quiet while the object is culled. The two zero records
place a scale-0 Object01 at the map origin.

All three are dropped and the record count in the header is lowered
accordingly; nothing indexes map objects by ordinal (``g_iTotalObj`` is only a
count and the objects live in the 16x16 ``ObjectBlock`` grid), so removing them
is safe. The remaining 1486 records are byte-identical to what shipped.

Usage:
    py -3.12 tools/arena_obj_clean.py            # report only
    py -3.12 tools/arena_obj_clean.py --write
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

from arena_terrain_restore import map_decrypt, map_encrypt

REPO = Path(__file__).resolve().parents[1]
OBJ = REPO / "src" / "bin" / "Data" / "World7" / "EncTerrain7.obj"

RECORD_SIZE = 30
HEADER_SIZE = 4
MAX_WORLD_OBJECTS = 64


def is_valid(record: bytes) -> bool:
    (obj_type,) = struct.unpack_from("<h", record, 0)
    x, y, _z = struct.unpack_from("<fff", record, 2)
    (scale,) = struct.unpack_from("<f", record, 26)
    return (
        0 <= obj_type < MAX_WORLD_OBJECTS
        and 0.0 <= x <= 25600.0
        and 0.0 <= y <= 25600.0
        and scale > 0.0
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true", help="rewrite the file")
    args = ap.parse_args()

    raw = OBJ.read_bytes()
    data = map_decrypt(raw)
    version, map_number = data[0], data[1]
    (count,) = struct.unpack_from("<h", data, 2)

    kept = []
    dropped = []
    for i in range(count):
        record = data[HEADER_SIZE + (i * RECORD_SIZE):HEADER_SIZE + ((i + 1) * RECORD_SIZE)]
        (kept if is_valid(record) else dropped).append((i, record))

    print(f"{OBJ.name}: version={version} map={map_number} records={count}")
    for i, record in dropped:
        (obj_type,) = struct.unpack_from("<h", record, 0)
        print(f"  drop [{i}] type={obj_type} raw={record.hex()}")
    print(f"keeping {len(kept)} of {count}")

    if not dropped:
        return 0
    if not args.write:
        print("dry run - pass --write to rewrite")
        return 0

    out = bytearray(data[:2])
    out += struct.pack("<h", len(kept))
    for _, record in kept:
        out += record
    OBJ.write_bytes(map_encrypt(bytes(out)))

    check = map_decrypt(OBJ.read_bytes())
    (new_count,) = struct.unpack_from("<h", check, 2)
    assert new_count == len(kept)
    assert len(check) == HEADER_SIZE + (new_count * RECORD_SIZE)
    assert check[1] == map_number
    print(f"wrote {OBJ} ({new_count} records)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
