"""Diagnostic overlay: render World7 VISUAL (ground textures + light + height +
objects) the way the client draws it, then paint the ATT walkability on top.

green wash = ATT walkable (value 0/1), red wash = ATT blocked (NOMOVE 4 /
NOGROUND 8 / anything else), yellow outline = safezone pad (bit 0).
Magenta crosses = objects from EncTerrain7.obj (marker at their tile).

This is intentionally independent of make_arena_minimap.py's void-filling:
the base picture must NOT be derived from the ATT, so divergences between the
visual and the walkability layer become visible.
"""
from __future__ import annotations

import io
import re
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

REF = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_arena-ref")
sys.path.insert(0, str(REF))
import muatt  # noqa: E402

WORLD = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_wt-arena3\src\bin\Data\World7")
OUT_DIR = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_arena-ref")

TILE_SLOTS = [
    "TileGrass01", "TileGrass02", "TileGround01", "TileGround02",
    "TileGround03", "TileWater01", "TileWood01", "TileRock01",
    "TileRock02", "TileRock03", "TileRock04", "TileRock05",
]


def read_ozj_rgb(data: bytes) -> Image.Image:
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
        try:
            img = read_ozj_rgb((WORLD / f"{name}.OZJ").read_bytes()).resize((16, 16))
            px = list(img.getdata())
            n = len(px)
            colors.append(tuple(sum(c[i] for c in px) // n for i in range(3)))
        except OSError:
            colors.append((200, 190, 160))
    return colors


def parse_obj(path: Path) -> list[tuple[int, float, float, float]]:
    dec = muatt.map_decrypt(path.read_bytes())
    count = struct.unpack_from("<h", dec, 2)[0]
    out = []
    off = 4
    for _ in range(count):
        (typ,) = struct.unpack_from("<h", dec, off)
        x, y, z = struct.unpack_from("<fff", dec, off + 2)
        off += 2 + 12 + 12 + 4
        out.append((typ, x, y, z))
    assert off == len(dec), (off, len(dec))
    return out


def load_light_and_height() -> tuple[list[tuple[int, int, int]], bytes]:
    light_img = read_ozj_rgb((WORLD / "TerrainLight.OZJ").read_bytes()).resize((256, 256))
    light = list(light_img.getdata())
    ozb = (WORLD / "TerrainHeight.OZB").read_bytes()
    heights = ozb[1084:1084 + 65536]
    if len(heights) < 65536:
        raise SystemExit("OZB too short")
    return light, heights


def render(walls: bytes, tag: str, draw_objects: bool = True) -> Image.Image:
    dec = muatt.map_decrypt((WORLD / "EncTerrain7.map").read_bytes())
    layer1 = dec[2:2 + 65536]
    layer2 = dec[2 + 65536:2 + 131072]
    alpha = dec[2 + 131072:2 + 196608]
    light, heights = load_light_and_height()
    tex = tile_average_colors()
    objects = parse_obj(WORLD / "EncTerrain7.obj") if draw_objects else []

    S = 8
    nx = 256 * S
    img = Image.new("RGB", (nx, nx), (0, 0, 0))
    dr = ImageDraw.Draw(img, "RGBA")

    # --- base visual: ground textures * light * height ------------------
    for ty in range(256):
        for tx in range(256):
            i = tx + (ty << 8)
            l1 = layer1[i] % 12
            l2v = layer2[i]
            a = alpha[i] / 255.0
            c1 = tex[l1]
            c2 = tex[l2v % 12] if l2v != 255 else c1
            r = c1[0] + (c2[0] - c1[0]) * a
            g = c1[1] + (c2[1] - c1[1]) * a
            b = c1[2] + (c2[2] - c1[2]) * a
            lr, lg, lb = light[i]
            lum = (lr + lg + lb) / (3.0 * 255.0)
            h = heights[i]
            shade = 0.9 + 0.2 * (h / 255.0)
            k = lum * shade
            dr.rectangle([tx * S, ty * S, tx * S + S - 1, ty * S + S - 1],
                         fill=(int(r * k), int(g * k), int(b * k)))

    # --- objects: marker per tile ---------------------------------------
    if objects:
        hist: dict[int, int] = {}
        for typ, x, y, _z in objects:
            hist[typ] = hist.get(typ, 0) + 1
        print(f"[{tag}] objects: {len(objects)} total, types: {dict(sorted(hist.items()))}")
        for typ, x, y, _z in objects:
            tx, ty = int(x / 100.0), int(y / 100.0)
            if not (0 <= tx < 256 and 0 <= ty < 256):
                continue
            px, py = tx * S + S // 2, ty * S + S // 2
            # color cycles per type so fence rows / groups stay recognizable
            hue = (typ * 47) % 360
            import colorsys
            r, g, b = (int(c * 255) for c in colorsys.hsv_to_rgb(hue / 360.0, 1.0, 1.0))
            dr.line([px - 3, py - 3, px + 3, py + 3], fill=(r, g, b, 255), width=1)
            dr.line([px - 3, py + 3, px + 3, py - 3], fill=(r, g, b, 255), width=1)

    # --- ATT overlay ------------------------------------------------------
    for ty in range(256):
        for tx in range(256):
            v = walls[tx + (ty << 8)]
            px, py = tx * S, ty * S
            walk = v in (0, 1)
            if walk:
                dr.rectangle([px, py, px + S - 1, py + S - 1], fill=(0, 255, 0, 90))
            else:
                dr.rectangle([px, py, px + S - 1, py + S - 1], fill=(255, 0, 0, 90))
            if v & 1:
                dr.rectangle([px, py, px + S - 1, py + S - 1], outline=(255, 255, 0, 255), width=1)
            # grid every 8 tiles for coordinate reading
            if tx % 16 == 0 and ty % 16 == 0:
                dr.text((px + 2, py + 1), f"{tx},{ty}", fill=(255, 255, 255, 220))

    out = OUT_DIR / f"arena_overlay_{tag}.png"
    img.save(out)
    print("wrote", out)
    return img


def main() -> None:
    _h, cur = muatt.read_att(WORLD / "EncTerrain7.att")
    render(cur, "before_currentATT")
    _h, bm = muatt.read_att(str(REF / "EncTerrain7.att.bloodmu"))
    render(bm, "before_bloodmuATT", draw_objects=False)


if __name__ == "__main__":
    main()
