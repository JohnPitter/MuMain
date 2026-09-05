"""Round-3 Arena/Stadium terrain diagnostics + ATT reconstruction.

Renders the true VISUAL layout of World7 (ground texture classes from
EncTerrain7.map, object footprints from EncTerrain7.obj using per-model BMD
bounds), compares it with the shipped EncTerrain7.att walkability, and can
rebuild the ATT so that collision matches the picture tile by tile.

Modes:
  analyze  -> renders before-overlay + divergence stats + zoomed cage panel
  build    -> writes the rebuilt ATT (client Terrain7.att/EncTerrain7.att + raw)
  verify   -> renders after-overlay + re-checks divergences + door report
"""
from __future__ import annotations

import colorsys
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

REF = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_arena-ref")
sys.path.insert(0, str(REF))
import muatt  # noqa: E402

WT = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_wt-arena3")
WORLD = WT / "src" / "bin" / "Data" / "World7"
OBJ_DIR = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_wt-arena3\src\bin\Data\Object7")
OUT = REF

# --- object model classes ----------------------------------------------------
RAILS = {20}          # fence rails / low fences
SOLID = {0, 1, 2, 3, 4, 5, 6, 7, 11, 18, 19, 21, 22, 23}  # structures/posts
WALLS = {12, 13, 25}  # wall segments
POSTS = {8, 9, 26}    # posts, pillars
BLOCKY = {14, 15, 38}  # low stone blocks, crates
# everything else is decor: flat slabs (10), banners (16,17,27,28,29,30),
# statue (24), trees (31-36), notice (37), big billboards (39,40)

BLOCKING = RAILS | SOLID | WALLS | POSTS | BLOCKY

CLASS_COLOR = {
    "rails": (255, 170, 40),     # orange
    "solid": (200, 60, 200),     # magenta
    "walls": (255, 80, 80),      # red-ish
    "posts": (120, 120, 255),    # blue-ish
    "blocky": (160, 90, 40),     # brown
    "decor": (120, 160, 120),    # pale green
}


def klass(t: int) -> str:
    if t in RAILS:
        return "rails"
    if t in SOLID:
        return "solid"
    if t in WALLS:
        return "walls"
    if t in POSTS:
        return "posts"
    if t in BLOCKY:
        return "blocky"
    return "decor"


# --- model bounds (computed once via arena_bmd_footprints logic) -------------
def bmd_xz_bounds(path: Path) -> tuple[tuple[float, float], tuple[float, float]]:
    raw = path.read_bytes()
    version = raw[3]
    if version == 0x0C:
        (enc_size,) = struct.unpack_from("<i", raw, 4)
        data = muatt.map_decrypt(raw[8:8 + enc_size])
        ptr = 0
    elif version == 0x0A:
        data = raw
        ptr = 4
    else:
        raise SystemExit(f"{path.name}: version {version}")
    ptr += 32
    num_meshs, _nb, _na = struct.unpack_from("<hhh", data, ptr)
    ptr += 6
    minx = miny = 1e9
    maxx = maxy = -1e9
    for _ in range(num_meshs):
        nv, nn, nt, ntri, _tex = struct.unpack_from("<hhhhh", data, ptr)
        ptr += 10
        for _v in range(nv):
            x, yv, z = struct.unpack_from("<fff", data, ptr + 4)
            minx, maxx = min(minx, x), max(maxx, x)
            miny, maxy = min(miny, yv), max(maxy, yv)
            ptr += 16
        ptr += nn * 20 + nt * 8 + ntri * 64 + 32
    return (minx, miny), (maxx, maxy)


def load_models() -> dict[int, dict]:
    models = {}
    for t in range(0, 41):
        p = OBJ_DIR / f"Object{t + 1:02d}.bmd"
        if not p.exists():
            continue
        (x0, y0), (x1, y1) = bmd_xz_bounds(p)
        models[t] = {"min": (x0, y0), "max": (x1, y1)}
    return models


def load_objects() -> list[tuple[int, float, float, float]]:
    dec = muatt.map_decrypt((WORLD / "EncTerrain7.obj").read_bytes())
    (count,) = struct.unpack_from("<h", dec, 2)
    off = 4
    out = []
    for _ in range(count):
        (typ,) = struct.unpack_from("<h", dec, off)
        x, y, _z = struct.unpack_from("<fff", dec, off + 2)
        ax, ay, _az = struct.unpack_from("<fff", dec, off + 14)
        (scale,) = struct.unpack_from("<f", dec, off + 26)
        off += 30
        out.append((typ, x, y, ay if ay else ax))
    return out


def object_tiles(models, objects) -> tuple[set, set]:
    """Returns (blocked_tiles, tiles_by_class)."""
    blocked: set[tuple[int, int]] = set()
    by_class: dict[str, set] = {}
    for typ, x, y, yaw in objects:
        if typ < 0 or typ not in models:
            continue
        m = models[typ]
        (x0, y0), (x1, y1) = m["min"], m["max"]
        s = 1.0
        # yaw is always 0 for World7; support 90-degree steps just in case
        yawd = round(yaw) % 360
        corners = [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]
        rc = []
        for cx, cy in corners:
            if yawd == 90:
                cx, cy = -cy, cx
            elif yawd == 180:
                cx, cy = -cx, -cy
            elif yawd == 270:
                cx, cy = cy, -cx
            rc.append((cx * s, cy * s))
        wx0 = x + min(c[0] for c in rc)
        wx1 = x + max(c[0] for c in rc)
        wy0 = y + min(c[1] for c in rc)
        wy1 = y + max(c[1] for c in rc)
        t0x, t1x = int(wx0 // 100), int(wx1 // 100)
        t0y, t1y = int(wy0 // 100), int(wy1 // 100)
        k = klass(typ)
        for ty in range(max(0, t0y), min(256, t1y + 1)):
            for tx in range(max(0, t0x), min(256, t1x + 1)):
                # require meaningful overlap (>15% of tile) to avoid fattening
                ox = min(wx1, (tx + 1) * 100) - max(wx0, tx * 100)
                oy = min(wy1, (ty + 1) * 100) - max(wy0, ty * 100)
                if ox > 15 and oy > 15:
                    by_class.setdefault(k, set()).add((tx, ty))
                    if k != "decor":
                        blocked.add((tx, ty))
    return blocked, by_class


# --- official doors / boxes / safezones (kept from rounds 1-2 docs) ----------
DOORS = [
    (16, 37, 16, 39), (16, 55, 16, 57), (16, 73, 16, 75), (16, 89, 18, 92),
    (23, 37, 24, 39), (23, 55, 24, 57), (23, 71, 24, 74), (23, 88, 24, 91),
    (41, 37, 41, 39), (41, 55, 41, 58), (41, 69, 41, 72), (41, 79, 41, 82),
    (60, 73, 64, 75),
]
DOOR_NAMES = ["Yeti", "Ice", "Cyclops", "PoisonBull", "Gorgon", "Shadow", "Devil",
              "DeathCow", "Bahamut", "LizardKing", "IronWheel", "Mutant", "Drakan"]
BOXES = [
    (9, 35, 15, 41), (9, 53, 15, 59), (9, 71, 15, 77), (9, 88, 15, 94),
    (25, 35, 31, 41), (25, 53, 31, 59), (25, 70, 31, 76), (25, 87, 31, 93),
    (45, 35, 51, 41), (45, 54, 51, 60), (45, 69, 51, 74), (45, 78, 51, 84),
    (65, 71, 71, 77),
]
PLAZA = (65, 43, 10)   # x, y, Chebyshev radius (pad center is a monument)
WARP = (102, 116, 3)
SPAWN = (54, 39)       # in-game walkable reference tile (owner's screenshot spot)
BOUNDARY = 2           # blocked world-boundary frame width


def build_walls(models, objects, doors=DOORS):
    """Visual-driven walls: everything open except object footprints and a
    2-tile world boundary; then the 13 official door rects are punched open
    (exactly what ArenaCageDoors.PunchTerrain does server-side, so the shipped
    client ATT matches the server blob byte for byte); safezones stamped on
    walkable pad tiles. Returns (walls, blocking set, per-door report)."""
    blocked, _by = object_tiles(models, objects)
    walls = bytearray(65536)
    for (tx, ty) in blocked:
        walls[tx + (ty << 8)] = 4  # Blocked/NOMOVE: rail-accurate
    # world boundary frame
    for i in range(256):
        for b in range(BOUNDARY):
            walls[i + (b << 8)] = 12
            walls[i + ((255 - b) << 8)] = 12
            walls[b + (i << 8)] = 12
            walls[(255 - b) + (i << 8)] = 12
    # official doors: punch open (PunchTerrain parity); report pre-punch conflicts
    door_report = {}
    for name, (a, b, c, d) in zip(DOOR_NAMES, doors):
        tiles = [(x, y) for y in range(b, d + 1) for x in range(a, c + 1)]
        conflict = [t for t in tiles if t in blocked]
        for (x, y) in tiles:
            walls[x + (y << 8)] = 0
        door_report[name] = (tiles, conflict)

    # safezones: v=1 on walkable tiles of the pads
    def stamp(cx, cy, r):
        n = 0
        for y in range(max(0, cy - r), min(256, cy + r + 1)):
            for x in range(max(0, cx - r), min(256, cx + r + 1)):
                if walls[x + (y << 8)] == 0:
                    walls[x + (y << 8)] = 1
                    n += 1
        return n
    n1 = stamp(*PLAZA)
    n2 = stamp(*WARP)
    print(f"safezone stamped: plaza {n1} tiles, warp {n2} tiles")
    return bytes(walls), blocked, door_report


def load_ground() -> tuple[bytes, bytes, bytes]:
    dec = muatt.map_decrypt((WORLD / "EncTerrain7.map").read_bytes())
    return dec[2:2 + 65536], dec[2 + 65536:2 + 131072], dec[2 + 131072:2 + 196608]


def read_att_current() -> bytes:
    _h, w = muatt.read_att(WORLD / "EncTerrain7.att")
    return w


def draw_map(scale: int) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("RGB", (256 * scale, 256 * scale), (0, 0, 0))
    dr = ImageDraw.Draw(img, "RGBA")
    return img, dr


def ground_picture(dr, layer1, layer2, alpha, heights, light, scale) -> None:
    tex_colors = {
        0: (168, 146, 104),   # sand/dirt (World7 TileGrass01)
        1: (120, 140, 70),    # grass02
        2: (150, 150, 150),   # ground01
        3: (140, 140, 145),   # ground02
        4: (170, 165, 140),   # ground03
        5: (70, 110, 160),    # water
        6: (140, 105, 60),    # wood
        7: (130, 130, 135),   # rock01
        8: (135, 135, 142),   # rock02 (paved)
        9: (120, 120, 128),   # rock03
        10: (110, 110, 118),  # rock04
        11: (100, 100, 110),  # rock05
    }
    for ty in range(256):
        for tx in range(256):
            i = tx + (ty << 8)
            l1 = layer1[i]
            l2v = layer2[i]
            a = alpha[i] / 255.0
            c1 = tex_colors.get(l1, (150, 150, 150))
            c2 = tex_colors.get(l2v, c1) if l2v != 255 else c1
            r = c1[0] + (c2[0] - c1[0]) * a
            g = c1[1] + (c2[1] - c1[1]) * a
            b = c1[2] + (c2[2] - c1[2]) * a
            lr, lg, lb = light[i]
            lum = (lr + lg + lb) / (3.0 * 255.0)
            k = 0.45 + 0.75 * lum
            h = heights[i]
            shade = 0.92 + 0.16 * (h / 255.0)
            kk = k * shade
            dr.rectangle([tx * scale, ty * scale, (tx + 1) * scale - 1, (ty + 1) * scale - 1],
                         fill=(int(r * kk), int(g * kk), int(b * kk)))


def render_panel(objects, blocked, by_class, walls, heights, light, layer1, layer2, alpha,
                 scale=6, mode="visual") -> Image.Image:
    img, dr = draw_map(scale)
    if mode == "visual":
        ground_picture(dr, layer1, layer2, alpha, heights, light, scale)
        for k, tiles in by_class.items():
            r, g, b = CLASS_COLOR[k]
            for (tx, ty) in tiles:
                dr.rectangle([tx * scale + 1, ty * scale + 1, (tx + 1) * scale - 2, (ty + 1) * scale - 2],
                             fill=(r, g, b, 150))
    elif mode == "att":
        ground_picture(dr, layer1, layer2, alpha, heights, light, scale)
        for ty in range(256):
            for tx in range(256):
                v = walls[tx + (ty << 8)]
                px, py = tx * scale, ty * scale
                if v in (0, 1):
                    dr.rectangle([px + 1, py + 1, px + scale - 2, py + scale - 2], fill=(60, 220, 60, 130))
                elif v & 1:
                    dr.rectangle([px + 1, py + 1, px + scale - 2, py + scale - 2], fill=(240, 220, 60, 130))
                else:
                    dr.rectangle([px + 1, py + 1, px + scale - 2, py + scale - 2], fill=(210, 40, 40, 130))
    elif mode == "diff":
        ground_picture(dr, layer1, layer2, alpha, heights, light, scale)
        dr.rectangle([0, 0, 256 * scale, 256 * scale], fill=(0, 0, 0, 110))
        att_blocked = set()
        att_walk = set()
        for ty in range(256):
            for tx in range(256):
                v = walls[tx + (ty << 8)]
                (att_walk if v in (0, 1) else att_blocked).add((tx, ty))
        for (tx, ty) in blocked - att_blocked:
            dr.rectangle([tx * scale + 1, ty * scale + 1, (tx + 1) * scale - 2, (ty + 1) * scale - 2],
                         fill=(255, 120, 40, 220), outline=(255, 255, 255, 255))
        for (tx, ty) in att_blocked - blocked:
            dr.rectangle([tx * scale + 1, ty * scale + 1, (tx + 1) * scale - 2, (ty + 1) * scale - 2],
                         fill=(40, 120, 255, 220))
        for (tx, ty) in (blocked & att_blocked):
            dr.rectangle([tx * scale + 1, ty * scale + 1, (tx + 1) * scale - 2, (ty + 1) * scale - 2],
                         fill=(120, 120, 120, 160))
        for (tx, ty) in att_walk:
            dr.rectangle([tx * scale + 1, ty * scale + 1, (tx + 1) * scale - 2, (ty + 1) * scale - 2],
                         outline=(60, 255, 60, 90))
    # labels every 16 tiles
    for ty in range(0, 256, 16):
        for tx in range(0, 256, 16):
            dr.text((tx * scale + 2, ty * scale + 1), f"{tx},{ty}", fill=(255, 255, 255, 200))
    return img


def att_blocked_set(walls) -> set:
    return {(tx, ty) for ty in range(256) for tx in range(256)
            if walls[tx + (ty << 8)] not in (0, 1)}


def check_cages(walls, door_report) -> bool:
    """Flood-fill from the spawn tile and verify each pen + door connectivity."""
    from collections import deque
    ok = True
    seen = set()
    q = deque([SPAWN])
    while q:
        x, y = q.popleft()
        if not (0 <= x < 256 and 0 <= y < 256) or (x, y) in seen:
            continue
        if walls[x + (y << 8)] not in (0, 1):
            continue
        seen.add((x, y))
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            q.append((x + dx, y + dy))
    print(f"reachable from spawn {SPAWN}: {len(seen)} tiles")
    for name, (a, b, c, d) in zip(DOOR_NAMES, BOXES):
        tiles = [(x, y) for y in range(b, d + 1) for x in range(a, c + 1)]
        reach = sum(1 for t in tiles if t in seen)
        status = "OK" if reach == len(tiles) else "PARTIAL"
        if reach != len(tiles):
            ok = False
        print(f"  cage {name:12s}: {reach}/{len(tiles)} reachable [{status}]")
    for name, (tiles, conflict) in door_report.items():
        note = f" ({len(conflict)} thin-object tiles punched: {conflict[:4]})" if conflict else ""
        print(f"  door {name:12s}: {len(tiles)} tiles open{note}")
    return ok


def write_att(walls: bytes) -> None:
    """Write client EncTerrain7.att (encrypted, hdr [0,7,255,255]) + Terrain7.att plain."""
    plain = bytes([0, 7, 255, 255]) + walls
    (WORLD / "EncTerrain7.att").write_bytes(muatt.map_encrypt(muatt.bux(plain)))
    (WORLD / "Terrain7.att").write_bytes(bytes([0, 255, 255]) + walls)
    print(f"wrote {WORLD / 'EncTerrain7.att'} ({len(plain)} plain bytes)")
    print(f"wrote {WORLD / 'Terrain7.att'}")


def main() -> None:
    mode = sys.argv[1] if len(sys.argv) > 1 else "analyze"
    models = load_models()
    objects = load_objects()
    blocked, by_class = object_tiles(models, objects)
    layer1, layer2, alpha = load_ground()
    ozb = (WORLD / "TerrainHeight.OZB").read_bytes()
    heights = ozb[1084:1084 + 65536]
    import io, re
    def read_ozj(data):
        for off in [m.start() for m in re.finditer(b"\xff\xd8\xff", data)]:
            try:
                img = Image.open(io.BytesIO(data[off:]))
                img.load()
                return img.convert("RGB")
            except Exception:
                continue
        raise ValueError
    light_img = read_ozj((WORLD / "TerrainLight.OZJ").read_bytes()).resize((256, 256))
    light = list(light_img.getdata())

    if mode == "build":
        walls, _blocked_new, door_report = build_walls(models, objects)
        check_cages(walls, door_report)
        write_att(walls)
        tag = "after"
    else:
        walls = read_att_current()
        tag = "before" if mode == "analyze" else "after"

    cur_blocked = att_blocked_set(walls)
    n_att_block = len(cur_blocked)
    holes = blocked - cur_blocked
    invisible = cur_blocked - blocked
    print(f"object-blocked tiles: {len(blocked)}  att-blocked: {n_att_block}")
    print(f"  visual-blocked but ATT-walkable (holes): {len(holes)}")
    print(f"  ATT-blocked but visually open (invisible walls): {len(invisible)}")

    s = 5
    panels = [
        render_panel(objects, blocked, by_class, walls, heights, light, layer1, layer2, alpha, s, "visual"),
        render_panel(objects, blocked, by_class, walls, heights, light, layer1, layer2, alpha, s, "att"),
        render_panel(objects, blocked, by_class, walls, heights, light, layer1, layer2, alpha, s, "diff"),
    ]
    W = 256 * s
    total = Image.new("RGB", (W, sum(p.height for p in panels) + 20 * len(panels)), (20, 20, 20))
    y = 0
    for p in panels:
        total.paste(p, (0, y))
        y += p.height + 20
    outp = OUT / f"arena_round3_{tag}.png"
    total.save(outp)
    print("wrote", outp)


if __name__ == "__main__":
    main()
