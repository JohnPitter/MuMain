"""Build Data/Interface/LuxUI/*.OZT button faces for the Mu Helper strip and
the main toolbar.

File format
-----------
Each OZT is what CGlobalBitmap::OpenTga reads (Render/Sprites/GlobalBitmap.cpp):
a 22-byte header with width at 16..17, height at 18..19 and depth 32 at 20,
followed by width*height*4 BGRA bytes stored bottom-up. Frames are stacked
vertically: 0 = up, 1 = hover, 2 = pressed (and 3 = attention blink on the
toolbar). Sizes are fixed by the call sites and must NOT change:

    helper_*.OZT   18 x 39   (3 frames of 18 x 13)  -- NewUIHeroPositionInfo
    toolbar_*.OZT  30 x 164  (4 frames of 30 x 41)  -- NewUIMainFrameWindow

Art (v4)
--------
v3 rasterized the glyphs straight at the final 13..41 px with binary coverage
and faked the outline by redrawing every quad at eight 1-unit offsets, so the
icons had jagged edges and the outline smeared into a blob. Everything is now
rasterized on an SS-times-larger canvas and box-downsampled by area, which
gives real anti-aliasing and real fractional alpha; the outline is a single
rolling-max dilation of the *union* glyph mask, so it never bleeds inside the
shape. Glyphs also get a gentle vertical gradient and a 1 px top highlight for
the metallic read of the original art.

The palette is sampled from the shipped Webzen interface art rather than
invented (Data/Interface/newui_menu*.OZJ colour + newui_menu_Bt0*.OZT alpha):
the frame is a strictly neutral grey ramp (#1F1F1F .. #BCBCBC, saturation 0)
accented by a warm brass at hue ~40 (#7D663A .. #DCCA9C), plus the bright
#FCDA17 the main bar uses for its hottest highlight.

The button plate itself (ink rim, bevel, face, lit top row, shaded bottom row)
is the v3 shape the owner approved; only its corners are now rounded by a
couple of pixels so the 3x on-screen upscale does not show hard black squares.
"""
from __future__ import annotations

import math
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "bin" / "Data" / "Interface" / "LuxUI"

SS = 8  # supersampling factor; the downsample is a plain area average

# -- palette sampled from the original Webzen art ---------------------------
# neutral steel ramp: newui_menu01/02.OZJ body greys (#1F1F1F #323232 #4D4D4D
# #6B6B6B #787878 #8C8C8C #A1A1A1 #B7B7B7), all at saturation 0.00.
INK = (0x0A, 0x0A, 0x0A)
STEEL_XD = (0x24, 0x24, 0x24)
STEEL_D = (0x4A, 0x4A, 0x4A)
STEEL_M = (0x78, 0x78, 0x78)
STEEL_L = (0xA1, 0xA1, 0xA1)
STEEL_H = (0xC8, 0xC8, 0xC8)
# warm brass ramp: newui_menu_Bt0*.OZJ under their OZT masks plus the gold
# trim of newui_menu01/02 (#7D663A #987E4B #B1945C #C0A264 #CAB075 #D5BF8E
# #DCCA9C), hue 39..43, and the #FCDA17 hot accent of the main bar.
GOLD_D = (0x7D, 0x66, 0x3A)
GOLD = (0xC0, 0xA2, 0x64)
GOLD_L = (0xD9, 0xC7, 0x99)
GOLD_H = (0xF0, 0xE3, 0xBC)

FACE_NORMAL = (0x20, 0x20, 0x20)
FACE_HOVER = (0x3A, 0x3A, 0x3A)
FACE_PRESSED = (0x14, 0x14, 0x14)
FACE_ALERT = (0x4A, 0x3D, 0x22)

STRIP_W, STRIP_FRAME_H, STRIP_FRAMES = 18, 13, 3
TOOL_W, TOOL_FRAME_H, TOOL_FRAMES = 30, 41, 4

# The strip face is only 14x9 px, so the glyph box (+-10 units) plus its
# outline has to land inside 9 px: 20 * 0.40 + 2 * 0.6 = 9.2.
STRIP_GLYPH_SCALE = 0.40  # icon units -> final pixels
TOOL_GLYPH_SCALE = 1.0
STRIP_OUTLINE_PX = 0.6
TOOL_OUTLINE_PX = 1.0
STRIP_PLATE_RADIUS = 1.5
TOOL_PLATE_RADIUS = 2.0

GRADIENT = 0.13  # +/- brightness across the glyph height
HIGHLIGHT = 0.34  # how far the 1 px top edge is pushed towards white


# ---------------------------------------------------------------- raster ---
class Layer:
    """RGBA8 buffer, top-down rows. Alpha is binary while rasterizing; the
    fractional alpha appears when the layer is downsampled."""

    __slots__ = ("w", "h", "buf")

    def __init__(self, w, h, rgba=(0, 0, 0, 0)):
        self.w = w
        self.h = h
        self.buf = bytearray(bytes(rgba)) * (w * h)

    def span(self, y, x0, x1, rgba4):
        if y < 0 or y >= self.h:
            return
        if x0 < 0:
            x0 = 0
        if x1 > self.w:
            x1 = self.w
        if x1 <= x0:
            return
        i = (y * self.w + x0) * 4
        self.buf[i:i + (x1 - x0) * 4] = rgba4 * (x1 - x0)


def _rows(y, h, limit):
    """Pixel rows whose centre falls inside [y, y+h)."""
    lo = max(0, int(math.ceil(y - 0.5)))
    hi = min(limit, int(math.ceil(y + h - 0.5)))
    return range(lo, hi)


def _cols(x, w):
    return int(math.ceil(x - 0.5)), int(math.ceil(x + w - 0.5))


def fill_rect(lay, x, y, w, h, rgba4):
    x0, x1 = _cols(x, w)
    for py in _rows(y, h, lay.h):
        lay.span(py, x0, x1, rgba4)


def fill_rrect(lay, x, y, w, h, r, rgba4):
    r = max(0.0, min(r, w / 2.0, h / 2.0))
    for py in _rows(y, h, lay.h):
        cy = py + 0.5
        inset = 0.0
        if cy < y + r:
            dy = (y + r) - cy
            inset = r - math.sqrt(max(0.0, r * r - dy * dy))
        elif cy > y + h - r:
            dy = cy - (y + h - r)
            inset = r - math.sqrt(max(0.0, r * r - dy * dy))
        x0, x1 = _cols(x + inset, w - 2 * inset)
        lay.span(py, x0, x1, rgba4)


def fill_ellipse(lay, cx, cy, rx, ry, rgba4):
    for py in _rows(cy - ry, 2 * ry, lay.h):
        dy = (py + 0.5) - cy
        t = 1.0 - (dy * dy) / (ry * ry)
        if t <= 0.0:
            continue
        dx = rx * math.sqrt(t)
        x0, x1 = _cols(cx - dx, 2 * dx)
        lay.span(py, x0, x1, rgba4)


def fill_poly(lay, pts, rgba4):
    ys = [p[1] for p in pts]
    lo = max(0, int(math.ceil(min(ys) - 0.5)))
    hi = min(lay.h, int(math.ceil(max(ys) - 0.5)))
    n = len(pts)
    for py in range(lo, hi):
        cy = py + 0.5
        xs = []
        for i in range(n):
            x0, y0 = pts[i]
            x1, y1 = pts[(i + 1) % n]
            if (y0 <= cy < y1) or (y1 <= cy < y0):
                xs.append(x0 + (cy - y0) * (x1 - x0) / (y1 - y0))
        xs.sort()
        for i in range(0, len(xs) - 1, 2):
            a, b = _cols(xs[i], xs[i + 1] - xs[i])
            lay.span(py, a, b, rgba4)


# ------------------------------------------------------------- glyph DSL ---
# Shapes are authored on a 24-unit grid, origin at the glyph centre, +y down.
# A colour of None erases (punches a hole in the glyph layer), which is how the
# gear teeth, the helmet visor and the bag handle are cut.
#   ("rect",  x, y, w, h, colour)
#   ("rrect", x, y, w, h, radius, colour)
#   ("ell",   cx, cy, rx, ry, colour)
#   ("circ",  cx, cy, r, colour)
#   ("poly",  [(x, y), ...], colour)

def _rot_rect(cx, cy, w, h, ang):
    ca, sa = math.cos(ang), math.sin(ang)
    return [(cx + px * ca - py * sa, cy + px * sa + py * ca)
            for px, py in ((-w / 2, -h / 2), (w / 2, -h / 2),
                           (w / 2, h / 2), (-w / 2, h / 2))]


def _gear_teeth(radius, tw, th, n=6):
    out = []
    for k in range(n):
        a = (k + 0.5) * 2.0 * math.pi / n
        out.append(("poly", _rot_rect(radius * math.cos(a), radius * math.sin(a),
                                      tw, th, a), None))
    return out


def _tri(x0, x1, half):
    return [(x0, -half), (x1, 0.0), (x0, half)]


def _bust(cx, head_cy, head_r, top, bottom, half_w, colour, grow=0.0, flat=0.62):
    """Head + shoulders: a circle over a body with a flattened round top."""
    r = half_w + grow
    ry = r * flat
    return [
        ("circ", cx, head_cy, head_r + grow, colour),
        ("ell", cx, top - grow + ry, r, ry, colour),
        ("rect", cx - r, top - grow + ry, 2 * r, (bottom + grow) - (top - grow + ry), colour),
    ]


# --- helper strip (18x13 plate, glyph box x +-15, y +-10) ------------------
SETTINGS = [("circ", 0, 0, 9.6, STEEL_L)] + _gear_teeth(10.4, 4.8, 5.4) + [
    ("circ", 0, 0, 4.3, INK),
    ("circ", 0, 0, 3.0, GOLD),
]

PLAY = [
    ("poly", _tri(-6.5, 10.5, 9.8), GOLD),
]

STOP = [
    ("rrect", -8.5, -8.5, 17.0, 17.0, 3.0, STEEL_L),
]

AUTO_BATTLE = [  # fast-forward: two triangles kept apart by an ink seam
    ("poly", _tri(-13.0, -3.0, 9.8), STEEL_L),
    ("poly", _tri(-1.8, 8.6, 9.8), GOLD),
    ("rect", -3.4, -10.0, 1.6, 20.0, INK),
]

MARKETPLACE = [  # stack of coins
    ("ell", 0, 4.4, 11.0, 5.2, GOLD),
    ("ell", 0, -4.4, 11.0, 5.2, GOLD_L),
    ("ell", 0, 0.0, 10.4, 1.7, GOLD_D),
]

# --- main toolbar (30x41 plate, glyph box x +-12.5, y -17..14) -------------
CASH_SHOP = [  # cut jewel -- the only glyph with a pointed bottom
    ("poly", [(-12, -5), (-7, -12.5), (7, -12.5), (12, -5)], GOLD_L),
    ("poly", [(-12, -5), (12, -5), (0, 13.5)], GOLD),
    ("poly", [(3.5, -5), (12, -5), (0, 13.5)], GOLD_D),
    ("rect", -12, -5.8, 24.0, 1.5, INK),
]

CHARACTER = [  # great helm: wide at the top, narrow at the jaw (the exact
               # inverse of the inventory pouch) with a T visor cut through it
    ("circ", 0, -3.0, 11.0, STEEL_M),
    ("poly", [(-11, -3.0), (11, -3.0), (6.5, 12.5), (-6.5, 12.5)], STEEL_M),
    ("rect", -11, -6.2, 22.0, 2.4, GOLD),
    ("rect", -8.6, -3.4, 17.2, 4.6, None),
    ("rect", -1.9, -3.4, 3.8, 13.5, None),
    ("rrect", -2.0, -17.5, 4.0, 6.0, 1.2, GOLD),
]

INVENTORY = [  # pouch with a carry handle
    ("circ", 0, -6.0, 8.0, STEEL_D),
    ("circ", 0, -6.0, 5.0, None),
    ("rect", -9.0, -6.0, 18.0, 10.0, None),
    ("poly", [(-8.5, -7.0), (8.5, -7.0), (12, 13.5), (-12, 13.5)], STEEL_M),
    ("poly", [(-9.0, -7.5), (9.0, -7.5), (10.8, -0.5), (-10.8, -0.5)], STEEL_L),
    ("rect", -10.8, -0.9, 21.6, 1.4, STEEL_XD),
    ("rrect", -3.2, -2.0, 6.4, 5.5, 1.2, GOLD),
]

FRIENDS = (  # two busts, the front one haloed in ink so they stay separate
    _bust(5.6, -8.5, 4.6, -3.4, 13.5, 6.4, GOLD_D)
    + _bust(-3.0, -4.2, 5.8, 0.8, 14.0, 7.8, INK, grow=1.1)
    + _bust(-3.0, -4.2, 5.8, 0.8, 14.0, 7.8, STEEL_L)
)

MENU = [  # gold frame over three bars
    ("rrect", -12, -12, 24.0, 24.0, 3.5, GOLD),
    ("rrect", -9.8, -9.8, 19.6, 19.6, 2.4, STEEL_XD),
    ("rrect", -6.8, -7.2, 13.6, 3.8, 1.4, STEEL_L),
    ("rrect", -6.8, -1.9, 13.6, 3.8, 1.4, STEEL_L),
    ("rrect", -6.8, 3.4, 13.6, 3.8, 1.4, STEEL_L),
]


def draw_glyph_shapes(lay, table, ox, oy, s):
    """Rasterize a glyph table into `lay` (already in supersampled space)."""
    for sh in table:
        kind, col = sh[0], sh[-1]
        rgba = bytes((0, 0, 0, 0)) if col is None else bytes(col) + b"\xff"
        if kind == "rect":
            _, x, y, w, h, _c = sh
            fill_rect(lay, ox + x * s, oy + y * s, w * s, h * s, rgba)
        elif kind == "rrect":
            _, x, y, w, h, r, _c = sh
            fill_rrect(lay, ox + x * s, oy + y * s, w * s, h * s, r * s, rgba)
        elif kind == "ell":
            _, cx, cy, rx, ry, _c = sh
            fill_ellipse(lay, ox + cx * s, oy + cy * s, rx * s, ry * s, rgba)
        elif kind == "circ":
            _, cx, cy, r, _c = sh
            fill_ellipse(lay, ox + cx * s, oy + cy * s, r * s, r * s, rgba)
        elif kind == "poly":
            _, pts, _c = sh
            fill_poly(lay, [(ox + px * s, oy + py * s) for px, py in pts], rgba)
        else:
            raise ValueError(kind)


# ------------------------------------------------------------ post passes ---
def alpha_mask(lay):
    return bytearray(lay.buf[3::4])


def dilate(mask, w, h, r):
    """Chebyshev dilation of a binary mask via row/column prefix sums."""
    if r <= 0:
        return bytearray(mask)
    tmp = bytearray(w * h)
    for y in range(h):
        base = y * w
        pre = [0] * (w + 1)
        for x in range(w):
            pre[x + 1] = pre[x] + (1 if mask[base + x] else 0)
        for x in range(w):
            a = x - r
            if a < 0:
                a = 0
            b = x + r + 1
            if b > w:
                b = w
            if pre[b] - pre[a]:
                tmp[base + x] = 1
    out = bytearray(w * h)
    for x in range(w):
        pre = [0] * (h + 1)
        for y in range(h):
            pre[y + 1] = pre[y] + (1 if tmp[y * w + x] else 0)
        for y in range(h):
            a = y - r
            if a < 0:
                a = 0
            b = y + r + 1
            if b > h:
                b = h
            if pre[b] - pre[a]:
                out[y * w + x] = 1
    return out


def shade_glyph(lay):
    """Vertical gradient + a 1 px lit top edge -- the metallic read of the
    original art. Near-black pixels (the ink halo) are left alone."""
    w, h, buf = lay.w, lay.h, lay.buf
    ys = [y for y in range(h) if any(buf[(y * w + x) * 4 + 3] for x in range(w))]
    if not ys:
        return
    y0, y1 = ys[0], ys[-1]
    span = max(1, y1 - y0)
    for y in range(y0, y1 + 1):
        f = 1.0 + GRADIENT * (1.0 - 2.0 * (y - y0) / span)
        base = y * w * 4
        for x in range(w):
            i = base + x * 4
            if not buf[i + 3]:
                continue
            if buf[i] + buf[i + 1] + buf[i + 2] < 120:  # keep the ink ink
                continue
            for k in range(3):
                v = int(buf[i + k] * f + 0.5)
                buf[i + k] = 255 if v > 255 else v

    hl = SS  # exactly one final pixel
    for x in range(w):
        for y in range(h):
            i = (y * w + x) * 4
            if not buf[i + 3]:
                continue
            if buf[i] + buf[i + 1] + buf[i + 2] >= 120:
                for yy in range(y, min(h, y + hl)):
                    j = (yy * w + x) * 4
                    if not buf[j + 3]:
                        break
                    for k in range(3):
                        v = buf[j + k]
                        buf[j + k] = int(v + (255 - v) * HIGHLIGHT + 0.5)
            break


def compose(plate, glyph, d):
    """Ink-outline the glyph silhouette, then paint the glyph over the plate.
    `d` is the pre-computed dilated silhouette mask."""
    w, h = plate.w, plate.h
    pb, gb = plate.buf, glyph.buf
    ink = bytes(INK) + b"\xff"
    for i in range(w * h):
        j = i * 4
        if gb[j + 3]:
            pb[j:j + 4] = gb[j:j + 4]
        elif d[i]:
            pb[j:j + 4] = ink
    return plate


def downsample(lay, w, h):
    out = Layer(w, h)
    src, ob, sw = lay.buf, out.buf, lay.w
    n = SS * SS
    half = n // 2
    for oy in range(h):
        ybase = oy * SS
        for ox in range(w):
            sr = sg = sb = sa = 0
            for dy in range(SS):
                i = ((ybase + dy) * sw + ox * SS) * 4
                for _ in range(SS):
                    if src[i + 3]:
                        sr += src[i]
                        sg += src[i + 1]
                        sb += src[i + 2]
                        sa += 1
                    i += 4
            j = (oy * w + ox) * 4
            if sa:
                ob[j] = (sr + sa // 2) // sa
                ob[j + 1] = (sg + sa // 2) // sa
                ob[j + 2] = (sb + sa // 2) // sa
                ob[j + 3] = (sa * 255 + half) // n
    return out


# ----------------------------------------------------------------- frames ---
def make_plate(w, h, face, bevel, radius):
    """v3 DrawButtonPlate: ink rim, bevel, face, lit top row, shaded bottom
    row -- now with rounded corners and a faint face gradient."""
    lay = Layer(w * SS, h * SS)
    S = float(SS)
    fill_rrect(lay, 0, 0, w * S, h * S, radius * S, bytes(INK) + b"\xff")
    fill_rrect(lay, S, S, (w - 2) * S, (h - 2) * S, max(0.0, radius - 0.5) * S,
               bytes(bevel) + b"\xff")
    fw, fh = (w - 4) * S, (h - 4) * S
    steps = h - 4
    for k in range(steps):
        f = 1.0 + 0.10 * (1.0 - 2.0 * k / max(1, steps - 1))
        c = bytes(min(255, int(v * f + 0.5)) for v in face) + b"\xff"
        fill_rrect(lay, 2 * S, (2 + k) * S, fw, S,
                   max(0.0, radius - 1.0) * S if k == 0 or k == steps - 1 else 0.0, c)
    fill_rect(lay, 2 * S, 2 * S, fw, S, bytes(STEEL_D) + b"\xff")
    fill_rect(lay, 2 * S, (h - 3) * S, fw, S, bytes(INK) + b"\xff")
    return lay


def make_glyph_layer(w, h, table, scale, outline_px):
    """The glyph is identical across the frames of one button, so it is
    rasterized, shaded and outline-dilated exactly once."""
    lay = Layer(w * SS, h * SS)
    draw_glyph_shapes(lay, table, w * SS / 2.0, h * SS / 2.0, scale * SS)
    shade_glyph(lay)
    d = dilate(alpha_mask(lay), lay.w, lay.h, int(round(outline_px * SS)))
    return lay, d


def make_strip(table):
    glyph, d = make_glyph_layer(STRIP_W, STRIP_FRAME_H, table,
                                STRIP_GLYPH_SCALE, STRIP_OUTLINE_PX)
    frames = []
    for face in (FACE_NORMAL, FACE_HOVER, FACE_PRESSED):
        plate = make_plate(STRIP_W, STRIP_FRAME_H, face, STEEL_D, STRIP_PLATE_RADIUS)
        frames.append(downsample(compose(plate, glyph, d), STRIP_W, STRIP_FRAME_H))
    return frames


def make_toolbar(table):
    glyph, d = make_glyph_layer(TOOL_W, TOOL_FRAME_H, table,
                                TOOL_GLYPH_SCALE, TOOL_OUTLINE_PX)
    frames = []
    for face, bevel in ((FACE_NORMAL, STEEL_D), (FACE_HOVER, STEEL_D),
                        (FACE_PRESSED, STEEL_D), (FACE_ALERT, GOLD_L)):
        plate = make_plate(TOOL_W, TOOL_FRAME_H, face, bevel, TOOL_PLATE_RADIUS)
        frames.append(downsample(compose(plate, glyph, d), TOOL_W, TOOL_FRAME_H))
    return frames


# ------------------------------------------------------------------- I/O ---
def write_ozt(path, frames):
    """frames: list of Layer (visual top-down). File rows are bottom-up."""
    w = frames[0].w
    total_h = frames[0].h * len(frames)
    blob = bytearray(22)
    blob[2] = 2
    blob[16:18] = struct.pack("<H", w)
    blob[18:20] = struct.pack("<H", total_h)
    blob[20] = 32
    blob[21] = 8

    rows = []
    for lay in frames:
        for y in range(lay.h):
            rows.append(lay.buf[(y * w) * 4:(y * w + w) * 4])
    rows.reverse()
    for row in rows:
        for x in range(w):
            i = x * 4
            blob += bytes((row[i + 2], row[i + 1], row[i], row[i + 3]))
    blob += b"\x00" * 26
    path.write_bytes(blob)
    print(path.name, w, "x", total_h, len(blob), "bytes")


def read_ozt(path):
    """Decode an OZT written by this script (or by the client's art) back into
    (w, h, rows of RGBA tuples). Used by the before/after comparison tool."""
    d = path.read_bytes()
    w = struct.unpack_from("<H", d, 16)[0]
    h = struct.unpack_from("<H", d, 18)[0]
    off = 22
    rows = []
    for y in range(h):
        row = []
        for x in range(w):
            i = off + (y * w + x) * 4
            row.append((d[i + 2], d[i + 1], d[i], d[i + 3]))
        rows.append(row)
    rows.reverse()
    return w, h, rows


def write_png(path, w, h, rows):
    """rows: list of h lists of (r, g, b, a)."""
    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    raw = bytearray()
    for row in rows:
        raw.append(0)
        for px in row:
            raw += bytes(px)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    Path(path).write_bytes(png)


def write_preview(strip_frames, tool_frames):
    """Contact sheet: every frame at 1x and at 4x over a HUD-dark ground."""
    bg = (0x14, 0x16, 0x1A, 255)
    pad, gap = 8, 6
    zoom = 4
    cell_w = STRIP_W + gap + STRIP_W * zoom
    tcell_w = TOOL_W + gap + TOOL_W * zoom
    strip_h = STRIP_FRAMES * STRIP_FRAME_H * zoom
    tool_h = TOOL_FRAMES * TOOL_FRAME_H * zoom
    cols = max(len(strip_frames), len(tool_frames))
    sheet_w = pad + cols * (max(cell_w, tcell_w) + gap * 3) + pad
    sheet_h = pad + strip_h + gap * 4 + tool_h + pad
    sheet = [[bg for _ in range(sheet_w)] for _ in range(sheet_h)]

    def blit(lay, ox, oy, z):
        for y in range(lay.h):
            for x in range(lay.w):
                i = (y * lay.w + x) * 4
                r, g, b, a = lay.buf[i], lay.buf[i + 1], lay.buf[i + 2], lay.buf[i + 3]
                if not a:
                    continue
                for sy in range(z):
                    yy = oy + y * z + sy
                    if not (0 <= yy < sheet_h):
                        continue
                    row = sheet[yy]
                    for sx in range(z):
                        xx = ox + x * z + sx
                        if 0 <= xx < sheet_w:
                            br, bg_, bb, _ = row[xx]
                            row[xx] = ((r * a + br * (255 - a)) // 255,
                                       (g * a + bg_ * (255 - a)) // 255,
                                       (b * a + bb * (255 - a)) // 255, 255)

    step = max(cell_w, tcell_w) + gap * 3
    for ci, frames in enumerate(strip_frames.values()):
        ox = pad + ci * step
        for fi, lay in enumerate(frames):
            blit(lay, ox, pad + fi * STRIP_FRAME_H * zoom, 1)
            blit(lay, ox + STRIP_W + gap, pad + fi * STRIP_FRAME_H * zoom, zoom)
    top = pad + strip_h + gap * 4
    for ci, frames in enumerate(tool_frames.values()):
        ox = pad + ci * step
        for fi, lay in enumerate(frames):
            blit(lay, ox, top + fi * TOOL_FRAME_H * zoom, 1)
            blit(lay, ox + TOOL_W + gap, top + fi * TOOL_FRAME_H * zoom, zoom)

    write_png(OUT / "_preview.png", sheet_w, sheet_h, sheet)
    print("preview:", OUT / "_preview.png")


STRIP_ICONS = {
    "helper_settings": SETTINGS,
    "helper_play": PLAY,
    "helper_stop": STOP,
    "helper_auto": AUTO_BATTLE,
    "helper_market": MARKETPLACE,
}
TOOLBAR_ICONS = {
    "toolbar_cashshop": CASH_SHOP,
    "toolbar_character": CHARACTER,
    "toolbar_inventory": INVENTORY,
    "toolbar_friends": FRIENDS,
    "toolbar_menu": MENU,
}


def build():
    strip_frames = {k: make_strip(t) for k, t in STRIP_ICONS.items()}
    tool_frames = {k: make_toolbar(t) for k, t in TOOLBAR_ICONS.items()}
    return strip_frames, tool_frames


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    strip_frames, tool_frames = build()
    for key, frames in strip_frames.items():
        write_ozt(OUT / (key + ".OZT"), frames)
    for key, frames in tool_frames.items():
        write_ozt(OUT / (key + ".OZT"), frames)
    write_preview(strip_frames, tool_frames)


if __name__ == "__main__":
    main()
