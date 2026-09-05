"""Painted Stadium (World7) minimap - the retail Webzen sheets, reproduced.

The shipped `Data/World*/mini_map.OZT` sheets are 1024x1024 BGRA paintings of
the whole 256x256 tile board (4 px per tile) that Webzen baked out of the 3D
scene: real ground textures at their real repeat, the terrain lightmap and the
height-field shading, every placed object drawn top-down with its own texture
and a cast shadow, and alpha 0 wherever the world has no ground.

World7 (Arena/Stadium) never shipped one - the retail client has no
`Data/World7/mini_map.OZT` at all - so this tool bakes ours the same way,
straight out of the client data:

  layer 1  terrain textures. `EncTerrain7.map` holds layer1[256^2] +
           layer2[256^2] + alpha[256^2] (see OpenTerrainMapping). The engine
           maps them with `uv = world * (64 / bitmapWidth) / TERRAIN_SCALE`
           (RenderTerrainFace), i.e. one tile always spans 64 texels, so a
           256 px tile texture repeats every 4 tiles and a 128 px one every 2.
           The two layers are mixed with the alpha interpolated across the quad
           from its four corner values, exactly like the vertex attribute.
  layer 2  light. `TerrainLight.OZJ` (256x256 RGB, decoded BOTTOM-UP by
           OpenJpegBuffer, hence the vertical flip here) times the height-field
           luminosity `dot(TerrainNormal, (0.5,-0.5,0.5)) + 0.5` that
           CreateTerrainLight() bakes, both interpolated per vertex.
  layer 3  objects. Every record of `EncTerrain7.obj` is the real
           `Data/Object7/ObjectNN.bmd` mesh, rotated by its Z yaw
           (AngleMatrix's `angles[2]`), rasterised orthographically from above
           with a Z buffer, textured from the model's own OZJ/OZT and shaded by
           the face normal. Cast shadows are the same silhouette displaced
           along the light direction by each pixel's own height.
  layer 4  alpha. `Terrain7.att` bit 0x08 (TW_NOGROUND) is the world's edge:
           outside the built campus the sheet is transparent, so the TAB
           overlay shows its 85% black underlay there - what Lorencia does.

Everything is rendered at 4096x4096 (16 px per tile) and box-reduced to
1024x1024, which is the anti-aliasing.

Orientation: `pack_ozt` writes map X descending (the TGA descriptor is 0x08 =
bottom-left origin, and CGlobalBitmap::OpenTga honours it, so file row 0 is the
last buffer row) - see the 2026-09-05 fix; getting this backwards mirrors the
sheet and puts the hero in the wrong half of the TAB map.

Run with `py -3.12 tools/render_arena_minimap_textured.py` (needs Pillow+numpy).
"""
from __future__ import annotations

import io
import math
import re
import struct
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
WORLD = ROOT / "src" / "bin" / "Data" / "World7"
OBJ_DIR = ROOT / "src" / "bin" / "Data" / "Object7"
OUT = WORLD / "mini_map.OZT"
TEMP = Path(r"C:\Users\joaop\AppData\Local\Temp")

FINAL = 1024            # shipped sheet size (4 px per tile)
SUPER = 4               # supersampling factor
RENDER = FINAL * SUPER  # 4096 (16 px per tile)
PPT = RENDER // 256     # render pixels per tile
TSCALE = 100.0          # TERRAIN_SCALE
HEIGHT_FACTOR = 1.5     # OpenTerrainHeight

# BITMAP_MAPTILE + n, in the order MapManager::LoadWorldTextures binds them.
TILE_SLOTS = [
    "TileGrass01", "TileGrass02", "TileGround01", "TileGround02",
    "TileGround03", "TileWater01", "TileWood01", "TileRock01",
    "TileRock02", "TileRock03", "TileRock04", "TileRock05",
    "TileRock06", "TileRock07",
]

MAP_KEY = bytes([0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
                 0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2])

# CreateTerrainLight()'s light vector, reused for the object shading so the
# whole sheet reads as one scene.
LIGHT = np.array([0.5, -0.5, 0.5], dtype=np.float64)
LIGHT_N = LIGHT / np.linalg.norm(LIGHT)

OBJ_AMBIENT = 0.42          # unlit floor of the object shading
# Additive glow planes (the Arena's red/blue corner markers). They are flat
# saturated primaries with no shape of their own; drawn opaque they read as
# paint spills, so they go down as translucent decals instead.
EFFECT_TEXTURES = {"au_5.tga", "au_6.tga", "au_05.tga", "au_06.tga"}
EFFECT_OPACITY = 0.34
SHADOW_LEN = 0.34           # cast-shadow length, as a fraction of the true one
SHADOW_STRENGTH = 0.42      # how much a shadow darkens the ground
EXPOSURE = 1.16             # the TAB overlay sits on 85% black; lift a little
GAMMA = 0.92


# --------------------------------------------------------------------------
# decoders
# --------------------------------------------------------------------------

def map_decrypt(src: bytes) -> bytes:
    """MapFileDecrypt: the chained cipher of EncTerrain*.map/.obj and BMD v12."""
    data = np.frombuffer(src, dtype=np.uint8).astype(np.int32)
    key = np.empty(len(data), dtype=np.int32)
    key[0] = 0x5E
    key[1:] = (data[:-1] + 0x3D) & 0xFF
    mask = np.frombuffer(MAP_KEY, dtype=np.uint8)[np.arange(len(data)) % 16]
    return ((data ^ mask) - key).astype(np.uint8).tobytes()


def read_ozj(path: Path) -> Image.Image:
    """OZJ = 24-byte header + JPEG. Returned RGB, top-down."""
    data = path.read_bytes()
    for m in re.finditer(b"\xff\xd8\xff", data):
        try:
            img = Image.open(io.BytesIO(data[m.start():]))
            img.load()
            return img.convert("RGB")
        except Exception:
            continue
    raise ValueError(f"{path.name}: no decodable JPEG stream")


def read_ozt(path: Path) -> Image.Image:
    """OZT = 4-byte header + uncompressed 32/24bpp TGA. Returned RGBA, top-down."""
    raw = path.read_bytes()
    w = int.from_bytes(raw[16:18], "little")
    h = int.from_bytes(raw[18:20], "little")
    bpp = raw[20]
    desc = raw[21]
    n = w * h * (bpp // 8)
    px = np.frombuffer(raw[22:22 + n], dtype=np.uint8).reshape(h, w, bpp // 8)
    img = px[..., [2, 1, 0, 3]] if bpp == 32 else np.dstack(
        [px[..., [2, 1, 0]], np.full((h, w, 1), 255, np.uint8)])
    if not (desc & 0x20):        # bottom-left origin
        img = img[::-1]
    return Image.fromarray(np.ascontiguousarray(img), "RGBA")


def read_texture(directory: Path, name: str) -> np.ndarray:
    """A BMD material name ('au_02.jpg' / 'tree_08.tga') as RGBA float32 0-255."""
    stem = Path(name).stem
    ext = Path(name).suffix.lower()
    if ext in (".tga", ".ozt"):
        p = directory / f"{stem}.OZT"
        if p.exists():
            return np.asarray(read_ozt(p), dtype=np.float32)
    p = directory / f"{stem}.OZJ"
    if p.exists():
        rgb = np.asarray(read_ozj(p), dtype=np.float32)
        return np.dstack([rgb, np.full(rgb.shape[:2], 255.0, np.float32)])
    p = directory / f"{stem}.OZT"
    if p.exists():
        return np.asarray(read_ozt(p), dtype=np.float32)
    raise FileNotFoundError(f"{directory.name}/{name}")


# --------------------------------------------------------------------------
# BMD
# --------------------------------------------------------------------------

def load_bmd(path: Path) -> list[dict]:
    """Meshes of an ObjectNN.bmd, as BMD::Open2 reads them.

    Vertex_t 16 B, Normal_t 20 B, TexCoord_t 8 B, Triangle_t2 64 B, then the
    material name (32 B). Returns a list of {V, T, tri, tex}.
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
    ptr += 32                                   # model name
    num_meshes = struct.unpack_from("<h", data, ptr)[0]
    ptr += 6                                    # + NumBones, NumActions
    meshes = []
    for _ in range(num_meshes):
        nv, nn, nt, ntri, _tex = struct.unpack_from("<hhhhh", data, ptr)
        ptr += 10
        verts = np.frombuffer(data, np.float32, nv * 4, ptr).reshape(nv, 4)[:, 1:]
        ptr += nv * 16
        ptr += nn * 20
        uvs = (np.frombuffer(data, np.float32, nt * 2, ptr).reshape(nt, 2)
               if nt else np.zeros((0, 2), np.float32))
        ptr += nt * 8
        blob = np.frombuffer(data, np.uint8, ntri * 64, ptr).reshape(ntri, 64)
        ptr += ntri * 64
        vi = blob[:, 2:8].copy().view(np.int16).reshape(ntri, 3)
        ti = blob[:, 18:24].copy().view(np.int16).reshape(ntri, 3)
        texname = data[ptr:ptr + 32].split(b"\x00")[0].decode("latin1")
        ptr += 32
        if ntri and nv:
            meshes.append({"V": np.array(verts, np.float64),
                           "T": np.array(uvs, np.float64),
                           "vi": vi.astype(np.int32),
                           "ti": ti.astype(np.int32),
                           "tex": texname})
    return meshes


def parse_obj(path: Path) -> list[tuple[int, np.ndarray, np.ndarray, float]]:
    """EncTerrain7.obj placements: (type, position, angles, scale)."""
    dec = map_decrypt(path.read_bytes())
    (count,) = struct.unpack_from("<h", dec, 2)
    out = []
    off = 4
    for _ in range(count):
        (typ,) = struct.unpack_from("<h", dec, off)
        pos = np.array(struct.unpack_from("<fff", dec, off + 2), np.float64)
        ang = np.array(struct.unpack_from("<fff", dec, off + 14), np.float64)
        (scale,) = struct.unpack_from("<f", dec, off + 26)
        off += 30
        if typ < 0 or scale <= 0:
            continue
        out.append((typ, pos, ang, float(scale)))
    return out


def angle_matrix(angles: np.ndarray) -> np.ndarray:
    """ZzzMathLib::AngleMatrix - (Z * Y) * X, degrees."""
    sy, cy = math.sin(math.radians(angles[2])), math.cos(math.radians(angles[2]))
    sp, cp = math.sin(math.radians(angles[1])), math.cos(math.radians(angles[1]))
    sr, cr = math.sin(math.radians(angles[0])), math.cos(math.radians(angles[0]))
    return np.array([
        [cp * cy, sr * sp * cy - cr * sy, cr * sp * cy + sr * sy],
        [cp * sy, sr * sp * sy + cr * cy, cr * sp * sy - sr * cy],
        [-sp,     sr * cp,                cr * cp],
    ], dtype=np.float64)


# --------------------------------------------------------------------------
# terrain
# --------------------------------------------------------------------------

def bilinear_upsample(corners: np.ndarray) -> np.ndarray:
    """(257,257[,c]) vertex grid -> (RENDER,RENDER[,c]) sampled at pixel centres."""
    idx = (np.arange(RENDER) + 0.5) / PPT
    i0 = np.clip(idx.astype(np.int32), 0, 255)
    t = (idx - i0)[:, None] if corners.ndim == 2 else (idx - i0)[:, None, None]
    rows = corners[i0] * (1.0 - t) + corners[i0 + 1] * t
    t2 = (idx - i0)[None, :] if corners.ndim == 2 else (idx - i0)[None, :, None]
    return rows[:, i0] * (1.0 - t2) + rows[:, i0 + 1] * t2


def terrain_vertex_light() -> np.ndarray:
    """BackTerrainLight as a (257,257,3) vertex grid, float 0..1."""
    light = np.asarray(read_ozj(WORLD / "TerrainLight.OZJ").resize((256, 256)),
                       dtype=np.float64)[::-1] / 255.0      # TJFLAG_BOTTOMUP
    height = np.frombuffer((WORLD / "TerrainHeight.OZB").read_bytes(),
                           np.uint8, 65536, 1084).reshape(256, 256).astype(np.float64)
    height *= HEIGHT_FACTOR                                  # BackTerrainHeight

    # CreateTerrainNormal: two face normals of the quad (x,y)..(x+1,y+1), summed.
    def h(dy, dx):
        return np.roll(np.roll(height, -dy, axis=0), -dx, axis=1)

    def face(a, b, c):
        n = np.cross(b - a, c - a)
        return n / np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-9)

    S = TSCALE
    yy, xx = np.meshgrid(np.arange(256.0) * S, np.arange(256.0) * S, indexing="ij")
    v4 = np.stack([xx, yy, h(0, 0)], axis=-1)
    v1 = np.stack([xx + S, yy, h(0, 1)], axis=-1)
    v2 = np.stack([xx + S, yy + S, h(1, 1)], axis=-1)
    v3 = np.stack([xx, yy + S, h(1, 0)], axis=-1)
    normal = face(v1, v2, v3) + face(v3, v4, v1)
    lum = np.clip((normal * LIGHT).sum(axis=-1) + 0.5, 0.0, 1.0)

    vertex = light * lum[..., None]
    grid = np.empty((257, 257, 3))
    grid[:256, :256] = vertex
    grid[256, :256] = vertex[255]
    grid[:256, 256] = vertex[:, 255]
    grid[256, 256] = vertex[255, 255]
    return grid


def ground_height_grid() -> np.ndarray:
    height = np.frombuffer((WORLD / "TerrainHeight.OZB").read_bytes(),
                           np.uint8, 65536, 1084).reshape(256, 256).astype(np.float64)
    height *= HEIGHT_FACTOR
    grid = np.empty((257, 257))
    grid[:256, :256] = height
    grid[256, :256] = height[255]
    grid[:256, 256] = height[:, 255]
    grid[256, 256] = height[255, 255]
    return grid


def build_terrain() -> np.ndarray:
    """(RENDER, RENDER, 3) uint8 painted ground, indexed [terrainY][terrainX]."""
    dec = map_decrypt((WORLD / "EncTerrain7.map").read_bytes())
    layer1 = np.frombuffer(dec[2:2 + 65536], np.uint8).reshape(256, 256).astype(np.int32)
    layer2 = np.frombuffer(dec[2 + 65536:2 + 131072], np.uint8).reshape(256, 256).astype(np.int32)
    alpha = np.frombuffer(dec[2 + 131072:2 + 196608], np.uint8).reshape(256, 256).astype(np.float64) / 255.0

    # RenderTerrainFace's layer collapse, evaluated on the quad's four corners.
    a4 = np.stack([alpha,
                   np.roll(alpha, -1, axis=1),
                   np.roll(np.roll(alpha, -1, axis=0), -1, axis=1),
                   np.roll(alpha, -1, axis=0)], axis=0)
    all_opaque = (a4 >= 1.0).all(axis=0)
    all_clear = (a4 <= 0.0).all(axis=0)
    tex1 = np.where(all_opaque, layer2, layer1)
    tex2 = np.where(all_clear, layer1, layer2)
    tex1 = np.where(tex1 == 255, layer1, tex1) % len(TILE_SLOTS)
    tex2 = np.where(tex2 == 255, tex1, tex2) % len(TILE_SLOTS)

    # One tile always spans 64 texels (uv = world * (64/width) / TERRAIN_SCALE),
    # so a texture of W px repeats every W/64 tiles. Box-reduce each texture to
    # PPT px per tile and let numpy's modulo do the GL_REPEAT.
    patterns: dict[int, np.ndarray] = {}
    for slot in set(np.unique(tex1)) | set(np.unique(tex2)):
        name = TILE_SLOTS[int(slot)]
        path = WORLD / f"{name}.OZJ"
        if not path.exists():
            patterns[int(slot)] = np.full((PPT, PPT, 3), 128.0)
            continue
        img = read_ozj(path)
        rep_w = max(1, img.width // 64)          # tiles per repeat
        rep_h = max(1, img.height // 64)
        patterns[int(slot)] = np.asarray(
            img.resize((rep_w * PPT, rep_h * PPT), Image.BOX), dtype=np.float64)

    corner_alpha = np.empty((257, 257))
    corner_alpha[:256, :256] = alpha
    corner_alpha[256, :256] = alpha[255]
    corner_alpha[:256, 256] = alpha[:, 255]
    corner_alpha[256, 256] = alpha[255, 255]

    light_grid = terrain_vertex_light()

    out = np.empty((RENDER, RENDER, 3), np.uint8)
    idx = (np.arange(RENDER) + 0.5) / PPT
    i0 = np.clip(idx.astype(np.int32), 0, 255)
    frac = idx - i0
    band = 256                                    # 16 tile rows at a time
    px = np.arange(RENDER)
    for y0 in range(0, RENDER, band):
        rows = slice(y0, y0 + band)
        ry = i0[rows]
        ty = frac[rows][:, None]
        # bilinear alpha and light for this band
        ca = corner_alpha
        a_top = ca[ry][:, i0] * (1 - frac)[None, :] + ca[ry][:, i0 + 1] * frac[None, :]
        a_bot = ca[ry + 1][:, i0] * (1 - frac)[None, :] + ca[ry + 1][:, i0 + 1] * frac[None, :]
        a = a_top * (1 - ty) + a_bot * ty
        lg = light_grid
        l_top = lg[ry][:, i0] * (1 - frac)[None, :, None] + lg[ry][:, i0 + 1] * frac[None, :, None]
        l_bot = lg[ry + 1][:, i0] * (1 - frac)[None, :, None] + lg[ry + 1][:, i0 + 1] * frac[None, :, None]
        lit = l_top * (1 - ty[..., None]) + l_bot * ty[..., None]

        base = np.zeros((rows.stop - rows.start, RENDER, 3))
        over = np.zeros_like(base)
        t1 = tex1[ry][:, i0]
        t2 = tex2[ry][:, i0]
        for slot, pat in patterns.items():
            ph, pw = pat.shape[:2]
            sample = pat[np.ix_(np.arange(y0, min(y0 + band, RENDER)) % ph, px % pw)]
            m1 = t1 == slot
            if m1.any():
                base[m1] = sample[m1]
            m2 = t2 == slot
            if m2.any():
                over[m2] = sample[m2]
        col = base * (1.0 - a[..., None]) + over * a[..., None]
        col *= lit
        out[rows] = np.clip(col, 0, 255).astype(np.uint8)
    return out


# --------------------------------------------------------------------------
# objects
# --------------------------------------------------------------------------

class ObjectRaster:
    """Orthographic top-down Z-buffer rasteriser over the whole sheet."""

    def __init__(self) -> None:
        self.color = np.zeros((RENDER, RENDER, 3), np.float32)
        self.zbuf = np.full((RENDER, RENDER), -1e9, np.float32)
        self.cover = np.zeros((RENDER, RENDER), bool)
        self.opacity = np.zeros((RENDER, RENDER), np.float32)

    def triangle(self, p: np.ndarray, uv: np.ndarray, tex: np.ndarray,
                 shade: float, opacity: float = 1.0) -> None:
        """p: (3,3) pixel x, pixel y, world z. uv: (3,2). tex: (h,w,4) 0..255."""
        x0 = max(int(math.floor(p[:, 0].min() - 1)), 0)
        x1 = min(int(math.ceil(p[:, 0].max() + 1)), RENDER - 1)
        y0 = max(int(math.floor(p[:, 1].min() - 1)), 0)
        y1 = min(int(math.ceil(p[:, 1].max() + 1)), RENDER - 1)
        if x1 < x0 or y1 < y0:
            return
        ax, ay = p[0, 0], p[0, 1]
        bx, by = p[1, 0], p[1, 1]
        cx, cy = p[2, 0], p[2, 1]
        area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
        if abs(area) < 1e-9:
            return
        sign = 1.0 if area > 0 else -1.0
        area *= sign
        gx = np.arange(x0, x1 + 1, dtype=np.float64) + 0.5
        gy = np.arange(y0, y1 + 1, dtype=np.float64) + 0.5
        X = gx[None, :]
        Y = gy[:, None]
        w0 = ((cx - bx) * (Y - by) - (cy - by) * (X - bx)) * sign
        w1 = ((ax - cx) * (Y - cy) - (ay - cy) * (X - cx)) * sign
        w2 = ((bx - ax) * (Y - ay) - (by - ay) * (X - ax)) * sign
        # Conservative by half a render pixel (1/8 of a shipped pixel) so that
        # fence rails and other sub-pixel geometry survive.
        e0 = math.hypot(cx - bx, cy - by) * 0.5 + 1e-9
        e1 = math.hypot(ax - cx, ay - cy) * 0.5 + 1e-9
        e2 = math.hypot(bx - ax, by - ay) * 0.5 + 1e-9
        inside = (w0 >= -e0) & (w1 >= -e1) & (w2 >= -e2)
        if not inside.any():
            return
        iy, ix = np.nonzero(inside)
        b0 = np.clip(w0[iy, ix] / area, 0.0, 1.0)
        b1 = np.clip(w1[iy, ix] / area, 0.0, 1.0)
        b2 = np.clip(1.0 - b0 - b1, 0.0, 1.0)
        z = b0 * p[0, 2] + b1 * p[1, 2] + b2 * p[2, 2]
        py = iy + y0
        px = ix + x0
        keep = z > self.zbuf[py, px]
        if not keep.any():
            return
        py, px, b0, b1, b2, z = py[keep], px[keep], b0[keep], b1[keep], b2[keep], z[keep]
        th, tw = tex.shape[:2]
        u = b0 * uv[0, 0] + b1 * uv[1, 0] + b2 * uv[2, 0]
        v = b0 * uv[0, 1] + b1 * uv[1, 1] + b2 * uv[2, 1]
        tu = np.mod((u * tw).astype(np.int32), tw)
        tv = np.mod((v * th).astype(np.int32), th)
        texel = tex[tv, tu]
        if tex.shape[2] == 4:
            solid = texel[:, 3] > 96
            if not solid.any():
                return
            py, px, z, texel = py[solid], px[solid], z[solid], texel[solid]
        self.zbuf[py, px] = z
        self.color[py, px] = texel[:, :3] * shade
        self.cover[py, px] = True
        self.opacity[py, px] = opacity


def draw_objects(raster: ObjectRaster) -> tuple[int, int]:
    placements = parse_obj(WORLD / "EncTerrain7.obj")
    models: dict[int, list[dict] | None] = {}
    textures: dict[str, np.ndarray] = {}
    drawn = skipped = 0
    for typ, pos, ang, scale in placements:
        if typ not in models:
            path = OBJ_DIR / f"Object{typ + 1:02d}.bmd"
            try:
                models[typ] = load_bmd(path)
            except Exception as exc:      # noqa: BLE001
                print(f"  skip type {typ}: {exc}")
                models[typ] = None
        meshes = models[typ]
        if not meshes:
            skipped += 1
            continue
        rot = angle_matrix(ang)
        for mesh in meshes:
            name = mesh["tex"]
            if name not in textures:
                try:
                    textures[name] = read_texture(OBJ_DIR, name)
                except Exception:         # noqa: BLE001
                    textures[name] = np.full((1, 1, 4), (150, 140, 130, 255), np.float32)
            tex = textures[name]
            opacity = EFFECT_OPACITY if name.lower() in EFFECT_TEXTURES else 1.0
            world = (mesh["V"] * scale) @ rot.T + pos
            sx = world[:, 0] / TSCALE * PPT
            sy = world[:, 1] / TSCALE * PPT
            sz = world[:, 2]
            uvs = mesh["T"]
            vi = mesh["vi"]
            ti = mesh["ti"]
            nv = len(world)
            nt = len(uvs)
            for k in range(len(vi)):
                a, b, c = vi[k]
                if a >= nv or b >= nv or c >= nv or a < 0 or b < 0 or c < 0:
                    continue
                tri = np.array([[sx[a], sy[a], sz[a]],
                                [sx[b], sy[b], sz[b]],
                                [sx[c], sy[c], sz[c]]])
                if (tri[:, 0].max() < 0 or tri[:, 0].min() > RENDER
                        or tri[:, 1].max() < 0 or tri[:, 1].min() > RENDER):
                    continue
                normal = np.cross(world[b] - world[a], world[c] - world[a])
                norm = np.linalg.norm(normal)
                if norm < 1e-9:
                    continue
                normal = normal / norm
                if normal[2] < 0:
                    normal = -normal
                shade = OBJ_AMBIENT + (1.0 - OBJ_AMBIENT) * max(
                    0.0, float(np.dot(normal, LIGHT_N)))
                ta, tb, tc = ti[k]
                if nt and 0 <= ta < nt and 0 <= tb < nt and 0 <= tc < nt:
                    uv = uvs[[ta, tb, tc]]
                else:
                    uv = np.zeros((3, 2))
                raster.triangle(tri, uv, tex, shade, opacity)
            drawn += 1
    return drawn, skipped


def cast_shadows(cover: np.ndarray, zbuf: np.ndarray,
                 ground: np.ndarray) -> np.ndarray:
    """Silhouette displaced along -L by each pixel's own height above ground."""
    ys, xs = np.nonzero(cover)
    if len(ys) == 0:
        return np.zeros((RENDER, RENDER), np.float32)
    h = np.maximum(zbuf[ys, xs] - ground[ys, xs], 0.0)
    step = h / TSCALE * PPT * SHADOW_LEN
    tx = np.clip((xs - step * (LIGHT_N[0] / LIGHT_N[2])).astype(np.int32), 0, RENDER - 1)
    ty = np.clip((ys - step * (LIGHT_N[1] / LIGHT_N[2])).astype(np.int32), 0, RENDER - 1)
    shadow = np.zeros((RENDER, RENDER), np.float32)
    shadow[ty, tx] = 1.0
    # soften: a 3-tap box in each axis, twice (cheap separable blur)
    for _ in range(2):
        shadow = (shadow
                  + np.roll(shadow, 1, 0) + np.roll(shadow, -1, 0)
                  + np.roll(shadow, 1, 1) + np.roll(shadow, -1, 1)) / 5.0
    return np.clip(shadow * 1.9, 0.0, 1.0)


# --------------------------------------------------------------------------
# packing
# --------------------------------------------------------------------------

def reachable_ground(has_ground: np.ndarray, walkable: np.ndarray) -> np.ndarray:
    """Ground tiles 4-connected to any walkable tile.

    Terrain7.att carries 326 stray ground tiles in rows 249-252 and column 252,
    left over from the original authoring: none of them is walkable and none
    touches the Arena, but painted they would show as a dashed line along the
    far edge of the TAB sheet. Lorencia has no such islands, so dropping them is
    what makes ours look like Lorencia, not a deviation from the .att rule.
    """
    seen = (walkable & has_ground).copy()
    stack = [(int(y), int(x)) for y, x in np.argwhere(seen)]
    if not stack:
        return has_ground
    while stack:
        y, x = stack.pop()
        for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
            if 0 <= ny < 256 and 0 <= nx < 256 and has_ground[ny, nx] and not seen[ny, nx]:
                seen[ny, nx] = True
                stack.append((ny, nx))
    return seen


def box_reduce(arr: np.ndarray) -> np.ndarray:
    h, w = arr.shape[:2]
    rest = arr.shape[2:]
    return arr.reshape(h // SUPER, SUPER, w // SUPER, SUPER, *rest).mean(axis=(1, 3))


def pack_ozt(rgb: np.ndarray, alpha: np.ndarray) -> bytes:
    """OZT blob. Buffer row = map X, column = map Y; the file stores X descending.

    CNewUIMiniMap centres the sheet on the hero at u = PositionY/256,
    v = PositionX/256, so map X grows downwards on screen; the TGA descriptor
    byte is 0x08 (bottom-left origin) and CGlobalBitmap::OpenTga copies file row
    y to buffer row ny-1-y. Hence the ::-1 - writing straight through mirrors
    the sheet on X (the 2026-09-05 bug).
    """
    p = np.transpose(rgb, (1, 0, 2))[::-1]
    pa = alpha.T[::-1]
    bgra = np.empty((FINAL, FINAL, 4), np.uint8)
    bgra[..., 0] = p[..., 2]
    bgra[..., 1] = p[..., 1]
    bgra[..., 2] = p[..., 0]
    bgra[..., 3] = pa
    header = bytearray(22)
    header[2] = 2
    header[6] = 2
    header[16:18] = FINAL.to_bytes(2, "little")
    header[18:20] = FINAL.to_bytes(2, "little")
    header[20] = 32
    header[21] = 8
    footer = (b"\x00" * 8) + b"TRUEVISION-XFILE." + b"\x00"
    blob = bytes(header) + bgra.tobytes() + footer
    if len(blob) != 4_194_352:
        raise SystemExit(f"unexpected OZT size {len(blob)}")
    return blob


# --------------------------------------------------------------------------

def main() -> None:
    t0 = time.time()
    att = (WORLD / "Terrain7.att").read_bytes()
    if len(att) < 3 + 65536:
        raise SystemExit(f"Terrain7.att too short: {len(att)}")
    walls = np.frombuffer(att[3:3 + 65536], np.uint8).reshape(256, 256)
    walkable = np.isin(walls, (0, 1))
    has_ground = reachable_ground((walls & 0x08) == 0, walkable)   # TW_NOGROUND

    print(f"[{time.time() - t0:5.1f}s] terrain ...")
    terrain = build_terrain().astype(np.float32)

    print(f"[{time.time() - t0:5.1f}s] objects ...")
    raster = ObjectRaster()
    drawn, skipped = draw_objects(raster)
    print(f"[{time.time() - t0:5.1f}s] objects: {drawn} meshes drawn, {skipped} skipped")

    ground_px = bilinear_upsample(ground_height_grid()).astype(np.float32)
    solid = raster.cover & (raster.opacity > 0.9)
    shadow = cast_shadows(solid, raster.zbuf, ground_px)
    del solid
    del ground_px
    terrain *= (1.0 - SHADOW_STRENGTH * shadow)[..., None]
    del shadow

    op = raster.opacity[..., None]
    terrain *= 1.0 - op
    terrain += raster.color * op                            # objects on top
    raster.color = None
    raster.zbuf = None
    raster.opacity = None

    print(f"[{time.time() - t0:5.1f}s] downsample ...")
    rgb = box_reduce(terrain)
    rgb = 255.0 * np.power(np.clip(rgb * EXPOSURE, 0, 255) / 255.0, GAMMA)
    rgb = np.clip(rgb, 0, 255).astype(np.uint8)

    mask = np.repeat(np.repeat(has_ground, PPT, 0), PPT, 1).astype(np.float32)
    mask = np.maximum(mask, raster.cover.astype(np.float32))
    for _ in range(2):                     # feather ~1 shipped pixel
        mask = (mask
                + np.roll(mask, 2, 0) + np.roll(mask, -2, 0)
                + np.roll(mask, 2, 1) + np.roll(mask, -2, 1)) / 5.0
    alpha = np.clip(box_reduce(np.clip(mask * 2.2, 0, 1)) * 255.0, 0, 255).astype(np.uint8)

    # Sanity: every walkable tile must land on a painted, opaque pixel.
    small = np.repeat(np.repeat(walkable, FINAL // 256, 0), FINAL // 256, 1)
    bad = int((small & (alpha < 200)).sum())
    print(f"walkable tiles: {int(walkable.sum())}, walkable pixels not opaque: {bad}")
    if bad:
        raise SystemExit("alpha mask does not cover the walkable board")

    blob = pack_ozt(rgb, alpha)
    OUT.write_bytes(blob)

    Image.fromarray(np.dstack([rgb, alpha]), "RGBA").save(
        TEMP / "arena-minimap-painted-sheet.png")
    px = np.frombuffer(blob[22:22 + FINAL * FINAL * 4], np.uint8)
    px = px.reshape(FINAL, FINAL, 4)[::-1, :, [2, 1, 0, 3]]
    Image.fromarray(px, "RGBA").save(TEMP / "arena-minimap-painted-client.png")

    for dest in (
        ROOT / "src" / "build" / "Release" / "Data" / "World7" / "mini_map.OZT",
        Path(r"C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\World7\mini_map.OZT"),
        Path(r"C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\World7\mini_map.OZT"),
    ):
        if dest.parent.exists():
            dest.write_bytes(blob)

    print(f"[{time.time() - t0:5.1f}s] wrote {OUT} ({len(blob)} bytes)")


if __name__ == "__main__":
    sys.exit(main())
