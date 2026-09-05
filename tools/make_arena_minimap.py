"""Build Data/World7/mini_map.OZT for Stadium TAB + corner minimap.

Official S6 never shipped a Stadium minimap. The sheet must be FULLY OPAQUE:
CNewUIMiniMap::Render paints an 85% black quad under the map image, so any
alpha-0 pixel lets the blackened world bleed through (the "mapa escuro / chão
some" report). Retail sheets (World1) are opaque with a dark void fill; this
generator follows that convention.

Source of truth is the shipped EncTerrain7.att authoring (plain copy in
Terrain7.att): fence rails are NOMOVE exactly where the .obj shows rails and
every visual gap stays walkable. The generator only applies the official cage
doors and the safezone pads (plaza 65,43 r10 + warp 102,116 r3), mirroring
OpenMU ArenaCageDoors / update 187.

Colors are terrain-driven: sand ground shaded by TerrainHeight.OZB, dark brown
fence lines, darker void fill, yellow safezone pads, distinct cage floors.
"""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ATT = ROOT / "src" / "bin" / "Data" / "World7" / "Terrain7.att"
if not ATT.exists():
    ATT = Path(
        r"C:\Users\joaop\Desenvolvimento\openmu"
        r"\OpenMU\src\Persistence\Initialization\Resources\Terrain7.att"
    )
OZB = ROOT / "src" / "bin" / "Data" / "World7" / "TerrainHeight.OZB"
OUT = ROOT / "src" / "bin" / "Data" / "World7" / "mini_map.OZT"
PREVIEW = Path(r"C:\Users\joaop\AppData\Local\Temp\arena-minimap-preview.png")

DOORS = [
    (16, 37, 16, 39), (16, 55, 16, 57), (16, 73, 16, 75), (16, 89, 18, 92),
    (23, 37, 24, 39), (23, 55, 24, 57), (23, 71, 24, 74), (23, 88, 24, 91),
    (41, 37, 41, 39), (41, 55, 41, 58), (41, 69, 41, 72), (41, 79, 41, 82),
    (60, 73, 64, 75),
]
HUNTING = [
    (9, 35, 15, 41), (9, 53, 15, 59), (9, 71, 15, 77), (9, 88, 15, 94),
    (25, 35, 31, 41), (25, 53, 31, 59), (25, 70, 31, 76), (25, 87, 31, 93),
    (45, 35, 51, 41), (45, 54, 51, 60), (45, 69, 51, 74), (45, 78, 51, 84),
    (65, 71, 71, 77),
]
CAMPUS = (0, 25, 120, 135)
PLAZA = (65, 43, 10)
WARP = (102, 116, 3)

# RGB palettes (converted to BGRA on write).
GROUND = (213, 199, 162)
GROUND_ALT = (203, 188, 150)
CAGE = (199, 172, 130)
CAGE_ALT = (189, 161, 119)
SAFE = (140, 205, 105)
SAFE_ALT = (128, 194, 94)
FENCE = (74, 62, 50)
FENCE_EDGE = (52, 43, 35)
VOID = (30, 28, 33)


def in_rects(rects, x, y) -> bool:
    for x1, y1, x2, y2 in rects:
        if x1 <= x <= x2 and y1 <= y <= y2:
            return True
    return False


def chebyshev(x, y, cx, cy, r) -> bool:
    return abs(x - cx) <= r and abs(y - cy) <= r


def is_door(x, y) -> bool:
    return in_rects(DOORS, x, y)


def is_safe(x, y) -> bool:
    if is_door(x, y) or in_rects(HUNTING, x, y):
        return False
    return chebyshev(x, y, *PLAZA) or chebyshev(x, y, *WARP)


def official_terrain(att: bytearray) -> None:
    """Punch doors and stamp safezone pads (update 187 semantics)."""
    def idx(x, y):
        return 3 + x + (y << 8)

    for x1, y1, x2, y2 in DOORS:
        for x in range(x1, x2 + 1):
            for y in range(y1, y2 + 1):
                att[idx(x, y)] = 0

    x1, y1, x2, y2 = CAMPUS
    for y in range(y1, y2 + 1):
        for x in range(x1, x2 + 1):
            i = idx(x, y)
            if att[i] == 1 and not is_safe(x, y):
                att[i] = 0
    for cx, cy, r in (PLAZA, WARP):
        for x in range(cx - r, cx + r + 1):
            for y in range(cy - r, cy + r + 1):
                if not (0 <= x < 256 and 0 <= y < 256):
                    continue
                if is_door(x, y) or in_rects(HUNTING, x, y):
                    continue
                i = idx(x, y)
                if att[i] in (0, 1):
                    att[i] = 1


def walkable(att: bytes, x: int, y: int) -> bool:
    if not (0 <= x < 256 and 0 <= y < 256):
        return False
    v = att[3 + x + (y << 8)]
    return v in (0, 1)


def load_heights() -> list[int]:
    try:
        raw = OZB.read_bytes()
        # 4-byte size prefix + 1080-byte BMP header, then 256*256 height bytes.
        return list(raw[1084:1084 + 65536])
    except OSError:
        return [128] * 65536


def main() -> None:
    att = bytearray(ATT.read_bytes())
    if len(att) < 3 + 256 * 256:
        raise SystemExit(f"Terrain7.att too short: {len(att)}")
    official_terrain(att)
    heights = load_heights()

    nx = ny = 1024
    scale = nx // 256
    pixels = bytearray(nx * ny * 4)
    rgba_preview = bytearray(nx * ny * 4)

    for ty in range(256):
        for tx in range(256):
            v = att[3 + tx + (ty << 8)]
            walk_here = v in (0, 1)
            if walk_here:
                base = GROUND
                if in_rects(HUNTING, tx, ty):
                    base = CAGE
                if is_safe(tx, ty):
                    base = SAFE
                alt = ((tx + ty) & 1) == 0
                if alt:
                    base = (SAFE_ALT if base is SAFE else CAGE_ALT if base is CAGE else GROUND_ALT)
                    if base is GROUND_ALT:
                        base = GROUND_ALT
                h = heights[(ty << 8) + tx]
                shade = (h - 128) // 16  # -8..8
                r = max(0, min(255, base[0] + shade * 2))
                g = max(0, min(255, base[1] + shade * 2))
                b = max(0, min(255, base[2] + shade * 2))
                rgb = (r, g, b)
            else:
                nomove = (v & 4) != 0
                near = any(
                    walkable(att, tx + dx, ty + dy)
                    for dx in (-1, 0, 1)
                    for dy in (-1, 0, 1)
                )
                if nomove and near:
                    rgb = FENCE_EDGE
                elif nomove:
                    rgb = FENCE
                elif near:
                    rgb = FENCE_EDGE
                else:
                    rgb = VOID

            bgra = bytes((rgb[2], rgb[1], rgb[0], 255))
            # MiniMap UV: U = map Y, V = map X (NewUIMiniMap Tx/Ty swap).
            for oy in range(scale):
                for ox in range(scale):
                    px = (ty * scale) + ox
                    py = (tx * scale) + oy
                    off = (py * nx + px) * 4
                    pixels[off: off + 4] = bgra
                    rgba_preview[off: off + 4] = bytes((bgra[2], bgra[1], bgra[0], 255))

    header = bytearray(22)
    header[2] = 2
    header[6] = 2
    header[16:18] = (1024).to_bytes(2, "little")
    header[18:20] = (1024).to_bytes(2, "little")
    header[20] = 32
    header[21] = 8
    blob = bytes(header) + bytes(pixels) + (b"\x00" * 26)
    if len(blob) != 4_194_352:
        raise SystemExit(f"unexpected OZT size {len(blob)}")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(blob)

    extra = [
        ROOT / "src" / "build" / "Release" / "Data" / "World7" / "mini_map.OZT",
        Path(r"C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\World7\mini_map.OZT"),
        Path(r"C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\World7\mini_map.OZT"),
    ]
    for dest in extra:
        if dest.parent.exists():
            dest.write_bytes(blob)

    try:
        from PIL import Image

        img = Image.frombytes("RGBA", (nx, ny), bytes(rgba_preview))
        img.save(PREVIEW)
        print("preview", PREVIEW)
    except Exception as exc:  # noqa: BLE001
        print("preview skipped:", exc)

    opaque = sum(1 for i in range(3, len(pixels), 4) if pixels[i] > 0)
    print(f"wrote {OUT} len={len(blob)} opaque_pct={100 * opaque / (nx * ny):.2f}")
    samples = [(65, 43, "plaza"), (102, 116, "warp"), (46, 64, "corridor"),
               (12, 38, "yeti"), (16, 38, "yeti_door"), (54, 81, "rail_gap"),
               (200, 200, "void"), (7, 38, "west_gap")]
    for x, y, name in samples:
        print(f"  {name:10} ({x},{y}) att={att[3 + x + (y << 8)]}")


if __name__ == "__main__":
    main()
