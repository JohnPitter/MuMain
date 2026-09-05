"""Derive the Arena plaza safezone rectangle from the map objects themselves.

The Arena (World7) plaza is a paved square fenced by four L-shaped masonry
corners. Those corners are the plaza's *outer* boundary, so the no-attack area
is the bounding box of their footprints - not a radius picked by eye. Update 192
used a Chebyshev pad (65,43 r10 -> x 55..75, y 33..53) that spilled four tiles
past the masonry into the west corridor; the owner stood at (59,34), out on the
path, and could not be attacked.

This tool reads the placements from ``Data/World7/EncTerrain7.obj`` and the model
bounding boxes from ``Data/Object7/Object*.bmd``, rotates and scales each box the
way the client does, groups the wall pieces into the four corners and prints the
rectangle. It is the provenance of ``PLAZA_RECT`` in
``tools/arena_terrain_restore.py`` - run it whenever the plaza objects change.

Coordinates: 100 world units = 1 tile, and tile ``t`` covers ``[t*100,(t+1)*100)``
(the terrain quad between vertices ``t`` and ``t+1``), so a footprint edge that
lands exactly on ``t*100`` does not touch tile ``t``.

Usage:
    py -3.12 tools/arena_plaza_walls.py
"""
from __future__ import annotations

import math
import struct
from pathlib import Path

from arena_terrain_restore import PLAZA_RECT, map_decrypt

REPO = Path(__file__).resolve().parents[1]
OBJ_DIR = REPO / "src" / "bin" / "Data" / "Object7"
WORLD = REPO / "src" / "bin" / "Data" / "World7"

RECORD_SIZE = 30
TILE = 100.0

# Object7 wall models: Object22/23/24.bmd. 22 is the corner/pillar piece
# (164.8 x 115.0 units), 23 and 24 are the 200 x 200 unit rail spans.
WALL_TYPES = {21, 22, 23}

# The plaza cluster: wall pieces around the paved square, at full size. The
# small (scale <= 0.75) type 21/23 pieces at x 65..68 are the fountain, not the
# fence, so they are reported separately and excluded from the corners.
CLUSTER = (58.0, 36.0, 74.0, 54.0)  # x0, y0, x1, y1 in tiles
MIN_WALL_SCALE = 0.55
FOUNTAIN = (64.5, 41.5, 69.5, 48.5)


def bmd_aabb(path: Path) -> tuple[list[float], list[float]]:
    """Model-space bounding box of a World7 BMD (v0x0A plain, v0x0C encrypted)."""
    raw = path.read_bytes()
    if raw[:3] != b"BMD":
        raise SystemExit(f"{path.name}: not a BMD")
    version = raw[3]
    if version == 0x0C:
        (enc_size,) = struct.unpack_from("<i", raw, 4)
        data = map_decrypt(raw[8:8 + enc_size])
        ptr = 0
    elif version == 0x0A:
        data = raw
        ptr = 4
    else:
        raise SystemExit(f"{path.name}: BMD version {version} not handled")

    ptr += 32  # model name
    (num_meshes, _bones, _actions) = struct.unpack_from("<hhh", data, ptr)
    ptr += 6
    lo = [1e9] * 3
    hi = [-1e9] * 3
    for _ in range(num_meshes):
        nv, nn, nt, ntri, _tex = struct.unpack_from("<hhhhh", data, ptr)
        ptr += 10
        for _ in range(nv):
            pos = struct.unpack_from("<fff", data, ptr + 4)  # short node + pad
            for k in range(3):
                lo[k] = min(lo[k], pos[k])
                hi[k] = max(hi[k], pos[k])
            ptr += 16
        ptr += (nn * 20) + (nt * 8) + (ntri * 64) + 32  # normals, uvs, tris, texname
    return lo, hi


def read_objects() -> list[dict]:
    data = map_decrypt((WORLD / "EncTerrain7.obj").read_bytes())
    (count,) = struct.unpack_from("<h", data, 2)
    out = []
    for i in range(count):
        off = 4 + (i * RECORD_SIZE)
        (obj_type,) = struct.unpack_from("<h", data, off)
        x, y, z = struct.unpack_from("<fff", data, off + 2)
        _ax, _ay, az = struct.unpack_from("<fff", data, off + 14)
        (scale,) = struct.unpack_from("<f", data, off + 26)
        out.append({"i": i, "type": obj_type, "x": x, "y": y, "z": z,
                    "az": az % 360.0, "scale": scale})
    return out


def world_box(obj: dict, boxes: dict) -> tuple[float, float, float, float]:
    """Axis-aligned world footprint: model box, scaled, yawed, translated."""
    if obj["type"] not in boxes:
        boxes[obj["type"]] = bmd_aabb(OBJ_DIR / f"Object{obj['type'] + 1:02d}.bmd")
    lo, hi = boxes[obj["type"]]
    rad = math.radians(obj["az"])
    cos_a, sin_a = math.cos(rad), math.sin(rad)
    xs, ys = [], []
    for cx in (lo[0], hi[0]):
        for cy in (lo[1], hi[1]):
            sx, sy = cx * obj["scale"], cy * obj["scale"]
            xs.append(obj["x"] + (sx * cos_a) - (sy * sin_a))
            ys.append(obj["y"] + (sx * sin_a) + (sy * cos_a))
    return min(xs), min(ys), max(xs), max(ys)


def tile_span(lo: float, hi: float) -> tuple[int, int]:
    """Tiles a world interval touches; an edge exactly on a boundary does not."""
    first = math.floor(lo / TILE)
    last = math.ceil(hi / TILE) - 1
    return first, max(first, last)


def main() -> int:
    boxes: dict = {}
    corners: dict[str, list] = {"-x,-y": [], "-x,+y": [], "+x,-y": [], "+x,+y": []}
    fountain = []
    cx0, cy0, cx1, cy1 = CLUSTER
    for obj in read_objects():
        if obj["type"] not in WALL_TYPES or obj["scale"] < MIN_WALL_SCALE:
            continue
        tx, ty = obj["x"] / TILE, obj["y"] / TILE
        if not (cx0 <= tx <= cx1 and cy0 <= ty <= cy1):
            continue
        box = world_box(obj, boxes)
        fx0, fy0, fx1, fy1 = FOUNTAIN
        if fx0 <= tx <= fx1 and fy0 <= ty <= fy1:
            fountain.append((obj, box))
            continue
        key = ("-x" if obj["x"] < 6600 else "+x") + "," + ("-y" if obj["y"] < 4500 else "+y")
        corners[key].append((obj, box))

    print("plaza fence pieces (Object7 types 21/22/23, full size)")
    gx0 = gy0 = 1e9
    gx1 = gy1 = -1e9
    for key, items in corners.items():
        print(f"\n  corner {key}: {len(items)} pieces")
        for obj, (x0, y0, x1, y1) in sorted(items, key=lambda it: it[0]["i"]):
            tx0, tx1 = tile_span(x0, x1)
            ty0, ty1 = tile_span(y0, y1)
            print(f"    [{obj['i']:4d}] Object{obj['type'] + 1:02d}.bmd "
                  f"at ({obj['x'] / TILE:6.2f},{obj['y'] / TILE:6.2f}) "
                  f"yaw {obj['az']:5.1f} scale {obj['scale']:.2f} -> "
                  f"world x[{x0:7.1f},{x1:7.1f}] y[{y0:7.1f},{y1:7.1f}] "
                  f"tiles x {tx0}..{tx1}, y {ty0}..{ty1}")
        bx0 = min(b[0] for _, b in items)
        by0 = min(b[1] for _, b in items)
        bx1 = max(b[2] for _, b in items)
        by1 = max(b[3] for _, b in items)
        tx0, tx1 = tile_span(bx0, bx1)
        ty0, ty1 = tile_span(by0, by1)
        print(f"    L footprint: tiles x {tx0}..{tx1}, y {ty0}..{ty1}")
        gx0, gy0 = min(gx0, bx0), min(gy0, by0)
        gx1, gy1 = max(gx1, bx1), max(gy1, by1)

    print(f"\n  fountain (excluded, {len(fountain)} pieces): "
          + ", ".join(str(o["i"]) for o, _ in fountain))

    rx0, rx1 = tile_span(gx0, gx1)
    ry0, ry1 = tile_span(gy0, gy1)
    print(f"\nbounding box of the four L: world x[{gx0:.1f},{gx1:.1f}] y[{gy0:.1f},{gy1:.1f}]")
    print(f"  -> safezone rectangle x {rx0}..{rx1}, y {ry0}..{ry1} "
          f"({(rx1 - rx0 + 1) * (ry1 - ry0 + 1)} tiles)")
    print(f"  PLAZA_RECT in arena_terrain_restore.py: {PLAZA_RECT}")
    if (rx0, ry0, rx1, ry1) != PLAZA_RECT:
        print("  !! MISMATCH - update PLAZA_RECT (and ArenaCageDoors) before shipping")
        return 1
    print("  match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
