"""Build Data/World7/mini_map.OZT for Stadium TAB + corner minimap.

Renders an actual picture of the map (the way every retail minimap looks):
per-tile terrain texture colors from EncTerrain7.map layers blended by the
mapping alpha, shaded by TerrainLight.OZJ and TerrainHeight.OZB, with fence
rails drawn dark, void areas dark-filled and the safezone pads tinted green.
The sheet is FULLY OPAQUE: CNewUIMiniMap paints an 85% black underlay, so any
alpha-0 pixel lets the blackened world bleed through.

Terrain source is the shipped EncTerrain7.att authoring (plain copy in
Terrain7.att) which already carries the official doors and safezone pads
(identical to the OpenMU update-187 blob), so minimap, client collision and
server collision all show the same picture.
"""
from __future__ import annotations

import io
import re
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
WORLD = ROOT / "src" / "bin" / "Data" / "World7"
ATT = WORLD / "Terrain7.att"
OUT = WORLD / "mini_map.OZT"
PREVIEW = Path(r"C:\Users\joaop\AppData\Local\Temp\arena-minimap-preview.png")

TILE_SLOTS = [
    "TileGrass01", "TileGrass02", "TileGround01", "TileGround02",
    "TileGround03", "TileWater01", "TileWood01", "TileRock01",
    "TileRock02", "TileRock03", "TileRock04", "TileRock05",
]

MAP_KEY = bytes([0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
                 0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2])

# RGB overlays.
FENCE = (56, 46, 38)
FENCE_SPAN = (78, 66, 54)
VOID = (24, 22, 27)
SAFE_TINT = (110, 190, 90)


def map_decrypt(src: bytes) -> bytes:
    """Same chained cipher the client uses for EncTerrain* files."""
    dst = bytearray(len(src))
    key = 0x5E
    for i, s in enumerate(src):
        dst[i] = ((s ^ MAP_KEY[i % 16]) - key) & 0xFF
        key = (s + 0x3D) & 0xFF
    return bytes(dst)


def read_ozj_rgb(data: bytes) -> Image.Image:
    """Decode a (nested-SOI) OZJ/JPEG blob into an RGB image."""
    sois = [m.start() for m in re.finditer(b"\xff\xd8\xff", data)]
    for off in sois:
        try:
            img = Image.open(io.BytesIO(data[off:]))
            img.load()
            return img.convert("RGB")
        except Exception:
            continue
    raise ValueError("no decodable JPEG stream in OZJ")


def tile_average_colors() -> list[tuple[int, int, int]]:
    colors = []
    for name in TILE_SLOTS:
        path = WORLD / f"{name}.OZJ"
        try:
            img = read_ozj_rgb(path.read_bytes()).resize((16, 16))
            px = list(img.getdata())
            n = len(px)
            avg = tuple(sum(c[i] for c in px) // n for i in range(3))
        except OSError:
            avg = (200, 190, 160)
        colors.append(avg)
    return colors


def main() -> None:
    att = ATT.read_bytes()
    if len(att) < 3 + 256 * 256:
        raise SystemExit(f"Terrain7.att too short: {len(att)}")
    walls = att[3:]

    # Terrain mapping layers (same decrypt chain as EncTerrain*.map).
    enc_map = (WORLD / "EncTerrain7.map").read_bytes()
    dec = map_decrypt(enc_map)
    layer1 = dec[2:2 + 65536]
    layer2 = dec[2 + 65536:2 + 131072]
    alpha = dec[2 + 131072:2 + 196608]

    light_img = read_ozj_rgb((WORLD / "TerrainLight.OZJ").read_bytes()).resize((256, 256))
    light = list(light_img.getdata())

    heights = (WORLD / "TerrainHeight.OZB").read_bytes()[1084:1084 + 65536]
    tex_colors = tile_average_colors()

    def walkable(x: int, y: int) -> bool:
        if not (0 <= x < 256 and 0 <= y < 256):
            return False
        v = walls[x + (y << 8)]
        return v in (0, 1)

    def fenced(x: int, y: int) -> bool:
        v = walls[x + (y << 8)]
        return (v & 4) != 0

    nx = ny = 1024
    scale = nx // 256
    pixels = bytearray(nx * ny * 4)
    rgba_preview = bytearray(nx * ny * 4)

    for ty in range(256):
        for tx in range(256):
            v = walls[tx + (ty << 8)]
            l1 = layer1[tx + (ty << 8)] % 12
            l2v = layer2[tx + (ty << 8)]
            a = alpha[tx + (ty << 8)] / 255.0
            c1 = tex_colors[l1]
            c2 = tex_colors[l2v % 12] if l2v != 255 else c1
            r = int(c1[0] + (c2[0] - c1[0]) * a)
            g = int(c1[1] + (c2[1] - c1[1]) * a)
            b = int(c1[2] + (c2[2] - c1[2]) * a)

            lr, lg, lb = light[(ty << 8) + tx]
            lum = (lr + lg + lb) / (3.0 * 255.0)
            lum = 0.55 + 0.45 * lum
            h = heights[(ty << 8) + tx]
            shade = 0.9 + 0.2 * (h / 255.0)
            k = lum * shade
            r, g, b = int(r * k), int(g * k), int(b * k)

            if v == 1:
                # Safezone pad: green tint keeps the pad readable.
                r = (r + SAFE_TINT[0]) // 2
                g = (g + SAFE_TINT[1]) // 2
                b = (b + SAFE_TINT[2]) // 2

            if fenced(tx, ty):
                near = any(walkable(tx + dx, ty + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
                r, g, b = FENCE if near else FENCE_SPAN
            elif not walkable(tx, ty):
                near = any(walkable(tx + dx, ty + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
                if not near:
                    r, g, b = VOID

            bgra = bytes((max(0, min(255, b)), max(0, min(255, g)), max(0, min(255, r)), 255))
            # MiniMap UV: U = map Y, V = map X (NewUIMiniMap Tx/Ty swap).
            for oy in range(scale):
                for ox in range(scale):
                    px = (ty * scale) + ox
                    py = (tx * scale) + oy
                    off = (py * nx + px) * 4
                    pixels[off:off + 4] = bgra
                    rgba_preview[off:off + 4] = bytes((bgra[2], bgra[1], bgra[0], 255))

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
        img = Image.frombytes("RGBA", (nx, ny), bytes(rgba_preview))
        img.save(PREVIEW)
        print("preview", PREVIEW)
    except Exception as exc:  # noqa: BLE001
        print("preview skipped:", exc)

    opaque = sum(1 for i in range(3, len(pixels), 4) if pixels[i] > 0)
    print(f"wrote {OUT} len={len(blob)} opaque_pct={100 * opaque / (nx * ny):.2f}")


if __name__ == "__main__":
    main()
