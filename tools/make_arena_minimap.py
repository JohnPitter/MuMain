"""Build Data/World7/mini_map.OZT for Stadium TAB + corner minimap.

Official S6 never shipped a Stadium minimap. Round 4 generated a 1024x1024
OpenTga blob from Terrain7.att but painted every tile opaque (void = dark
NoMove). TAB draws that quad over ~640x430, so the screen became a black sheet.

Match Icarus (World11): unused tiles stay alpha 0; only the stadium footprint
is visible. Colors follow update 162: teal plaza/warp, sand PvP halls, terracotta
hunting cages, stone walls adjacent to walkable tiles.
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
OUT = ROOT / "src" / "bin" / "Data" / "World7" / "mini_map.OZT"
PREVIEW = Path(r"C:\Users\joaop\AppData\Local\Temp\arena-minimap-preview.png")

# BGRA (OpenTga swaps R/B on upload)
VOID = (0, 0, 0, 0)
PVP = (0x6E, 0xB4, 0xDC, 0xFF)       # sand RGB(220,180,110)
SAFE = (0xBE, 0xC8, 0x46, 0xFF)      # teal RGB(70,200,190)
PVM = (0x3C, 0x5A, 0xD2, 0xFF)       # terracotta RGB(210,90,60)
WALL = (0x5A, 0x62, 0x6E, 0xE6)      # stone, slight alpha
WALL_EDGE = (0x28, 0x2C, 0x32, 0xFF)

FENCE_SEALS = [
    (35, 33, 35, 45), (35, 51, 35, 63), (35, 69, 35, 97),
    (36, 33, 36, 45), (36, 51, 36, 63), (36, 69, 36, 81), (36, 85, 36, 97),
    (41, 32, 41, 36), (41, 40, 41, 54), (41, 59, 41, 68), (41, 73, 41, 78), (41, 83, 41, 93),
    (54, 33, 54, 45), (54, 51, 54, 63), (54, 68, 54, 97),
    (16, 88, 18, 88), (16, 93, 18, 93),
    (60, 69, 63, 72), (60, 76, 63, 88), (61, 62, 61, 73), (64, 69, 71, 69), (64, 81, 71, 81),
    (7, 34, 7, 43), (7, 52, 7, 61), (6, 70, 7, 79), (6, 87, 7, 91), (8, 95, 15, 96),
]
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
PLAZA = (65, 43, 10)
WARP = (102, 116, 3)


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


def punch(att: bytearray) -> None:
    def idx(x, y):
        return 3 + x + (y << 8)

    for x1, y1, x2, y2 in FENCE_SEALS:
        for x in range(x1, x2 + 1):
            for y in range(y1, y2 + 1):
                if is_door(x, y):
                    continue
                i = idx(x, y)
                v = att[i]
                if v == 5 or (v & 8):
                    continue
                att[i] = 4
    for x1, y1, x2, y2 in DOORS:
        for x in range(x1, x2 + 1):
            for y in range(y1, y2 + 1):
                att[idx(x, y)] = 0
    # Clear leftover campus safezone, then stamp 162 pads.
    for y in range(25, 136):
        for x in range(0, 121):
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


def classify(att: bytes, x: int, y: int, walk: set[tuple[int, int]]) -> bytes:
    v = att[3 + x + (y << 8)]
    if v in (0, 1):
        if is_safe(x, y):
            return bytes(SAFE)
        if in_rects(HUNTING, x, y):
            return bytes(PVM)
        return bytes(PVP)
    near = False
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            if (x + dx, y + dy) in walk:
                near = True
                break
        if near:
            break
    if not near:
        return bytes(VOID)
    # Edge of the footprint: darker outline.
    walk_n = sum(
        1 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
        if (x + dx, y + dy) in walk
    )
    return bytes(WALL_EDGE if walk_n else WALL)


def main() -> None:
    att = bytearray(ATT.read_bytes())
    if len(att) < 3 + 256 * 256:
        raise SystemExit(f"Terrain7.att too short: {len(att)}")
    punch(att)
    walk = {
        (x, y)
        for y in range(256)
        for x in range(256)
        if att[3 + x + (y << 8)] in (0, 1)
    }
    nx = ny = 1024
    scale = nx // 256
    pixels = bytearray(nx * ny * 4)
    rgba_preview = bytearray(nx * ny * 4)
    for ty in range(256):
        for tx in range(256):
            bgra = classify(att, tx, ty, walk)
            # MiniMap UV: U = map Y, V = map X (NewUIMiniMap Tx/Ty swap).
            for oy in range(scale):
                for ox in range(scale):
                    px = (ty * scale) + ox
                    py = (tx * scale) + oy
                    off = (py * nx + px) * 4
                    pixels[off : off + 4] = bgra
                    # preview in RGBA
                    rgba_preview[off : off + 4] = bytes((bgra[2], bgra[1], bgra[0], bgra[3]))

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
    samples = [(65, 43, "plaza"), (46, 64, "corridor"), (12, 38, "yeti"), (7, 38, "yeti_back"), (200, 200, "void")]
    for x, y, name in samples:
        bgra = classify(att, x, y, walk)
        print(f"  {name:10} ({x},{y}) BGRA={bgra.hex()} att={att[3 + x + (y << 8)]}")


if __name__ == "__main__":
    main()
