"""Build Data/Interface/LuxUI/*.OZT button faces for the Mu Helper strip, the
main toolbar and the two voice-chat buttons.

File format
-----------
Each OZT is what CGlobalBitmap::OpenTga reads (Render/Sprites/GlobalBitmap.cpp):
a 22-byte header with width at 16..17, height at 18..19 and depth 32 at 20,
followed by width*height*4 BGRA bytes stored bottom-up. Frames are stacked
vertically: 0 = up, 1 = hover, 2 = pressed (and 3 = attention blink on the
toolbar). Sizes are fixed by the call sites and must NOT change:

    helper_*.OZT   18 x 39   (3 frames of 18 x 13)  -- NewUIHeroPositionInfo
    toolbar_*.OZT  30 x 164  (4 frames of 30 x 41)  -- NewUIMainFrameWindow
    voice_mic.OZT  16 x 46   (2 frames of 16 x 23)  -- UI/Voice/VoiceIcons
    voice_sound.OZT 20 x 34  (2 frames of 20 x 17)  -- UI/Voice/VoiceIcons

The two voice faces have no button plate: they are free-standing glyphs on
transparent ground, drawn over the native MU button by VoiceIcons.cpp. Frame
0 is the active state, frame 1 the muted one (dimmed metal + a slash).

Art (v4)
--------
v3 rasterized the glyphs straight at the final 13..41 px with binary coverage
and faked the outline by redrawing every quad at eight 1-unit offsets, so the
icons had jagged edges and the outline smeared into a blob. Everything is now
rasterized on an SS-times-larger canvas and box-downsampled by area, which
gives real anti-aliasing and real fractional alpha; the outline is a single
rolling-max dilation of the *union* glyph mask, so it never bleeds inside the
shape.

Art (v5) -- the "candidate 1" direction
---------------------------------------
The owner picked a Canva mock-up (a brass two-pan balance over two stacks of
coins, top-lit on a near-black ground) as the art direction. That render is a
*reference*, not an asset: at 41 px its chains, pedestal and individual coins
collapse into a brown smear and the export has no alpha. What is carried over
is the language, not the pixels:

  * palette -- the brass ramp is now sampled from that render instead of from
    the pale Webzen trim. Its lit body sits at hue ~36, saturation ~0.55,
    lightness ~0.36 (#87682F is the single most common colour of the whole
    reference), and the ramp runs #2A1E0B .. #EFC574. Steel stays strictly
    neutral so the brass reads as "the valuable part".
  * ground -- the button face drops from #202020 to a near-black warm #1A1917,
    matching the reference's #050404 field, so the glyph carries the contrast.
  * lighting -- light comes from straight above: the vertical gradient is now
    asymmetric (a modest lift at the top, a much deeper fall at the bottom),
    the top row of every column is pushed towards white and the bottom row is
    multiplied down into an occlusion edge. That is what gives the reference
    its rounded mass, and it is the cheapest volume cue at 9 px.
  * outline -- a single dark ring still closes every silhouette, now in the
    reference's warm near-black and one notch thicker on the toolbar.
  * helper_market -- redrawn as a balance over a coin stack in that language,
    reduced to the masses that survive a 14 x 9 px face: a thick beam, two
    short hangers, two pan strokes, a centre post and a two-coin stack. No
    chains, no individual coins -- both vanish below ~2 px.

The button plate itself (ink rim, bevel, face, lit top row, shaded bottom row)
is the v3 shape the owner approved; only its corners are rounded by a couple of
pixels so the 3x on-screen upscale does not show hard black squares.
"""
from __future__ import annotations

import math
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "bin" / "Data" / "Interface" / "LuxUI"

SS = 8  # supersampling factor; the downsample is a plain area average

# -- palette ----------------------------------------------------------------
# Steel stays the strictly neutral ramp sampled from the shipped Webzen art
# (Data/Interface/newui_menu01/02.OZJ body greys #1F1F1F .. #B7B7B7, all at
# saturation 0.00). Only the brass moved in v5.
INK = (0x09, 0x07, 0x05)  # the reference's warm near-black field (#050404)
STEEL_XD = (0x24, 0x24, 0x24)
STEEL_D = (0x4A, 0x4A, 0x4A)
STEEL_M = (0x78, 0x78, 0x78)
STEEL_L = (0xA1, 0xA1, 0xA1)
STEEL_H = (0xC8, 0xC8, 0xC8)
# Brass ramp resampled from the candidate-1 render: of its 15.5k lit pixels the
# hue clusters at 36 +- 3 with saturation 0.55, and the lightness percentiles
# run p10 #291E0C, p25 #43341A, p50/body #87682F, p90 #B26E17, p99 #E8A94D.
# Rebuilt here as one clean ramp at hue 37 / saturation 0.59, with the top end
# pushed a little brighter than the reference so it still reads on a 9 px face.
GOLD_XD = (0x2A, 0x1E, 0x0B)
GOLD_D = (0x6B, 0x4D, 0x1C)
GOLD = (0xA0, 0x72, 0x2A)
GOLD_L = (0xCE, 0x98, 0x3C)
GOLD_H = (0xEF, 0xC5, 0x74)

# Near-black ground, like the reference's field: the glyph carries the contrast.
FACE_NORMAL = (0x1A, 0x19, 0x17)
FACE_HOVER = (0x33, 0x30, 0x2B)
FACE_PRESSED = (0x10, 0x0F, 0x0E)
FACE_ALERT = (0x57, 0x41, 0x1B)

STRIP_W, STRIP_FRAME_H, STRIP_FRAMES = 18, 13, 3
TOOL_W, TOOL_FRAME_H, TOOL_FRAMES = 30, 41, 4
# Free-standing voice glyphs (no plate): frame 0 active, frame 1 muted.
MIC_W, MIC_FRAME_H = 16, 23
SND_W, SND_FRAME_H = 20, 17

# The strip face is only 14x9 px, so the glyph box (+-10 units) plus its
# outline has to land inside 9 px: 20 * 0.40 + 2 * 0.6 = 9.2.
STRIP_GLYPH_SCALE = 0.40  # icon units -> final pixels
TOOL_GLYPH_SCALE = 1.0
VOICE_GLYPH_SCALE = 1.0
STRIP_OUTLINE_PX = 0.6
TOOL_OUTLINE_PX = 1.15
VOICE_OUTLINE_PX = 1.0
STRIP_PLATE_RADIUS = 1.5
TOOL_PLATE_RADIUS = 2.0

# Light from straight above, as in the reference: a modest lift at the top of
# the glyph and a much deeper fall at the bottom.
GRADIENT_TOP = 0.16
GRADIENT_BOTTOM = 0.30
HIGHLIGHT = 0.42  # how far the 1 px top edge is pushed towards white
OCCLUSION = 0.52  # how far the 1 px bottom edge is multiplied down
# Both edge passes cost a whole final pixel, which on the 30x41 toolbar and the
# voice glyphs is a thin rim but on the 18x13 strip is a *third* of a 3 px mass
# -- at full strength they simply ate the market pans. The strip therefore gets
# the same lighting at a fraction of the amplitude.
STRIP_HIGHLIGHT = 0.30
STRIP_OCCLUSION = 0.80


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

# Candidate 1 reduced to what survives a 14 x 9 px face. The reference's beam,
# pans, post and coin stack are all kept; its chains (below 0.4 px), pedestal
# and individual coins are dropped -- at 0.40 units/px a chain link would be a
# quarter of a pixel and would only fog the silhouette. Every mass below is at
# least 2.4 units, i.e. ~1 final pixel, so nothing dissolves in the downsample.
# Value ordering matters more than detail here: the beam and the two pans are
# the brightest masses (they carry the "balance" read), the post is deliberately
# dark so it separates them from the coins instead of welding beam, post and
# stack into one lit blob, and the stack is bright again at the foot.
MARKETPLACE = [
    # beam: lit top face over a dark underside, 3.5 units = 1.4 px total
    ("rect", -13.0, -9.0, 26.0, 2.3, GOLD_H),
    ("rect", -13.0, -6.7, 26.0, 1.2, GOLD_D),
    # centre post: mid value, so it still joins beam to stack without competing
    # with either (at GOLD_D it vanished into the face and the beam floated)
    ("rect", -1.5, -8.2, 3.0, 10.8, GOLD),
    # hangers: one pixel wide each, standing in for the reference's chains
    ("rect", -11.5, -5.5, 2.4, 2.2, GOLD_D),
    ("rect", 9.1, -5.5, 2.4, 2.2, GOLD_D),
    # the two pans, as single thick strokes
    ("poly", [(-14.7, -3.3), (-8.0, -3.3), (-9.6, -0.1), (-13.1, -0.1)], GOLD_H),
    ("poly", [(8.0, -3.3), (14.7, -3.3), (13.1, -0.1), (9.6, -0.1)], GOLD_L),
    # two-coin stack at the foot of the post. The seam has to be ~0.7 px or the
    # downsample averages it away and the stack reads as one bright slab.
    ("ell", 0.0, 4.3, 6.4, 1.8, GOLD_H),
    ("ell", 0.0, 8.5, 6.4, 1.8, GOLD_L),
    ("rect", -6.4, 5.5, 12.8, 1.7, GOLD_XD),
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


# --- voice chat (free-standing glyphs, 1 unit = 1 px) ----------------------
# Drawn by UI/Voice/VoiceIcons.cpp over the native MU button and, at world
# scale, over the speaker's head. Same steel body / brass accent split as the
# toolbar: the metal is neutral, the "live" parts (the capsule ring, the sound
# waves) are brass.

# NB: unlike the ten LuxUI faces, these do not sit on a near-black plate of
# their own -- they are drawn over the *light grey* native MU button
# (IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL). So the steel runs one step brighter than
# it would on the toolbar; the ink outline, not a dark ground, is what separates
# the glyph from the plate.
MIC_ACTIVE = [  # studio mic in a yoke: capsule, brass cap, grille, stand
    ("rrect", -4.6, -10.0, 9.2, 12.0, 4.6, STEEL_L),
    ("rect", -3.7, -7.6, 1.7, 9.0, STEEL_H),      # lit left flank
    ("rect", 2.1, -7.6, 2.3, 9.0, STEEL_M),       # shaded right flank
    ("rrect", -4.6, -10.0, 9.2, 2.8, 1.4, GOLD_L),  # brass cap ring
    ("rect", -3.4, -6.4, 6.8, 1.1, INK),          # grille slits
    ("rect", -3.4, -4.1, 6.8, 1.1, INK),
    ("rect", -3.4, -1.8, 6.8, 1.1, INK),
    ("rect", -7.0, -1.6, 1.8, 6.2, STEEL_M),      # yoke arms
    ("rect", 5.2, -1.6, 1.8, 6.2, STEEL_M),
    ("rect", -7.0, 4.6, 14.0, 1.8, STEEL_L),      # yoke bottom
    ("rect", -1.6, 6.4, 3.2, 1.9, STEEL_L),       # stem
    ("rrect", -6.0, 8.3, 12.0, 2.4, 1.0, STEEL_M),  # base
    ("rect", -5.2, 8.3, 10.4, 0.9, STEEL_H),      # lit lip of the base
]

SND_ACTIVE = [  # speaker cone + two brass waves
    ("rect", -8.4, -2.6, 3.2, 5.2, STEEL_M),      # magnet box
    ("poly", [(-5.4, -2.8), (-1.2, -7.2), (-1.2, 7.2), (-5.4, 2.8)], STEEL_M),
    ("poly", [(-4.0, -3.6), (-1.6, -6.0), (-1.6, 6.0), (-4.0, 3.6)], STEEL_L),
    ("rect", -1.8, -7.2, 2.0, 14.4, STEEL_H),     # lit cone rim
    # brass waves; the first one has to clear the bright rim by more than the
    # outline width or the two fuse into a single lit bar
    ("poly", [(2.4, -3.2), (4.2, -2.4), (4.2, 2.4), (2.4, 3.2)], GOLD_H),
    ("poly", [(6.2, -6.4), (8.2, -4.8), (8.2, 4.8), (6.2, 6.4)], GOLD_L),
]

# How far the muted frame drops the metal. Not lower than this: these glyphs
# sit on a mid-grey button, so below ~0.7 the dimmed body sinks into the plate
# and the muted frame collapses into a bare slash floating in an outline.
MUTE_DIM = 0.72


def _dim(colour, f=MUTE_DIM):
    return tuple(int(c * f + 0.5) for c in colour)


def dim_table(table, f=MUTE_DIM):
    """Muted variant of a glyph table: everything is pulled down towards the
    ground and the brass is desaturated to steel, so "off" reads as cold metal
    rather than as the same icon at a different alpha."""
    out = []
    for sh in table:
        col = sh[-1]
        if col is None or col == INK:
            out.append(sh)
            continue
        grey = (col[0] * 30 + col[1] * 59 + col[2] * 11) // 100
        out.append(sh[:-1] + (_dim((grey, grey, grey), f),))
    return out


def _slash(length, cx=0.0, cy=0.0, ink_w=5.0, core_w=2.4):
    """The muted bar: a dark rotated plate with a bright core, so it keeps its
    own edge where it crosses the glyph (the outline pass only rings the
    exterior of the union mask). It is centred on the *mass* it crosses out,
    not on the canvas, and sized to that canvas -- a slash long enough for the
    tall mic swamps the short speaker cone."""
    return [
        ("poly", _rot_rect(cx, cy, length, ink_w, -math.pi / 4.0), INK),
        ("poly", _rot_rect(cx, cy, length - 1.5, core_w, -math.pi / 4.0), STEEL_H),
    ]


MIC_MUTED = dim_table(MIC_ACTIVE) + _slash(21.0)
# waves drop with the sound; the slash re-centres over the cone it crosses out
SND_MUTED = dim_table(SND_ACTIVE[:4]) + _slash(18.0, -1.6, 0.0, 4.2, 2.0)


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


def shade_glyph(lay, hl_amt=HIGHLIGHT, occ_amt=OCCLUSION):
    """Top-down lighting, the candidate-1 read: an asymmetric vertical gradient
    (small lift at the top, deep fall at the bottom) plus a 1 px lit top edge
    and a 1 px occluded bottom edge on every column. Near-black pixels (the ink
    halo and the cut seams) are left alone so they stay ink."""
    w, h, buf = lay.w, lay.h, lay.buf
    ys = [y for y in range(h) if any(buf[(y * w + x) * 4 + 3] for x in range(w))]
    if not ys:
        return
    y0, y1 = ys[0], ys[-1]
    span = max(1, y1 - y0)
    for y in range(y0, y1 + 1):
        t = (y - y0) / span  # 0 at the top of the glyph, 1 at the bottom
        f = (1.0 + GRADIENT_TOP * (1.0 - 2.0 * t)) if t < 0.5 else \
            (1.0 - GRADIENT_BOTTOM * (2.0 * t - 1.0))
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

    edge = SS  # exactly one final pixel
    for x in range(w):
        col = [y for y in range(h) if buf[(y * w + x) * 4 + 3]]
        if not col:
            continue
        top, bot = col[0], col[-1]
        if buf[(top * w + x) * 4] + buf[(top * w + x) * 4 + 1] + \
                buf[(top * w + x) * 4 + 2] >= 120:
            for yy in range(top, min(h, top + edge)):
                j = (yy * w + x) * 4
                if not buf[j + 3]:
                    break
                for k in range(3):
                    v = buf[j + k]
                    buf[j + k] = int(v + (255 - v) * hl_amt + 0.5)
        if buf[(bot * w + x) * 4] + buf[(bot * w + x) * 4 + 1] + \
                buf[(bot * w + x) * 4 + 2] >= 120:
            for yy in range(bot, max(-1, bot - edge), -1):
                j = (yy * w + x) * 4
                if not buf[j + 3]:
                    break
                for k in range(3):
                    buf[j + k] = int(buf[j + k] * occ_amt + 0.5)


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


def make_glyph_layer(w, h, table, scale, outline_px,
                     hl_amt=HIGHLIGHT, occ_amt=OCCLUSION):
    """The glyph is identical across the frames of one button, so it is
    rasterized, shaded and outline-dilated exactly once."""
    lay = Layer(w * SS, h * SS)
    draw_glyph_shapes(lay, table, w * SS / 2.0, h * SS / 2.0, scale * SS)
    shade_glyph(lay, hl_amt, occ_amt)
    d = dilate(alpha_mask(lay), lay.w, lay.h, int(round(outline_px * SS)))
    return lay, d


def make_strip(table):
    glyph, d = make_glyph_layer(STRIP_W, STRIP_FRAME_H, table,
                                STRIP_GLYPH_SCALE, STRIP_OUTLINE_PX,
                                STRIP_HIGHLIGHT, STRIP_OCCLUSION)
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


def make_voice(w, h, tables):
    """Free-standing frames: glyph + ink outline over transparent ground, so
    the client can lay them over any button plate (or over the world)."""
    frames = []
    for table in tables:
        glyph, d = make_glyph_layer(w, h, table, VOICE_GLYPH_SCALE, VOICE_OUTLINE_PX)
        frames.append(downsample(compose(Layer(w * SS, h * SS), glyph, d), w, h))
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


def write_preview(strip_frames, tool_frames, voice_frames):
    """Contact sheet: every frame at 1x and at 4x over a HUD-dark ground, one
    band per family (helper strip, toolbar, voice)."""
    bg = (0x14, 0x16, 0x1A, 255)
    pad, gap = 8, 6
    zoom = 4
    bands = [strip_frames, tool_frames, voice_frames]
    # column pitch and band height are driven by the widest / tallest member
    pitch = 0
    band_h = []
    for band in bands:
        w = max(f[0].w for f in band.values())
        h = max(sum(l.h for l in f) for f in band.values()) * zoom
        pitch = max(pitch, w + gap + w * zoom + gap * 3)
        band_h.append(h)
    cols = max(len(b) for b in bands)
    sheet_w = pad + cols * pitch + pad
    sheet_h = pad + sum(band_h) + gap * 4 * len(bands) + pad
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

    top = pad
    for bi, band in enumerate(bands):
        for ci, frames in enumerate(band.values()):
            ox = pad + ci * pitch
            oy = top
            for lay in frames:
                blit(lay, ox, oy, 1)
                blit(lay, ox + lay.w + gap, oy, zoom)
                oy += lay.h * zoom
        top += band_h[bi] + gap * 4

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
# (width, frame height, [frame 0 = active, frame 1 = muted])
VOICE_ICONS = {
    "voice_mic": (MIC_W, MIC_FRAME_H, [MIC_ACTIVE, MIC_MUTED]),
    "voice_sound": (SND_W, SND_FRAME_H, [SND_ACTIVE, SND_MUTED]),
}


def build():
    strip_frames = {k: make_strip(t) for k, t in STRIP_ICONS.items()}
    tool_frames = {k: make_toolbar(t) for k, t in TOOLBAR_ICONS.items()}
    voice_frames = {k: make_voice(w, h, tabs)
                    for k, (w, h, tabs) in VOICE_ICONS.items()}
    return strip_frames, tool_frames, voice_frames


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    strip_frames, tool_frames, voice_frames = build()
    for group in (strip_frames, tool_frames, voice_frames):
        for key, frames in group.items():
            write_ozt(OUT / (key + ".OZT"), frames)
    write_preview(strip_frames, tool_frames, voice_frames)


if __name__ == "__main__":
    main()
