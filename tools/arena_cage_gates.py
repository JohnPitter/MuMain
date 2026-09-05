"""Prove that every Arena hunting-pen door is the gate the artist drew.

The Arena (World7) has thirteen hunting boxes inside twelve fenced pens. Each pen
is a run of ``담장`` fence sections (``Object11.bmd``, 198 x 185 units) with one
gap in it, and that gap is flanked by a pair of ``작은기둥`` pillars
(``Object10.bmd``, 62 x 62 x 238 units). The pair of pillars *is* the main gate:
it is the only opening the map author left. (Twelve pens for thirteen boxes -
Iron Wheel and Mutant share one long pen with a single gate.)

Round 7 exists because nine of the thirteen LuxView doors had been punched on the
wrong face of the pen - through a solid rail run - while the authored gate on the
other side stayed open. The owner, standing in the cage at (27,57), put it
plainly: *"you can walk through the bars, you shouldn't; the player has to get in
and out only through the main gate."* This tool is the falsifiable version of
that sentence:

  * a door tile must sit under **no** ``담장`` footprint (you cannot walk through
    a fence), and
  * a door must be flanked by ``작은기둥`` pillars (that is what makes it a gate).

Both numbers come from the placements in ``Data/World7/EncTerrain7.obj`` and the
model bounding boxes in ``Data/Object7/*.bmd``, rotated and scaled the way the
client does - the same method as ``tools/arena_plaza_walls.py``.

For the record, the old rects fail it exactly where the owner said they would:

    (16,89)-(18,92) Poison Bull   8 fence tiles, 0 pillars touching
    (23,37)-(24,39) Gorgon        6 fence tiles, 0 pillars touching
    (23,55)-(24,57) Shadow        6 fence tiles, 0 pillars touching
    (23,71)-(24,74) Devil         8 fence tiles, 0 pillars touching
    (23,88)-(24,91) Death Cow     8 fence tiles, 0 pillars touching
    (41,37)-(41,39) Bahamut       3 fence tiles, 0 pillars touching
    (41,55)-(41,58) Lizard King   4 fence tiles, 0 pillars touching
    (41,69)-(41,72) Iron Wheel    4 fence tiles, 0 pillars touching
    (41,79)-(41,82) Mutant        4 fence tiles, 0 pillars touching

Usage:
    py -3.12 tools/arena_cage_gates.py
"""
from __future__ import annotations

from arena_plaza_walls import read_objects, tile_span, world_box
from arena_terrain_restore import (CLIENT_ENC, HUNTING_BOXES, NEIGHBOURS8,
                                   TERRAIN_HOLES, read_enc_att, tiles, walkable)

FENCE_TYPE = 10   # Object11.bmd - 담장, the fence run
PILLAR_TYPE = 9   # Object10.bmd - 작은기둥, the gate pillar

# The pens themselves are read off the shipped attribute sheet with the doors
# shut: with every door tile treated as a wall, each hunting box must sit in a
# closed room. Two rooms hold two boxes (Iron Wheel and Mutant share one long
# pen), so thirteen boxes make twelve pens.
CLIENT_ATT = CLIENT_ENC


def footprints() -> tuple[set, set]:
    """Tile masks of the fence sections and of the gate pillars."""
    fence: set = set()
    pillar: set = set()
    boxes: dict = {}
    for obj in read_objects():
        if obj["type"] not in (FENCE_TYPE, PILLAR_TYPE):
            continue
        x0, y0, x1, y1 = world_box(obj, boxes)
        tx0, tx1 = tile_span(x0, x1)
        ty0, ty1 = tile_span(y0, y1)
        target = fence if obj["type"] == FENCE_TYPE else pillar
        for x in range(tx0, tx1 + 1):
            for y in range(ty0, ty1 + 1):
                target.add((x, y))
    return fence, pillar


def flood(payload: bytes, start, blocked) -> set:
    seen = {start}
    stack = [start]
    while stack:
        x, y = stack.pop()
        for dx, dy in NEIGHBOURS8:
            n = (x + dx, y + dy)
            if n in seen or n in blocked or not (0 <= n[0] < 256 and 0 <= n[1] < 256):
                continue
            if walkable(payload[n[0] + (n[1] << 8)]):
                seen.add(n)
                stack.append(n)
    return seen


def pens_with_doors_shut() -> dict:
    """Group the hunting boxes by the closed room they sit in."""
    _, payload = read_enc_att(CLIENT_ATT)
    doors = set(tiles(TERRAIN_HOLES))
    pens: dict = {}
    for index, (x1, y1, x2, y2) in enumerate(HUNTING_BOXES):
        start = next((x, y) for x in range(x1, x2 + 1) for y in range(y1, y2 + 1)
                     if walkable(payload[x + (y << 8)]))
        pens.setdefault(frozenset(flood(payload, start, doors)), []).append(index)
    return pens


def main() -> int:
    fence, pillar = footprints()
    print(f"EncTerrain7.obj: {len(fence)} tiles under 담장 fence sections, "
          f"{len(pillar)} under 작은기둥 pillars")

    pens = pens_with_doors_shut()
    print(f"\n{CLIENT_ATT.name} with the doors shut: {len(pens)} closed pens "
          f"for {len(HUNTING_BOXES)} hunting boxes")
    for pen, ids in sorted(pens.items(), key=lambda it: min(it[1])):
        xs = [t[0] for t in pen]
        ys = [t[1] for t in pen]
        print(f"  cages {ids}: {len(pen):4d} tiles, x {min(xs)}..{max(xs)}, y {min(ys)}..{max(ys)}")
        if len(pen) > 400:
            print("    !! not a closed pen - it spills into the campus")
            return 1

    ok = True
    print(f"\ndoors in arena_terrain_restore.py ({len(TERRAIN_HOLES)} for {len(pens)} pens)")
    for x1, y1, x2, y2 in TERRAIN_HOLES:
        door = [(x, y) for x in range(x1, x2 + 1) for y in range(y1, y2 + 1)]
        under_fence = sorted(t for t in door if t in fence)
        touching = {(x + dx, y + dy) for x, y in door
                    for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                    if (x + dx, y + dy) in pillar}
        flag = "OK " if not under_fence and len(touching) >= 2 else "!! "
        print(f"  {flag}({x1},{y1})-({x2},{y2}) {len(door):2d} tiles · "
              f"under fence {len(under_fence)} · pillar tiles touching {len(touching)}")
        if under_fence:
            print(f"      !! these door tiles are cut through the fence: {under_fence}")
            ok = False
        if len(touching) < 2:
            print("      !! no pillars around this door - it is not the gate")
            ok = False

    # Every pen must own exactly one door, and every door exactly one pen.
    print("\ndoor <-> pen mapping")
    door_tiles = list(tiles(TERRAIN_HOLES))
    for pen, ids in sorted(pens.items(), key=lambda it: min(it[1])):
        neighbours = {t for t in door_tiles
                      if any((t[0] + dx, t[1] + dy) in pen for dx, dy in NEIGHBOURS8)}
        owning = [r for r in TERRAIN_HOLES
                  if any(t in neighbours for t in
                         [(x, y) for x in range(r[0], r[2] + 1) for y in range(r[1], r[3] + 1)])]
        flag = "OK " if len(owning) == 1 else "!! "
        if len(owning) != 1:
            ok = False
        print(f"  {flag}cages {ids}: doors {owning}")

    if not ok:
        print("\nFAILED - a door is not a gate", flush=True)
        return 1
    print("\nevery door is a gap in the fence flanked by pillars, one per pen")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
