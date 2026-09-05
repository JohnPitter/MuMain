"""Textured Stadium (World7) minimap: replaces the flat-color per-tile look.

Renders the terrain the way the retail Webzen OZT minimaps were made - from
the REAL ground textures: per tile, the Tile*.OZJ texture pointed to by
EncTerrain7.map layer 1 is blitted (full texture per tile, seamless), the
layer-2 texture is mixed in with the mapping alpha, and the result is shaded
by TerrainLight.OZJ x TerrainHeight.OZB. Rendered at 2048x2048 and downscaled
with Lanczos to 1024x1024.

Objects from EncTerrain7.obj are drawn as proportional dark shadows using the
REAL XZ footprints parsed from Data/Object7/ObjectNN.bmd (same parser as
_wt-arena3/tools/arena_bmd_footprints.py): pen fences show up as thin dark
lines, walls/towers as dark blocks - no diagnostic markers.

Packing is identical to make_arena_minimap.py: OZT (22-byte TGA-like header,
BGRA bottom-up, 32bpp), MiniMap UV swaps axes (U = map Y, V = map X), sheet
100% opaque (CNewUIMiniMap paints an 85% black underlay).
"""
from __future__ import annotations

import io
import math
import re
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
WORLD = ROOT / "src" / "bin" / "Data" / "World7"
OBJ_DIR = ROOT / "src" / "bin" / "Data" / "Object7"
OUT = WORLD / "mini_map.OZT"
TEMP = Path(r"C:\Users\joaop\AppData\Local\Temp")

TILE_SLOTS = [
    "TileGrass01", "TileGrass02", "TileGround01", "TileGround02",
    "TileGround03", "TileWater01", "TileWood01", "TileRock01",
    "TileRock02", "TileRock03", "TileRock04", "TileRock05",
]

MAP_KEY = bytes([0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
                 0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2])

RENDER = 2048          # terrain render size (8 px per tile)
FINAL = 1024           # shipped minimap size (4 px per tile)
PTILE_R = RENDER // 256
PTILE_F = FINAL // 256

SHADOW_RGB = (28, 25, 22)   # object shadow tone
VOID_RGB = (24, 22, 27)     # outside-the-world fill


def map_decrypt(src: bytes) -> bytes:
    """Chained cipher shared by EncTerrain* maps and encrypted BMD payloads."""
    dst = bytearray(len(src))
    key = 0x5E
    for i, s in enumerate(src):
        dst[i] = ((s ^ MAP_KEY[i % 16]) - key) & 0xFF
        key = (s + 0x3D) & 0xFF
    return bytes(dst)


def read_ozj_rgb(data: bytes) -> Image.Image:
    """Decode a (possibly nested-SOI) OZJ/JPEG blob into an RGB image."""
    for m in re.finditer(b"\xff\xd8\xff", data):
        try:
            img = Image.open(io.BytesIO(data[m.start():]))
            img.load()
            return img.convert("RGB")
        except Exception:
            continue
    raise ValueError("no decodable JPEG stream in OZJ")


def bmd_aabb(path: Path) -> tuple[float, float, float, float, float]:
    """Model-space AABB of an ObjectNN.bmd (v0x0C encrypted / v0x0A plain).

    Returns (minx, maxx, minz, maxz, height) in client units (100 units = 1 tile).
    """
    raw = path.read_bytes()
    if raw[:3] != b"BMD":
        raise ValueError(f"{path.name}: not a BMD")
    version = raw[3]
    if version == 0x0C:
        (enc_size,) = struct.unpack_from("<i", raw, 4)
        data = map_decrypt(raw[8:8 + enc_size])
        ptr = 0
    elif version == 0x0A:
        data = raw
        ptr = 4
    else:
        raise ValueError(f"{path.name}: version {version}")
    ptr += 32  # mesh name
    (num_meshs, _num_bones, _num_actions) = struct.unpack_from("<hhh", data, ptr)
    ptr += 6
    minx = miny = minz = 1e9
    maxx = maxy = maxz = -1e9
    for _ in range(num_meshs):
        nv, nn, nt, ntri, _tex = struct.unpack_from("<hhhhh", data, ptr)
        ptr += 10
        for _v in range(nv):
            x, y, z = struct.unpack_from("<fff", data, ptr + 4)
            minx, maxx = min(minx, x), max(maxx, x)
            miny, maxy = min(miny, y), max(maxy, y)
            minz, maxz = min(minz, z), max(maxz, z)
            ptr += 16
        ptr += nn * 20 + nt * 8 + ntri * 64 + 32
    return minx, maxx, minz, maxz, maxy - miny


def parse_obj(path: Path) -> list[tuple[int, float, float, float, float]]:
    """EncTerrain7.obj placements: (type, x, y, ay_yaw, scale)."""
    dec = map_decrypt(path.read_bytes())
    (count,) = struct.unpack_from("<h", dec, 2)
    out = []
    off = 4
    for _ in range(count):
        (typ,) = struct.unpack_from("<h", dec, off)
        x, y, _z = struct.unpack_from("<fff", dec, off + 2)
        _ax, ay, _az = struct.unpack_from("<fff", dec, off + 14)
        (scale,) = struct.unpack_from("<f", dec, off + 26)
        off += 30
        if typ < 0 or scale <= 0:      # terminator / garbage records
            continue
        out.append((typ, x, y, ay, scale))
    return out


def build_terrain(att_walls: bytes) -> Image.Image:
    """2048x2048 textured+shaded terrain, natural orientation (x right, y down)."""
    dec = map_decrypt((WORLD / "EncTerrain7.map").read_bytes())
    layer1 = np.frombuffer(dec[2:2 + 65536], dtype=np.uint8).reshape(256, 256)
    layer2 = np.frombuffer(dec[2 + 65536:2 + 131072], dtype=np.uint8).reshape(256, 256)
    alpha = np.frombuffer(dec[2 + 131072:2 + 196608], dtype=np.uint8).reshape(256, 256)

    # One 8x8 sample of each ground texture (full OZJ per tile; seamless).
    tex = np.zeros((12, PTILE_R, PTILE_R, 3), dtype=np.float32)
    for i, name in enumerate(TILE_SLOTS):
        img = read_ozj_rgb((WORLD / f"{name}.OZJ").read_bytes())
        tex[i] = np.asarray(img.resize((PTILE_R, PTILE_R), Image.LANCZOS), dtype=np.float32)

    l1 = layer1.astype(np.int32) % 12
    l2raw = layer2
    l2 = np.where(l2raw != 255, l2raw.astype(np.int32) % 12, l1)
    a = (alpha.astype(np.float32) / 255.0)[..., None, None, None]
    blend = tex[l1] * (1.0 - a) + tex[l2] * a                      # (256,256,8,8,3)
    nat = blend.transpose(0, 2, 1, 3, 4).reshape(RENDER, RENDER, 3)

    # Light x height shading (same response the flat renderer was tuned with).
    light = np.asarray(
        read_ozj_rgb((WORLD / "TerrainLight.OZJ").read_bytes()).resize((256, 256)),
        dtype=np.float32)[..., :3].mean(axis=2) / 255.0
    heights = np.frombuffer((WORLD / "TerrainHeight.OZB").read_bytes(),
                            dtype=np.uint8, count=65536, offset=1084).reshape(256, 256)
    k = (0.55 + 0.45 * light) * (0.9 + 0.2 * heights.astype(np.float32) / 255.0)
    k = np.repeat(np.repeat(k, PTILE_R, axis=0), PTILE_R, axis=1)
    nat *= k[..., None]

    # Void (outside the world geometry) goes near-black, like the official maps.
    walk = np.frombuffer(att_walls, dtype=np.uint8).reshape(256, 256)
    walk = np.isin(walk, (0, 1))
    padded = np.zeros((258, 258), dtype=bool)
    padded[1:257, 1:257] = walk
    near = np.zeros((256, 256), dtype=bool)
    for dy in (0, 1, 2):
        for dx in (0, 1, 2):
            near |= padded[dy:dy + 256, dx:dx + 256]
    void = ~near
    void = np.repeat(np.repeat(void, PTILE_R, axis=0), PTILE_R, axis=1)
    nat[void] = VOID_RGB

    return Image.fromarray(np.clip(nat, 0, 255).astype(np.uint8))


def draw_objects(img: Image.Image) -> Image.Image:
    """Proportional dark footprints (rotated quads) for every placed object."""
    bounds: dict[int, tuple[float, float, float, float, float] | None] = {}
    placements = parse_obj(WORLD / "EncTerrain7.obj")

    overlay = Image.new("RGBA", (FINAL, FINAL), (0, 0, 0, 0))
    dr = ImageDraw.Draw(overlay)
    drawn = skipped = 0

    items = []
    for typ, x, y, ay, scale in placements:
        if typ not in bounds:
            p = OBJ_DIR / f"Object{typ + 1:02d}.bmd"
            try:
                bounds[typ] = bmd_aabb(p)
            except Exception:
                bounds[typ] = None
        b = bounds[typ]
        if b is None:
            skipped += 1
            continue
        minx, maxx, minz, maxz, height = b
        ft_x = (maxx - minx) * scale / 100.0
        ft_z = (maxz - minz) * scale / 100.0
        if ft_x < 0.05 or ft_z < 0.05:   # degenerate (billboards) - skip
            skipped += 1
            continue
        items.append((height * scale, typ, x, y, ay, scale, minx, maxx, minz, maxz))

    items.sort(key=lambda it: it[0])          # tall objects drawn last (on top)
    for _h, typ, x, y, ay, scale, minx, maxx, minz, maxz in items:
        ang = math.radians(ay)
        cos_a, sin_a = math.cos(ang), math.sin(ang)
        corners = []
        for cx, cz in ((minx, minz), (maxx, minz), (maxx, maxz), (minx, maxz)):
            rx = (cx * cos_a - cz * sin_a) * scale
            rz = (cx * sin_a + cz * cos_a) * scale
            corners.append(((x + rx) / 100.0 * PTILE_F, (y + rz) / 100.0 * PTILE_F))

        e01 = math.dist(corners[0], corners[1])
        e12 = math.dist(corners[1], corners[2])
        alpha = int(min(215, 55 + 0.85 * _h))
        fill = SHADOW_RGB + (alpha,)
        if min(e01, e12) < 1.1:               # thinner than a pixel -> axis line
            if e01 <= e12:
                m1 = ((corners[0][0] + corners[1][0]) / 2, (corners[0][1] + corners[1][1]) / 2)
                m2 = ((corners[2][0] + corners[3][0]) / 2, (corners[2][1] + corners[3][1]) / 2)
            else:
                m1 = ((corners[1][0] + corners[2][0]) / 2, (corners[1][1] + corners[2][1]) / 2)
                m2 = ((corners[3][0] + corners[0][0]) / 2, (corners[3][1] + corners[0][1]) / 2)
            dr.line([m1, m2], fill=fill, width=max(1, int(round(min(e01, e12)))) or 1)
        else:
            dr.polygon(corners, fill=fill)
        drawn += 1

    out = Image.alpha_composite(img.convert("RGBA"), overlay).convert("RGB")
    print(f"objects: {drawn} drawn, {skipped} skipped")
    return out


def pack_ozt(nat: Image.Image) -> bytes:
    """OZT blob: client MiniMap swaps axes (U = map Y, V = map X), BGRA, opaque."""
    arr = np.asarray(nat, dtype=np.uint8)
    p = np.transpose(arr, (1, 0, 2))                       # P[py=mapX][px=mapY]
    bgra = np.empty((FINAL, FINAL, 4), dtype=np.uint8)
    bgra[..., 0] = p[..., 2]
    bgra[..., 1] = p[..., 1]
    bgra[..., 2] = p[..., 0]
    bgra[..., 3] = 255
    header = bytearray(22)
    header[2] = 2
    header[6] = 2
    header[16:18] = FINAL.to_bytes(2, "little")
    header[18:20] = FINAL.to_bytes(2, "little")
    header[20] = 32
    header[21] = 8
    blob = bytes(header) + bgra.tobytes() + (b"\x00" * 26)
    if len(blob) != 4_194_352:
        raise SystemExit(f"unexpected OZT size {len(blob)}")
    return blob


def main() -> None:
    att = (WORLD / "Terrain7.att").read_bytes()
    if len(att) < 3 + 256 * 256:
        raise SystemExit(f"Terrain7.att too short: {len(att)}")
    walls = att[3:]

    terrain = build_terrain(walls)            # RENDER x RENDER (8 px per tile)
    terrain.save(TEMP / "arena-minimap-textured-2048.png")

    final = terrain.resize((FINAL, FINAL), Image.LANCZOS)
    final = draw_objects(final)
    final.save(TEMP / "arena-minimap-textured-natural-1024.png")

    blob = pack_ozt(final)
    OUT.write_bytes(blob)

    # Client-orientation preview (what the TAB minimap shows).
    arr = np.asarray(final, dtype=np.uint8)
    Image.fromarray(np.transpose(arr, (1, 0, 2))).save(
        TEMP / "arena-minimap-textured-client-1024.png")

    for dest in (
        ROOT / "src" / "build" / "Release" / "Data" / "World7" / "mini_map.OZT",
        Path(r"C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\World7\mini_map.OZT"),
        Path(r"C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\World7\mini_map.OZT"),
    ):
        if dest.parent.exists():
            dest.write_bytes(blob)

    print("wrote", OUT, len(blob), "bytes")
    print("previews:", TEMP / "arena-minimap-textured-2048.png",
          TEMP / "arena-minimap-textured-natural-1024.png",
          TEMP / "arena-minimap-textured-client-1024.png")


if __name__ == "__main__":
    main()
