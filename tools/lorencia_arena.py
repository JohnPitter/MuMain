"""Replace the Lorencia (World1, map 0) central fountain with a PvP arena base.

The owner's request: put an arena "where the Crywolf altar/wolf statue base
sits" in the middle of Lorencia, on top of the fountain -- WITHOUT the wolf
statue and WITHOUT the green teleport pillars, for PvP and events.

What this does, precisely
--------------------------
1. Removes the fountain (MODEL_WATERSPOUT, EncTerrain1.obj record type 105)
   at tile (141,128) plus the handful of small decorative records (flower/
   fence tufts) sitting directly inside the new platform's footprint --
   everything else in the plaza (lamps, benches, houses, NPC paths) is left
   untouched.
2. Adds one new EncTerrain1.obj record at the same position/tile, using a
   *wolf-free* Crywolf model: ``Data/Object35/Object57.bmd`` ("cr_stonewolf_
   ba1" -- CryWolf STONEWOLF BAse #1, texture ``cr_stonewolf_ba.OZJ``). This
   is a genuinely separate mesh from the wolf statue (``Object82.bmd`` /
   texture ``cr_stonewolf.OZJ``, loaded as a different NPC model,
   MODEL_CRYWOLF_STATUE) -- confirmed by reading both BMD headers (distinct
   mesh, distinct name, distinct texture) rather than guessing from the
   in-game silhouette. Object57 is loaded by the *official* client only as
   an invisible MODEL_CRYWOLF_ALTAR1..5 contract-state marker
   (``Object.Visible = false`` in ZzzCharacter.cpp) -- Webzen never renders
   it -- but the mesh itself is real, static, and wolf-free, which is
   exactly the "base without the statue" the request asks for. It is flat
   (native footprint ~131x132 units, ~5.4 units tall) rather than a tall
   multi-tier dais; see the report for why a taller cut of the combined
   statue+base mesh (Object82) was investigated and NOT shipped.
3. Copies ``Object35/Object57.bmd`` and its texture ``Object35/
   cr_stonewolf_ba.OZJ`` into ``Data/Object1/`` unchanged (byte-identical),
   because World1 (Lorencia) does NOT use the generic per-world
   "ObjectNN.bmd" loader that every other town uses -- ``CMapManager::Load``
   hard-codes a named model list for WorldActive==0 (Tree/Grass/.../Beer),
   see src/source/World/MapInfra/MapManager.cpp:1027-1099. Slot 49 (the gap
   between MODEL_TOMB03=46 and MODEL_FIRE_LIGHT01=50) is not used by that
   list nor referenced by any EncTerrain1.obj record, so a one-line
   ``AccessModel`` call was added there (see the accompanying C++ diff) --
   this is the "implement minimal code only if the loader cannot do it"
   fallback the task calls for; the client's own loader has no generic path
   for World1.
4. Clears TW_SAFEZONE from a circle covering the new platform's footprint in
   EncTerrain1.att and STAMPS TW_SAFEZONE onto the surrounding plaza rect
   (only on tiles that are plain-walkable, value 0 -- matching
   GameMapTerrain.SafezoneMap's ``value == 1`` exact-byte check, so NOMOVE
   tiles such as walls are left alone).

   IMPORTANT, evidence-based correction to the brief: the *official* Lorencia
   EncTerrain1.att does not flag the fountain plaza as TW_SAFEZONE at all
   (measured: zero safezone tiles in x 133..151 / y 118..138 before this
   tool runs). So "clear the platform, keep the rest as safezone" is not a
   clear-then-restore -- there was nothing to restore. This tool instead
   ADDS safezone to the plaza ring for the first time, so the platform reads
   as carved OUT of a safe town rather than the town never having been safe.
   See tools/../CHANGELOG.md and the session report for the full reasoning,
   including why this alone does not yet enable non-duel PvP server-side
   (PvpRules.IsPvpByDesignMap needs a map-scoped check; not implemented in
   this pass -- /duel already works anywhere, including on the platform).

Usage:
    py -3.12 tools/lorencia_arena.py                 # report only (dry run)
    py -3.12 tools/lorencia_arena.py --write          # write the 3 files + copy assets
    py -3.12 tools/lorencia_arena.py --verify         # dump the resulting records/tiles

Idempotent: running --write twice produces byte-identical output the second
time (the tool detects the arena record/model files already being in place
and no-ops instead of double-inserting).
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DATA = REPO / "src" / "bin" / "Data"
WORLD1 = DATA / "World1"
OBJECT1 = DATA / "Object1"
OBJECT35 = DATA / "Object35"

OBJ_PATH = WORLD1 / "EncTerrain1.obj"
ATT_PATH = WORLD1 / "EncTerrain1.att"

SOURCE_BMD = OBJECT35 / "Object57.bmd"
SOURCE_TEX = OBJECT35 / "cr_stonewolf_ba.OZJ"
DEST_BMD = OBJECT1 / "ArenaAltar.bmd"
# Data/Object1/cr_stonewolf_ba.ozj (lowercase) already ships with Lorencia -- verified
# byte-identical to the Object35 source (sha256 370444af...386da on both). MU data
# packages duplicate shared textures into every world folder that might reference
# them; this one already made it into Object1 even though nothing used it there
# before this tool. No new texture file is needed -- this constant exists only so
# --write can assert the two stay identical, in case a future authoring pass changes
# the source texture and someone forgets the (untracked-by-this-tool) copy in Object1.
DEST_TEX = OBJECT1 / "cr_stonewolf_ba.ozj"

MAP_KEY = bytes([0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
                 0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2])
BUX = bytes([0xFC, 0xCF, 0xAB])

MAP_INDEX = 1  # World1 == Lorencia == server map 0
TW_SAFEZONE = 0x01
TW_NOMOVE = 0x04
TW_NOGROUND = 0x08

RECORD_SIZE = 30
HEADER_SIZE = 4

# --- Placement -----------------------------------------------------------
FOUNTAIN_TYPE = 105          # MODEL_WATERSPOUT
FOUNTAIN_TILE = (141, 128)   # measured from the live EncTerrain1.obj record
FOUNTAIN_POS = (14100.0, 12800.0, 165.0)

ARENA_SLOT = 49              # unused Lorencia world-object slot (gap 47-49
                              # between MODEL_TOMB03=46 and MODEL_FIRE_LIGHT01=50)
ARENA_SCALE = 5.3            # native Object57 footprint radius ~66 units;
                              # 5.3x -> ~700 unit (7 tile) platform diameter
ARENA_YAW = 0.0
ARENA_RADIUS_TILES = 3.5     # platform-top circle: clear tiles + drop objects here

PLAZA_RECT = (131, 118, 151, 138)  # x0,y0,x1,y1 -- the fountain plaza ring
# (Provenance: this is the plaza extent used by the prior, reverted Lorencia
# combat-plaza attempt -- commit 063cae6ede5, "kPlazaMinX/Y=131/118 size 21x21"
# in the now-deleted LorenciaCombatPlaza.cpp -- reused here only as a plaza
# *boundary* for the new, data-baked safezone stamp; none of that commit's
# runtime code is reused.)


def map_decrypt(src: bytes) -> bytes:
    dst = bytearray(len(src))
    wkey = 0x5E
    for i, s in enumerate(src):
        dst[i] = ((s ^ MAP_KEY[i % 16]) - wkey) & 0xFF
        wkey = (s + 0x3D) & 0xFF
    return bytes(dst)


def map_encrypt(src: bytes) -> bytes:
    dst = bytearray(len(src))
    wkey = 0x5E
    for i, s in enumerate(src):
        d = ((s + wkey) & 0xFF) ^ MAP_KEY[i % 16]
        dst[i] = d
        wkey = (d + 0x3D) & 0xFF
    return bytes(dst)


def bux(src: bytes) -> bytes:
    return bytes(b ^ BUX[i % 3] for i, b in enumerate(src))


def dist_tiles(x: float, y: float) -> float:
    dx = (x - FOUNTAIN_POS[0]) / 100.0
    dy = (y - FOUNTAIN_POS[1]) / 100.0
    return (dx * dx + dy * dy) ** 0.5


# --- EncTerrain1.obj -------------------------------------------------------

def read_obj_records(data: bytes) -> list[dict]:
    (count,) = struct.unpack_from("<h", data, 2)
    out = []
    for i in range(count):
        off = HEADER_SIZE + i * RECORD_SIZE
        (typ,) = struct.unpack_from("<h", data, off)
        x, y, z = struct.unpack_from("<fff", data, off + 2)
        ax, ay, az = struct.unpack_from("<fff", data, off + 14)
        (scale,) = struct.unpack_from("<f", data, off + 26)
        out.append(dict(i=i, type=typ, x=x, y=y, z=z, ax=ax, ay=ay, az=az, scale=scale, raw=data[off:off + RECORD_SIZE]))
    return out


def pack_record(typ: int, x: float, y: float, z: float, ax: float, ay: float, az: float, scale: float) -> bytes:
    return struct.pack("<h fff fff f", typ, x, y, z, ax, ay, az, scale)


def build_obj(data: bytes) -> tuple[bytes, dict]:
    records = read_obj_records(data)
    already_present = any(r["type"] == ARENA_SLOT for r in records)

    dropped = []
    kept = []
    for r in records:
        if r["type"] == ARENA_SLOT:
            continue  # drop a pre-existing arena record; we re-add it below (idempotency)
        if r["type"] == FOUNTAIN_TYPE and dist_tiles(r["x"], r["y"]) < 0.1:
            dropped.append(r)
            continue
        if dist_tiles(r["x"], r["y"]) <= ARENA_RADIUS_TILES:
            dropped.append(r)
            continue
        kept.append(r)

    new_record = dict(type=ARENA_SLOT, x=FOUNTAIN_POS[0], y=FOUNTAIN_POS[1], z=FOUNTAIN_POS[2],
                       ax=0.0, ay=0.0, az=ARENA_YAW, scale=ARENA_SCALE)
    kept.append(new_record)

    out = bytearray(data[:2])
    out += struct.pack("<h", len(kept))
    for r in kept:
        out += pack_record(r["type"], r["x"], r["y"], r["z"], r["ax"], r["ay"], r["az"], r["scale"])

    info = dict(dropped=dropped, kept_count=len(kept), already_present=already_present, new_record=new_record)
    return bytes(out), info


# --- EncTerrain1.att -------------------------------------------------------

def build_att(payload: bytearray) -> dict:
    x0, y0, x1, y1 = PLAZA_RECT
    stamped = 0
    cleared = 0
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            idx = x + (y << 8)
            in_circle = dist_tiles(x * 100.0 + 50.0, y * 100.0 + 50.0) <= ARENA_RADIUS_TILES
            if in_circle:
                if payload[idx] != 0:
                    cleared += 1
                payload[idx] = 0
            else:
                if payload[idx] == 0:
                    payload[idx] = TW_SAFEZONE
                    stamped += 1
    return dict(stamped=stamped, cleared=cleared)


# --- top level --------------------------------------------------------------

def report(obj_info: dict, att_info: dict) -> None:
    print(f"EncTerrain1.obj: dropped {len(obj_info['dropped'])} record(s), "
          f"kept {obj_info['kept_count']} total, arena record already present: {obj_info['already_present']}")
    for r in obj_info["dropped"]:
        print(f"    drop i={r['i']:4d} type={r['type']:3d} tile=({r['x']/100:.2f},{r['y']/100:.2f})")
    nr = obj_info["new_record"]
    print(f"    add  type={nr['type']} tile=({nr['x']/100:.0f},{nr['y']/100:.0f}) "
          f"scale={nr['scale']} yaw={nr['az']}")
    print(f"EncTerrain1.att: stamped {att_info['stamped']} plaza tile(s) TW_SAFEZONE, "
          f"cleared {att_info['cleared']} platform tile(s)")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true", help="write EncTerrain1.obj/.att and copy model assets")
    ap.add_argument("--verify", action="store_true", help="dump the resulting records/tiles after (re)computing")
    args = ap.parse_args()

    if not SOURCE_BMD.exists() or not SOURCE_TEX.exists():
        print(f"missing source asset(s): {SOURCE_BMD} / {SOURCE_TEX}", file=sys.stderr)
        return 1

    obj_data = map_decrypt(OBJ_PATH.read_bytes())
    version, mapnum = obj_data[0], obj_data[1]
    if mapnum != MAP_INDEX:
        print(f"unexpected EncTerrain1.obj map number {mapnum}", file=sys.stderr)
        return 1
    new_obj, obj_info = build_obj(obj_data)

    att_dec = bux(map_decrypt(ATT_PATH.read_bytes()))
    hdr = att_dec[:4]
    if list(hdr) != [0, MAP_INDEX, 255, 255]:
        print(f"unexpected EncTerrain1.att header {list(hdr)}", file=sys.stderr)
        return 1
    payload = bytearray(att_dec[4:4 + 65536])
    att_info = build_att(payload)

    report(obj_info, att_info)

    if not args.write:
        print("\ndry run -- pass --write to write EncTerrain1.obj/.att and copy assets")
        return 0

    DEST_BMD.write_bytes(SOURCE_BMD.read_bytes())
    print(f"\ncopied {SOURCE_BMD} -> {DEST_BMD}")
    if DEST_TEX.exists() and DEST_TEX.read_bytes() == SOURCE_TEX.read_bytes():
        print(f"texture already present and identical: {DEST_TEX} (ships with Lorencia; not written)")
    else:
        DEST_TEX.write_bytes(SOURCE_TEX.read_bytes())
        print(f"copied {SOURCE_TEX} -> {DEST_TEX}")

    OBJ_PATH.write_bytes(map_encrypt(new_obj))
    ATT_PATH.write_bytes(map_encrypt(bux(bytes(hdr) + bytes(payload))))
    print(f"wrote {OBJ_PATH}")
    print(f"wrote {ATT_PATH}")

    # round-trip verification
    check_obj = map_decrypt(OBJ_PATH.read_bytes())
    (check_count,) = struct.unpack_from("<h", check_obj, 2)
    assert check_count == obj_info["kept_count"], "obj record count mismatch after write"
    check_att_hdr, check_att = read_att_payload(ATT_PATH)
    assert list(check_att_hdr) == [0, MAP_INDEX, 255, 255]
    assert bytes(check_att) == bytes(payload), "att payload mismatch after write"
    print("round-trip verified")

    if args.verify:
        dump_verify(check_obj, check_att)
    return 0


def read_att_payload(path: Path) -> tuple[bytes, bytes]:
    dec = bux(map_decrypt(path.read_bytes()))
    return dec[:4], dec[4:4 + 65536]


def dump_verify(obj_data: bytes, att_payload: bytes) -> None:
    print("\n--- verify: EncTerrain1.obj records at/near the arena ---")
    for r in read_obj_records(obj_data):
        if dist_tiles(r["x"], r["y"]) <= ARENA_RADIUS_TILES + 1:
            print(f"  i={r['i']:4d} type={r['type']:3d} tile=({r['x']/100:.2f},{r['y']/100:.2f}) "
                  f"scale={r['scale']:.2f} yaw={r['az']%360:.1f}")

    print("\n--- verify: EncTerrain1.att grid around the plaza ---")
    x0, y0, x1, y1 = PLAZA_RECT
    for y in range(y0, y1 + 1):
        row = ""
        for x in range(x0, x1 + 1):
            v = att_payload[x + (y << 8)]
            if v & TW_NOGROUND:
                row += "G"
            elif v & TW_NOMOVE:
                row += "#"
            elif v == TW_SAFEZONE:
                row += "S"
            elif v == 0:
                row += "."
            else:
                row += "?"
        print(f"  y={y:3d} {row}")

    safe = sum(1 for v in att_payload if v == TW_SAFEZONE)
    circle_tiles = [(x, y) for y in range(256) for x in range(256)
                    if dist_tiles(x * 100.0 + 50.0, y * 100.0 + 50.0) <= ARENA_RADIUS_TILES]
    circle_bad = [(x, y) for x, y in circle_tiles if att_payload[x + (y << 8)] != 0]
    print(f"\ntotal safezone tiles on sheet: {safe}")
    print(f"platform-top circle tiles: {len(circle_tiles)}, non-walkable-or-safezone among them: {len(circle_bad)}")
    if circle_bad:
        print(f"  !! {circle_bad}")


if __name__ == "__main__":
    raise SystemExit(main())
