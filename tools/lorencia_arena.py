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
4. Clears TW_SAFEZONE and TW_NOMOVE from the new platform's footprint circle
   in EncTerrain1.att. Nothing else in the plaza is touched.

   CORRECTION (2026-09-06): an earlier version of this tool assumed
   ``EncTerrain1.att`` is always 1 byte/tile (65 536-byte payload). Lorencia's
   file has always shipped as the "extAtt" layout instead -- 2 bytes/tile
   (131 072-byte payload, little-endian WORD per tile, only the low byte
   carries the attribute bits; see the client engine's
   ``ZzzLodTerrain::OpenTerrainAttribute``, which accepts both sizes). Reading
   that as 1 byte/tile pulls every tile's low byte *and* the next tile's high
   byte into the array as if they were two consecutive tiles, silently
   desyncing the whole 256x256 sheet by one byte per tile pair. That bug
   corrupted ~31.6% of the tiles outside the plaza (walls, holes, the
   safezone shape) in the file this tool previously wrote, and the previous
   revision of this docstring additionally claimed "the official plaza has no
   safezone tiles at all" -- also false: measured against the pristine,
   correctly decoded source, the whole plaza rect (x 131..151, y 118..138) is
   100% flagged ``TW_SAFEZONE`` already (either alone, value 1, or combined
   with ``TW_NOMOVE`` on wall/planter tiles, value 5 -- including the
   fountain's own footprint). There was nothing to "restore" and nothing new
   to stamp: this tool now only clears the platform-top circle (32 tiles,
   center 141,128, radius 3.5 tiles) of both ``TW_SAFEZONE`` and
   ``TW_NOMOVE`` -- the plaza was always safe, and the platform now
   deliberately carves an open-PvP, walkable hole out of it. The output is
   written back in the SAME layout the input was read in (extAtt for
   Lorencia), so the file no longer silently loses its high bytes.

   This alone does not yet enable non-duel PvP server-side (PvpRules.
   IsPvpByDesignMap needs a map-scoped check; not implemented in this pass --
   /duel already works anywhere, including on the platform).

Usage:
    py -3.12 tools/lorencia_arena.py                 # report only (dry run)
    py -3.12 tools/lorencia_arena.py --write          # write the 3 files + copy assets
    py -3.12 tools/lorencia_arena.py --verify         # dump the resulting records/tiles

Idempotent: running --write twice produces byte-identical output the second
time (the tool detects the arena record/model files already being in place
and no-ops instead of double-inserting; the platform circle is already
cleared the second time, so re-clearing it is a no-op too).
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

TILE_COUNT = 256 * 256
STD_BODY_SIZE = TILE_COUNT           # 1 byte/tile
EXT_BODY_SIZE = TILE_COUNT * 2       # 2 bytes/tile (WORD per tile, low byte used)

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

# Plaza rect kept only for the --verify grid dump (visualization); the tool no
# longer writes anything to tiles outside the platform circle -- see the
# 2026-09-06 correction in the module docstring.
PLAZA_RECT = (131, 118, 151, 138)  # x0,y0,x1,y1


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


def platform_circle_tiles() -> list[tuple[int, int]]:
    """The 32 tiles of the platform-top circle (tile-center distance <= radius)."""
    cx, cy = FOUNTAIN_TILE
    span = int(ARENA_RADIUS_TILES) + 2
    return [
        (x, y)
        for y in range(cy - span, cy + span + 1)
        for x in range(cx - span, cx + span + 1)
        if dist_tiles(x * 100.0 + 50.0, y * 100.0 + 50.0) <= ARENA_RADIUS_TILES
    ]


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

def read_att(path: Path) -> tuple[bytes, bytearray, bytearray | None, str]:
    """Decode a .att file, detecting its layout from the decoded body size.

    Returns ``(header, low, high, layout)``:

    * ``low`` is the 65 536-byte attribute array (one byte per tile, low byte
      of the WORD in the extAtt layout).
    * ``high`` is the 65 536-byte array of preserved high bytes for the
      extAtt layout, or ``None`` for the plain 1 byte/tile layout.
    * ``layout`` is ``"std"`` (65 540-byte file, 1 byte/tile) or ``"ext"``
      (131 076-byte file, 2 bytes/tile) -- see ``ZzzLodTerrain::
      OpenTerrainAttribute`` in the client engine, which accepts both.
    """
    dec = bux(map_decrypt(path.read_bytes()))
    hdr = dec[:4]
    body = dec[4:]
    if len(body) == STD_BODY_SIZE:
        return hdr, bytearray(body), None, "std"
    if len(body) == EXT_BODY_SIZE:
        low = bytearray(body[0::2])
        high = bytearray(body[1::2])
        return hdr, low, high, "ext"
    raise ValueError(f"unexpected EncTerrain.att decoded body size {len(body)} (expected {STD_BODY_SIZE} or {EXT_BODY_SIZE})")


def write_att(path: Path, hdr: bytes, low: bytearray, high: bytearray | None, layout: str) -> None:
    """Write a .att file back in the SAME layout it was read in."""
    if layout == "std":
        body = bytes(low)
    elif layout == "ext":
        assert high is not None, "extAtt layout requires the preserved high-byte array"
        interleaved = bytearray(EXT_BODY_SIZE)
        interleaved[0::2] = low
        interleaved[1::2] = high
        body = bytes(interleaved)
    else:
        raise ValueError(f"unknown att layout {layout!r}")
    path.write_bytes(map_encrypt(bux(bytes(hdr) + body)))


def build_att(payload: bytearray) -> dict:
    """Clears TW_SAFEZONE and TW_NOMOVE on the platform-top circle only.

    Nothing outside the circle is touched -- the rest of the plaza is
    already safezone in the source data (see the module docstring's
    2026-09-06 correction), so there is nothing to (re)stamp.
    """
    circle = platform_circle_tiles()
    cleared_safezone = 0
    cleared_nomove = 0
    before = {}
    for x, y in circle:
        idx = x + (y << 8)
        before[(x, y)] = payload[idx]
        if payload[idx] & TW_SAFEZONE:
            cleared_safezone += 1
        if payload[idx] & TW_NOMOVE:
            cleared_nomove += 1
        payload[idx] &= ~(TW_SAFEZONE | TW_NOMOVE) & 0xFF
    return dict(circle=circle, before=before, cleared_safezone=cleared_safezone, cleared_nomove=cleared_nomove)


# --- top level --------------------------------------------------------------

def report(obj_info: dict, att_info: dict) -> None:
    print(f"EncTerrain1.obj: dropped {len(obj_info['dropped'])} record(s), "
          f"kept {obj_info['kept_count']} total, arena record already present: {obj_info['already_present']}")
    for r in obj_info["dropped"]:
        print(f"    drop i={r['i']:4d} type={r['type']:3d} tile=({r['x']/100:.2f},{r['y']/100:.2f})")
    nr = obj_info["new_record"]
    print(f"    add  type={nr['type']} tile=({nr['x']/100:.0f},{nr['y']/100:.0f}) "
          f"scale={nr['scale']} yaw={nr['az']}")
    print(f"EncTerrain1.att: platform circle ({len(att_info['circle'])} tiles) -- "
          f"cleared TW_SAFEZONE on {att_info['cleared_safezone']}, TW_NOMOVE on {att_info['cleared_nomove']}; "
          f"nothing else touched (plaza was already safezone)")


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

    hdr, low, high, layout = read_att(ATT_PATH)
    if list(hdr) != [0, MAP_INDEX, 255, 255]:
        print(f"unexpected EncTerrain1.att header {list(hdr)}", file=sys.stderr)
        return 1
    print(f"EncTerrain1.att layout detected: {layout} "
          f"({'131 072' if layout == 'ext' else '65 536'}-byte body)")

    before_low = bytearray(low)  # snapshot for --verify's outside-circle identity check
    att_info = build_att(low)

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
    write_att(ATT_PATH, hdr, low, high, layout)
    print(f"wrote {OBJ_PATH}")
    print(f"wrote {ATT_PATH} ({layout} layout)")

    # round-trip verification
    check_obj = map_decrypt(OBJ_PATH.read_bytes())
    (check_count,) = struct.unpack_from("<h", check_obj, 2)
    assert check_count == obj_info["kept_count"], "obj record count mismatch after write"
    check_hdr, check_low, check_high, check_layout = read_att(ATT_PATH)
    assert list(check_hdr) == [0, MAP_INDEX, 255, 255]
    assert check_layout == layout, "att layout changed across write -- must be preserved"
    assert bytes(check_low) == bytes(low), "att low-byte payload mismatch after write"
    if layout == "ext":
        assert bytes(check_high) == bytes(high), "att high-byte payload mismatch after write"
    print("round-trip verified")

    if args.verify:
        dump_verify(check_obj, before_low, check_low, att_info)
    return 0


def dump_verify(obj_data: bytes, before_low: bytearray, after_low: bytearray, att_info: dict) -> None:
    print("\n--- verify: EncTerrain1.obj records at/near the arena ---")
    for r in read_obj_records(obj_data):
        if dist_tiles(r["x"], r["y"]) <= ARENA_RADIUS_TILES + 1:
            print(f"  i={r['i']:4d} type={r['type']:3d} tile=({r['x']/100:.2f},{r['y']/100:.2f}) "
                  f"scale={r['scale']:.2f} yaw={r['az']%360:.1f}")

    print("\n--- verify: EncTerrain1.att grid around the plaza (before -> after) ---")
    x0, y0, x1, y1 = PLAZA_RECT

    def render(payload: bytearray) -> list[str]:
        rows = []
        for y in range(y0, y1 + 1):
            row = ""
            for x in range(x0, x1 + 1):
                v = payload[x + (y << 8)]
                if v & TW_NOGROUND:
                    row += "G"
                elif v & TW_NOMOVE and v & TW_SAFEZONE:
                    row += "X"  # wall + safezone (e.g. planters/fences)
                elif v & TW_NOMOVE:
                    row += "#"
                elif v == TW_SAFEZONE:
                    row += "S"
                elif v == 0:
                    row += "."
                else:
                    row += "?"
            rows.append(row)
        return rows

    before_rows = render(before_low)
    after_rows = render(after_low)
    for y, (b, a) in enumerate(zip(before_rows, after_rows), start=y0):
        marker = "  (changed)" if b != a else ""
        print(f"  y={y:3d} before={b} after={a}{marker}")

    # (a)+(b): outside-circle byte-identity assertion
    circle_set = set(att_info["circle"])
    mismatches = [
        (x, y, before_low[x + (y << 8)], after_low[x + (y << 8)])
        for y in range(256)
        for x in range(256)
        if (x, y) not in circle_set and before_low[x + (y << 8)] != after_low[x + (y << 8)]
    ]
    if mismatches:
        print(f"\n!! {len(mismatches)} tile(s) outside the platform circle changed -- THIS IS A BUG:")
        for m in mismatches[:20]:
            print(f"   {m}")
        raise SystemExit("verify failed: tiles outside the platform circle were modified")
    print(f"\nOK: all {65536 - len(circle_set)} tiles outside the platform circle are byte-identical before/after.")

    # (c): counts
    safe_before = sum(1 for v in before_low if v & TW_SAFEZONE)
    safe_after = sum(1 for v in after_low if v & TW_SAFEZONE)
    exact1_before = sum(1 for v in before_low if v == TW_SAFEZONE)
    exact1_after = sum(1 for v in after_low if v == TW_SAFEZONE)
    print(f"\nsafezone tiles (bit0 set) before={safe_before} after={safe_after} (delta={safe_after - safe_before})")
    print(f"safezone tiles (exact value==1) before={exact1_before} after={exact1_after} (delta={exact1_after - exact1_before})")
    print(f"platform circle: {len(att_info['circle'])} tiles, "
          f"cleared TW_SAFEZONE on {att_info['cleared_safezone']}, TW_NOMOVE on {att_info['cleared_nomove']}")
    print("circle tiles (x,y) before -> after:")
    for (x, y) in sorted(att_info["circle"], key=lambda t: (t[1], t[0])):
        b = att_info["before"][(x, y)]
        a = after_low[x + (y << 8)]
        print(f"  ({x:3d},{y:3d}) {b} -> {a}")


if __name__ == "__main__":
    raise SystemExit(main())
