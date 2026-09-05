"""Rebuild the Arena/Stadium (World7, map 6) walkability from the authored ATT.

Round 3 (client c3102d8a / server update 190) replaced the authored Stadium ATT
with a synthetic "every rendered tile is walkable" map. That was wrong: World7 is
a *partial* map. Everything the designer built lives in x 0..118 / y 0..188 --
all 1486 map objects, all terrain relief (height varies only there; the rest of
the sheet is a flat 149) and every non-default ground texture. The remaining
~3/4 of the 256x256 sheet is engine filler that the client still *renders* as
grass but that the authoring marks TW_NOGROUND (0x08). The client pathfinder
walks a tile only when its attribute is < TW_NOMOVE (see PATH::FindPath,
`iWall > byMapAttribute`), so 0x08 is a wall. Round 3 cleared 0x08 from ~50k
tiles, so the hero could walk off the built campus onto an empty plane, and it
also stamped object footprints *inside* the hunting cages, shrinking them from
49 to 41-46 walkable tiles.

This tool restores the authored walkability, seals the walkable leftovers the
editor left on the sheet edges and re-applies the LuxView design on top (13 cage
doors, the warp landing rect and the plaza safezone pad), writing one identical
65536-byte attribute payload into the three files that must agree:

  * MuMain/src/bin/Data/World7/EncTerrain7.att  (client, encrypted, 4B header)
  * MuMain/src/bin/Data/World7/Terrain7.att     (client copy of the server blob)
  * OpenMU/src/Persistence/Initialization/Resources/Terrain7.att (server, 3B header)

Round 5 (server update 192) narrowed the safezone: the stock authoring flags the
whole stadium interior TW_SAFEZONE and the design used to add a pad around the
warp arrival, so mobs and players were unattackable far outside the plaza. The
mask now strips the bit from the entire sheet and stamps only the plaza.

Round 6 (server update 193) fits that stamp to the plaza walls. Round 5 used a
Chebyshev pad (65,43 r10 -> x 55..75, y 33..53) that was drawn by eye and spilled
a full four tiles past the masonry into the west corridor - the owner stood at
(59,34), outside the plaza, and could not be attacked. The plaza is walled by four
L-shaped corner pieces in EncTerrain7.obj (Object7 types 21/22/23, the 200x200 and
164x115 unit wall models); their outer faces are the plaza's real boundary. Taking
the model footprints (position +- the rotated, scaled BMD bounding box) the four
corners span world x 5900..7297, y 3700..5300, i.e. tiles x 59..72, y 37..52, and
that rectangle is now the whole safezone.

The punch mirrors MUnique.OpenMU.GameLogic.ArenaCageDoors.PunchTerrain exactly and
the sealing is baked into the payload, so the update plug-in that re-bakes the live
blobs from that resource converges on the same bytes.

Usage:
    py -3.12 tools/arena_terrain_restore.py                  # report only
    py -3.12 tools/arena_terrain_restore.py --write          # rebuild the 3 files

The authored source is the Season 8 reference client's World7 ATT, archived at
``_arena-ref/EncTerrain7.att.refclient80``. It is preferred over the base
client's own authoring because it keeps the road neck into the stadium open and
reaches all 13 cages; both agree everywhere else that matters.
"""
from __future__ import annotations

import argparse
import sys
from collections import deque
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
OPENMU = REPO.parent / "OpenMU"
ARENA_REF = REPO.parent / "_arena-ref"

SOURCE_ATT = ARENA_REF / "EncTerrain7.att.refclient80"
CLIENT_ENC = REPO / "src" / "bin" / "Data" / "World7" / "EncTerrain7.att"
CLIENT_RAW = REPO / "src" / "bin" / "Data" / "World7" / "Terrain7.att"
SERVER_RES = OPENMU / "src" / "Persistence" / "Initialization" / "Resources" / "Terrain7.att"

MAP_INDEX = 7  # World7 == map 6, the ATT header carries the 1-based world number
TW_SAFEZONE = 0x01
TW_NOMOVE = 0x04
TW_NOGROUND = 0x08

MAP_KEY = bytes([0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
                 0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2])
BUX = bytes([0xFC, 0xCF, 0xAB])

# --- LuxView design, mirrored from OpenMU GameLogic/ArenaCageDoors.cs ---------
TERRAIN_HOLES = [
    (16, 37, 16, 39), (16, 55, 16, 57), (16, 73, 16, 75), (16, 89, 18, 92),
    (23, 37, 24, 39), (23, 55, 24, 57), (23, 71, 24, 74), (23, 88, 24, 91),
    (41, 37, 41, 39), (41, 55, 41, 58), (41, 69, 41, 72), (41, 79, 41, 82),
    (60, 73, 64, 75),
]
HUNTING_BOXES = [
    (9, 35, 15, 41), (9, 53, 15, 59), (9, 71, 15, 77), (9, 88, 15, 94),
    (25, 35, 31, 41), (25, 53, 31, 59), (25, 70, 31, 76), (25, 87, 31, 93),
    (45, 35, 51, 41), (45, 54, 51, 60), (45, 69, 51, 74), (45, 78, 51, 84),
    (65, 71, 71, 77),
]
# The Arena is a PvP map: the ONLY no-attack area is the walled plaza with the
# fountain. Its bounds are not a guess - they are the bounding box of the four
# L-shaped masonry corners that fence the paved square, read out of
# EncTerrain7.obj + Data/Object7/*.bmd (see tools/arena_plaza_walls.py):
#
#   corner (-x,-y): records 664/665/666/671/672  world x 5900..6300, y 3700..4300
#   corner (-x,+y): records 697/698/699/702/703  world x 5900..6300, y 4700..5300
#   corner (+x,-y): records 1055/1071            world x 6900..7297, y 3700..3900
#   corner (+x,+y): records 1110/1113            world x 6900..7297, y 5100..5300
#
# Bounding box world x 5900..7297.2, y 3700..5300 -> tiles x 59..72, y 37..52
# (100 world units = 1 tile; tile t covers [t*100, (t+1)*100)). It lines up with
# the paved floor in EncTerrain7.map, which carries ground texture 4 on
# x 60..73 / y 38..51 - exactly one tile of masonry outside the pavement on the
# west and on both y sides, and the stadium building on the east.
#
# Everything else is fair game, including the warp arrival, the west corridor and
# the stadium field. The stock Season 8 authoring flags the whole stadium interior
# (x 51..74, y 130..184) TW_SAFEZONE, so the mask below has to clear the bit over
# the *whole* sheet, not just inside a campus rect - that leftover is what made
# mobs unattackable outside the plaza (owner report at 98,113).
PLAZA_RECT = (59, 37, 72, 52)
PLAZA = (65, 43)  # a walkable reference tile inside the plaza (reachability check)
SAFEZONE_RECTS = [PLAZA_RECT]
WARP = (102, 116)  # arrival tile of ExitGate 50 - walkable, NOT safezone
# ExitGate 50, the only way into the map. The authoring leaves only its west
# column walkable, so the punch opens the whole rect (ArenaCageDoors.WarpLanding).
WARP_LANDING = [(101, 115, 103, 117)]

# Bounds of everything the designer actually built: every map object, all terrain
# relief and every non-default ground texture live inside this box. Outside it the
# sheet is flat filler, so any walkable tile there is authoring leftover on the map
# edges - sealed so no spawn picker or GM move can drop anyone off the map.
BUILT_MAX_X = 118
BUILT_MAX_Y = 188
OUTSIDE = 12  # TW_NOGROUND | TW_NOMOVE, the value the authoring uses off-map


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


def read_enc_att(path: Path) -> tuple[bytes, bytes]:
    dec = bux(map_decrypt(path.read_bytes()))
    return dec[:4], dec[4:4 + 65536]


def tiles(rects):
    for x1, y1, x2, y2 in rects:
        for x in range(x1, x2 + 1):
            for y in range(y1, y2 + 1):
                yield x, y


def in_rects(x, y, rects):
    return any(x1 <= x <= x2 and y1 <= y <= y2 for x1, y1, x2, y2 in rects)


def is_player_safe_tile(x, y):
    if in_rects(x, y, TERRAIN_HOLES) or in_rects(x, y, HUNTING_BOXES):
        return False
    return in_rects(x, y, [PLAZA_RECT])


def punch(payload: bytes) -> bytes:
    """Mirror of ArenaCageDoors.PunchTerrain over the bare 65536-byte payload."""
    w = bytearray(payload)
    for x, y in tiles(TERRAIN_HOLES):
        w[x + (y << 8)] = 0
    for x, y in tiles(WARP_LANDING):
        w[x + (y << 8)] = 0
    # ApplySafezoneMask: strip TW_SAFEZONE from the whole sheet (walkable tiles
    # and blocked ones alike, so the flag is nowhere but the plaza), then stamp
    # the plaza pad back on.
    for y in range(256):
        for x in range(256):
            i = x + (y << 8)
            if (w[i] & TW_SAFEZONE) and not is_player_safe_tile(x, y):
                w[i] &= ~TW_SAFEZONE
    for x, y in tiles(SAFEZONE_RECTS):
        i = x + (y << 8)
        if is_player_safe_tile(x, y) and w[i] in (0, TW_SAFEZONE):
            w[i] = TW_SAFEZONE
    return bytes(w)


def walkable(v: int) -> bool:
    return not (v & TW_NOMOVE) and not (v & TW_NOGROUND)


def component_from(payload: bytes, seeds) -> set:
    start = next((s for s in seeds if walkable(payload[s[0] + (s[1] << 8)])), None)
    if start is None:
        return set()
    seen = {start}
    q = deque([start])
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < 256 and 0 <= ny < 256 and (nx, ny) not in seen \
                    and walkable(payload[nx + (ny << 8)]):
                seen.add((nx, ny))
                q.append((nx, ny))
    return seen


def seal_outside(payload: bytes) -> tuple[bytes, int]:
    """Seal the walkable tiles the authoring left on the map edges.

    They sit outside everything the designer built, are disconnected from the
    campus and from the stadium, and only exist because the editor wrote the
    whole 256x256 sheet. Sealing them keeps random spawn pickers inside the map.
    """
    w = bytearray(payload)
    sealed = 0
    for y in range(256):
        for x in range(256):
            if x <= BUILT_MAX_X and y <= BUILT_MAX_Y:
                continue
            i = x + (y << 8)
            if walkable(w[i]):
                w[i] = OUTSIDE
                sealed += 1
    return bytes(w), sealed


def report(payload: bytes) -> bool:
    total = sum(1 for v in payload if walkable(v))
    landing = list(tiles(WARP_LANDING))
    reach = component_from(payload, landing)
    ok = True
    print(f"  walkable tiles ........ {total}")
    print(f"  reachable from warp ... {len(reach)}")
    blocked = [p for p in landing if not walkable(payload[p[0] + (p[1] << 8)])]
    if blocked:
        print(f"  !! warp landing tiles still blocked: {blocked}")
        ok = False
    for i, (x1, y1, x2, y2) in enumerate(HUNTING_BOXES):
        inside = [(x, y) for x in range(x1, x2 + 1) for y in range(y1, y2 + 1)]
        free = sum(1 for p in inside if walkable(payload[p[0] + (p[1] << 8)]))
        hit = sum(1 for p in inside if p in reach)
        flag = "OK " if hit else "!! "
        if not hit:
            ok = False
        print(f"  {flag}cage {i:2d} ({x1},{y1})-({x2},{y2}) walkable {free}/{len(inside)} reachable {hit}")
    if PLAZA not in reach:
        print("  !! plaza centre unreachable")
        ok = False
    x0, y0, x1, y1 = PLAZA_RECT
    flagged = [(i & 0xFF, i >> 8) for i, v in enumerate(payload) if v & TW_SAFEZONE]
    outside = [(x, y) for x, y in flagged if not (x0 <= x <= x1 and y0 <= y <= y1)]
    walk_safe = sum(1 for x, y in flagged if walkable(payload[x + (y << 8)]))
    print(f"  safezone tiles ........ {len(flagged)} ({walk_safe} walkable)")
    if outside:
        xs = [p[0] for p in outside]
        ys = [p[1] for p in outside]
        print(f"  !! {len(outside)} safezone tiles outside the plaza "
              f"(x {min(xs)}..{max(xs)}, y {min(ys)}..{max(ys)})")
        ok = False
    else:
        print(f"  safezone contained .... yes (plaza x {x0}..{x1}, y {y0}..{y1})")
    for sx, sy in ((59, 34), (55, 45), (65, 53), (98, 113)):
        if payload[sx + (sy << 8)] & TW_SAFEZONE:
            print(f"  !! sentinel ({sx},{sy}) outside the plaza walls is safezone")
            ok = False
    for sx, sy in ((64, 45), (69, 45), (66, 41), (66, 49)):
        if not payload[sx + (sy << 8)] & TW_SAFEZONE:
            print(f"  !! sentinel ({sx},{sy}) next to the fountain is NOT safezone")
            ok = False
    if not (payload[WARP[0] + (WARP[1] << 8)] & TW_SAFEZONE):
        print(f"  warp arrival PvP ...... yes ({WARP[0]},{WARP[1]})")
    else:
        print(f"  !! warp arrival {WARP} is still safezone")
        ok = False
    return ok


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true", help="write the three files")
    args = ap.parse_args()

    hdr, authored = read_enc_att(SOURCE_ATT)
    if list(hdr) != [0, MAP_INDEX, 255, 255]:
        print(f"unexpected source header {list(hdr)}", file=sys.stderr)
        return 1
    payload, sealed = seal_outside(punch(authored))

    print(f"source: {SOURCE_ATT.name}")
    print(f"  sealed off-map tiles .. {sealed}")
    ok = report(payload)
    if not ok:
        print("verification FAILED - not writing", file=sys.stderr)
        return 1

    if not args.write:
        _, current = read_enc_att(CLIENT_ENC)
        diff = sum(1 for a, b in zip(current, payload) if a != b)
        print(f"\ndry run: {diff} tiles would change in {CLIENT_ENC.name}")
        return 0

    CLIENT_ENC.write_bytes(map_encrypt(bux(bytes([0, MAP_INDEX, 255, 255]) + payload)))
    blob = bytes([0, 255, 255]) + payload
    CLIENT_RAW.write_bytes(blob)
    SERVER_RES.write_bytes(blob)
    print(f"\nwrote {CLIENT_ENC}\nwrote {CLIENT_RAW}\nwrote {SERVER_RES}")

    check_hdr, check = read_enc_att(CLIENT_ENC)
    assert list(check_hdr) == [0, MAP_INDEX, 255, 255] and check == payload
    assert SERVER_RES.read_bytes()[3:] == payload
    print("round-trip verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
