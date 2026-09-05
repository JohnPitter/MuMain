"""Build Data/Interface/LuxUI/*.OZT button faces for the Mu Helper strip, the
main toolbar and the voice-chat glyphs.

File format
-----------
Each OZT is what CGlobalBitmap::OpenTga reads (Render/Sprites/GlobalBitmap.cpp):
a 22-byte header with width at 16..17, height at 18..19 and depth 32 at 20,
followed by width*height*4 BGRA bytes stored bottom-up. Frames are stacked
vertically. Sizes are fixed by the call sites and must NOT change:

    helper_*.OZT    18 x 39   (3 frames of 18 x 13)  -- NewUIHeroPositionInfo
    toolbar_*.OZT   30 x 164  (4 frames of 30 x 41)  -- NewUIMainFrameWindow
    voice_mic.OZT   16 x 46   (2 frames of 16 x 23)  -- UI/Voice/VoiceIcons
    voice_sound.OZT 20 x 34   (2 frames of 20 x 17)  -- UI/Voice/VoiceIcons

Art (v6) -- Canva silhouettes, Webzen finish
--------------------------------------------
v4 and v5 drew the glyphs from a hand-written vector DSL. The owner has since
approved a set of Canva renders whose silhouettes are much better, but whose
finish is wrong in a very specific, measurable way: they are mobile-app icons
(3D render, hard gradient, specular hotspot, projected shadow, saturation 14%
to 72%) rather than MU interface art (flat 2D, one warm diffuse light, matte
and slightly dirty, mean saturation 8%).

So the pixels now come from the Canva sheet and the FINISH comes from a grade
measured off the shipped Webzen art. tools/webzen_grade.py holds the
measurement and the colour science -- including which side of the
"are the Webzen greys neutral or warm?" question is right (both, split by
luminance: neutral shadows, warm highlights at hue 41.5 deg). Per icon:

  1. isolate()  strips the black card, the vignette/spotlight and the drop
                shadow by fitting a bi-cubic backdrop model to the border ring.
  2. grade()    flattens the render shading, matches the value histogram onto
                the Webzen one (so nothing reaches white -- the Webzen maximum
                is V=87% -- and nothing reaches black), posterizes, then sets
                saturation from the measured S(V) curve at hue 41.5 deg.
  3. here       fit into the glyph box at 8x, ink-outline by a true dilation of
                the alpha, composite onto the v3 button plate the owner
                approved, and box-filter down to the final size.

Two glyphs are not straight Canva imports:
  helper_auto    is DERIVED from the graded play_2 -- the same triangle, the
                 same material and bevel, twice, so "fast forward" reads as
                 "play, more of it". The Canva auto_* candidates all came out
                 as mountains.
  helper_market  is drawn here. cand1.png (the two-pan balance over coin
                 stacks) is the owner's direction reference, but it cannot be
                 cropped into an asset: the left pan is occluded by the coin
                 stacks and the chains are sub-pixel at 8 px tall. The balance
                 is redrawn with a beam and pans thick enough to survive, then
                 pushed through the same grade so the material matches.

The plate (v7) -- the shipped potion slot, not a drawn one
----------------------------------------------------------
v3..v6 drew the button plate procedurally (ink rim, bevel, flat steel face).
In game that reads as a smooth opaque grey card next to the potion/consumable
slots, whose plate is Webzen's engraved, mottled, almost-black metal. So the
plate is no longer drawn: it is LIFTED from the shipped bar art.

  source     Interface/newui_menu01.OZJ, the "Q" potion slot at (0,0)-(38,41).
             That art is the whole slot: black gutter, the bar's gold top
             rule at row 1, the lit rim at row 4, the four corner brackets and
             the cloudy interior. The baked yellow "Q" in the top-left is
             erased by mirroring the slot's own right half over it (the plate
             is left/right symmetric, so the seam is invisible).
  toolbar    38 -> 30 px wide by DROPPING the middle columns and crossfading
             over the seam, never by scaling: the brackets and the texture
             stay at 1:1. The height is already 41, so nothing is resampled
             vertically. Row for row this now lines up with Webzen's own
             30x41 toolbar buttons in partCharge1/newui_menu_Bt0*.OZJ --
             including the gold rule, which the main bar stops supplying at
             x=488 and which those buttons carry themselves.
  helper     the same plate below the gold rule (rows 2..40), box-filtered to
             18x13. There is no gold rule on the Minimap_position bar the
             helper strip sits on.

The four states are not invented either: measured on the border ring of all
five partCharge1/newui_menu_Bt0*.OZJ, Webzen's own frames are the SAME plate
at gain 1.000 / 1.251 / 0.790 / 0.809 (up / over / down / down+over), always
neutral (R=G=B). Those gains are used verbatim. Frame 3 additionally keeps the
gold accent the owner already had, as a hue-41.5 tint at the measured gain.
"""
from __future__ import annotations

import struct
import sys
import zlib
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
import webzen_grade as wg  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "bin" / "Data" / "Interface" / "LuxUI"
IFACE = ROOT / "src" / "bin" / "Data" / "Interface"

# The Canva sheet lives outside the repo (it is not an asset, it is a source
# render). Override with MU_CANVA_SHEET if it moved.
import os  # noqa: E402

CANVA = Path(os.environ.get("MU_CANVA_SHEET", str(
    Path.home() / "AppData" / "Local" / "Temp" / "claude"
    / "C--Users-joaop-Desenvolvimento-openmu"
    / "d85fad75-627a-4ee9-ba65-4a3efd0fcda5" / "scratchpad" / "canva")))

SS = 8      # supersampling factor; the downsample is a plain area average
WORK = 240  # the Canva renders are 400x400; grading at 240 is plenty and quick

# The glyph outline. Strictly neutral, and deliberately so: the measured
# Webzen shadow bins sit at 0.40% saturation and both quantised darks
# (#373737, #2A2A2A) are exactly R=G=B. A "hair warm" ink is also a trap for
# the metric -- +2 of R-B at value 18 is 11% HSV saturation, and the outline
# is a fifth of the art.
INK = (0x11, 0x11, 0x11)

# -- the shipped plate ------------------------------------------------------
# The "Q" potion slot inside Interface/newui_menu01.OZJ, and the box holding
# the baked yellow "Q" that gets mirrored away (see the module docstring).
SLOT_RECT = (0, 0, 38, 41)
SLOT_LETTER = (0, 2, 17, 16)      # x0, y0, x1, y1
SLOT_GOLD_RULE = 2                # rows 0..1 are the main bar's gold top rule

# Plate gain per frame, measured on the border ring of all five
# partCharge1/newui_menu_Bt0*.OZJ: up / over / down / down+over. Identical to
# three decimals across the five buttons, and neutral in all four frames.
GAIN_UP, GAIN_OVER, GAIN_DOWN, GAIN_ALERT = 1.000, 1.251, 0.790, 0.809
# Frame 3 keeps the gold accent the owner already had. Hue 41.5 deg is the
# Webzen highlight hue webzen_grade.py measured; the channel factors are
# renormalised so the tint costs no luminance.
ALERT_SAT = 0.30

STRIP_W, STRIP_FRAME_H, STRIP_FRAMES = 18, 13, 3
TOOL_W, TOOL_FRAME_H, TOOL_FRAMES = 30, 41, 4
MIC_W, MIC_FRAME_H, MIC_FRAMES = 16, 23, 2
SND_W, SND_FRAME_H, SND_FRAMES = 20, 17, 2

# Glyph box in final pixels, INCLUDING the ink outline, and the outline width.
# The strip face is 14x9 and the toolbar face 26x37, so these are what fits.
STRIP_BOX, STRIP_OUTLINE = (13.0, 9.0), 0.7
# TOOL_BOX was (25.0, 32.0) through v7: 32/41 = 78% of the frame height, against
# the plated glyphs which now read as too big for the plate's corner brackets.
# Measured against Webzen's own reference (union of the non-plate bounding
# box across all five partCharge1/newui_menu_Bt01..05.OZJ, frame 1 of 4 --
# median-plate diff per pixel, closed/opened to drop JPEG speckle, largest
# component kept): x 7..23, y 11..31 in the 30x41 frame, i.e. 17x21, or 21/41
# = 51% of the frame height -- in line with the ~55% estimate that flagged
# this in the first place.
TOOL_BOX, TOOL_OUTLINE = (17.0, 21.0), 0.8
# The voice glyphs sit on the light grey native MU button, not on a plate of
# their own. They are one row short of their frame on purpose: VoiceIcons draws
# a vertically stacked sheet with GL_LINEAR, so a glyph that touched the frame
# boundary would bleed the muted frame into the active one at the seam.
MIC_BOX, MIC_OUTLINE = (16.0, 21.0), 1.0
SND_BOX, SND_OUTLINE = (20.0, 15.0), 1.0


# ----------------------------------------------------------------- plate ---
def _ozj(path):
    """A .OZJ is a JPEG behind a 24-byte header."""
    import io

    im = Image.open(io.BytesIO(Path(path).read_bytes()[24:]))
    im.load()
    return im.convert("RGB")


_plate = None


def slot_art():
    """The empty potion slot, as float RGB rows x cols x 3.

    The shipped slot has a yellow "Q" baked into its top-left corner. The
    plate is left/right symmetric, so the letter box is overwritten with the
    slot's own mirrored right half; nothing is invented and nothing is blurred.
    """
    global _plate
    if _plate is not None:
        return _plate
    import numpy as np

    x0, y0, x1, y1 = SLOT_RECT
    bar = np.asarray(_ozj(IFACE / "newui_menu01.OZJ"), dtype=np.float64)
    q = bar[y0:y1, x0:x1].copy()
    lx0, ly0, lx1, ly1 = SLOT_LETTER
    q[ly0:ly1, lx0:lx1] = q[:, ::-1][ly0:ly1, lx0:lx1]
    _plate = q
    return q


def _narrow(a, w):
    """Take the plate down to w columns by DROPPING middle columns, with a
    short crossfade over the seam. The interior is a low-frequency cloud, so
    the join is invisible -- and unlike a resize this keeps both corner
    brackets and the texture itself at 1:1."""
    import numpy as np

    big = a.shape[1]
    if w == big:
        return a.copy()
    assert w < big, (w, big)
    left = w // 2
    out = np.concatenate([a[:, :left], a[:, big - (w - left):]], axis=1)
    blend = 6
    for i in range(blend):
        t = (i + 0.5) / blend
        c = left - blend // 2 + i
        if 0 <= c < w:
            out[:, c] = a[:, c] * (1.0 - t) + a[:, big - w + c] * t
    return out


def plate_rgb(w, h):
    """The plate at the frame size, before shading.

    Toolbar (30x41): the slot is already 41 tall, so only the width changes,
    and it changes by dropping columns rather than by resampling.
    Helper strip (18x13): the gold rule is dropped -- the Minimap_position bar
    has none -- and what is left is box-filtered down.
    """
    import numpy as np

    a = slot_art()
    if h == a.shape[0]:
        return _narrow(a, w)
    a = a[SLOT_GOLD_RULE:]
    im = Image.fromarray(np.clip(a, 0, 255).astype("uint8"))
    return np.asarray(im.resize((w, h), Image.BOX), dtype=np.float64)


def plate_ss(w, h, gain, warm=0.0):
    """A shaded plate at supersample resolution.

    NEAREST to SS and an area average back down is the identity, so every
    pixel the glyph does not cover survives byte for byte.
    """
    import numpy as np

    a = plate_rgb(w, h) * gain
    if warm:
        f = 41.5 / 60.0
        cf = np.array([1.0, 1.0 - warm * (1.0 - f), 1.0 - warm])
        a = a * (cf / cf.mean())
    im = Image.fromarray(np.clip(a, 0, 255).astype("uint8")).convert("RGBA")
    return im.resize((w * SS, h * SS), Image.NEAREST)


# ------------------------------------------------------------ glyph source ---
_cache = {}


def graded(name, **kw):
    """Load a Canva render, isolate it and apply the Webzen grade. Returns an
    RGBA image cropped to the object's bounding box."""
    if name in _cache:
        return _cache[name]
    path = CANVA / "sheet" / (name + ".png")
    if not path.exists():
        path = CANVA / (name + ".png")
    im = Image.open(path).convert("RGB").resize((WORK, WORK), Image.LANCZOS)
    mask = wg.isolate(im, **kw)
    # crc32, not hash(): str hashing is salted per process, so hash() made the
    # 3% grime noise -- and therefore the shipped bytes -- differ on every run.
    g = wg.grade(im, mask, seed=zlib.crc32(name.encode()) & 0xFFFF)
    g = g.crop(g.getbbox())
    _cache[name] = g
    return g


def synthetic(draw_fn, size=WORK, **kw):
    """Draw a flat greyscale shape, then push it through the same grade so its
    material is identical to the imported art."""
    im = Image.new("RGB", (size, size), (10, 10, 10))
    draw_fn(ImageDraw.Draw(im), size)
    mask = wg.isolate(im, **kw)
    g = wg.grade(im, mask, seed=1337)
    return g.crop(g.getbbox())


def _market(d, S):
    """A two-pan balance, in cand1.png's spirit but built for 8 px: the beam
    is 6% of the height, the pans are solid wedges, and there are no chains."""
    def s(v):
        return v * S / 100.0
    lo, mid, hi = (86, 86, 84), (150, 148, 143), (206, 200, 188)
    # base
    d.polygon([(s(30), s(88)), (s(70), s(88)), (s(62), s(74)), (s(38), s(74))], fill=mid)
    d.rectangle([s(44), s(28), s(56), s(76)], fill=mid)            # post
    d.rectangle([s(44), s(28), s(48), s(76)], fill=hi)             # post highlight
    d.polygon([(s(50), s(12)), (s(58), s(24)), (s(42), s(24))], fill=hi)  # finial
    d.rectangle([s(10), s(26), s(90), s(36)], fill=hi)             # beam
    d.rectangle([s(10), s(33), s(90), s(36)], fill=lo)             # beam underside
    d.rectangle([s(17), s(36), s(20), s(48)], fill=mid)            # hangers
    d.rectangle([s(80), s(36), s(83), s(48)], fill=mid)
    # pans: solid wedges, deep enough to survive a 3 px cell
    d.polygon([(s(4), s(46)), (s(32), s(46)), (s(24), s(62)), (s(12), s(62))], fill=hi)
    d.polygon([(s(4), s(46)), (s(32), s(46)), (s(30), s(51)), (s(6), s(51))], fill=lo)
    d.polygon([(s(68), s(46)), (s(96), s(46)), (s(88), s(62)), (s(76), s(62))], fill=mid)
    d.polygon([(s(68), s(46)), (s(96), s(46)), (s(94), s(51)), (s(70), s(51))], fill=lo)


def derive_auto(play_img, gap=0.10):
    """helper_auto from the approved play_2: the same graded triangle twice."""
    w, h = play_img.size
    tw = int(round(w * (1.0 - gap) / 2.0))
    th = max(1, int(round(h * tw / float(w))))
    one = play_img.resize((tw, th), Image.LANCZOS)
    step = tw + max(1, int(round(w * gap)))
    out = Image.new("RGBA", (step + tw, th), (0, 0, 0, 0))
    out.paste(one, (0, 0))
    out.paste(one, (step, 0))
    return out


def mute(img, slash=True):
    """Frame 1 of the voice glyphs: cold, dimmed metal plus a slash. Baked art,
    because at 14 px a half-brightness copy of the live glyph was not readable
    as 'off'."""
    im = img.copy()
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if not a:
                continue
            v = int(0.299 * r + 0.587 * g + 0.114 * b)
            v = int(v * 0.62)
            px[x, y] = (v, v, min(255, int(v * 1.04)), a)  # neutral, a touch cold
    if slash:
        d = ImageDraw.Draw(im)
        t = max(2, int(round(min(w, h) * 0.11)))
        d.line([(w * 0.10, h * 0.08), (w * 0.90, h * 0.92)],
               fill=(0x12, 0x11, 0x10, 255), width=t + max(2, t // 2))
        d.line([(w * 0.10, h * 0.08), (w * 0.90, h * 0.92)],
               fill=(0xC6, 0xB6, 0x99, 255), width=t)
    return im


# -------------------------------------------------------------- composing ---
def fit(glyph, box_w, box_h, outline_px=0.0):
    """Scale to fit the glyph box (in final pixels) at SS resolution, aspect
    preserved. The box includes the ink outline the caller will add."""
    w, h = glyph.size
    box_w -= 2.0 * outline_px
    box_h -= 2.0 * outline_px
    k = min(box_w * SS / w, box_h * SS / h)
    return glyph.resize((max(1, int(round(w * k))), max(1, int(round(h * k)))),
                        Image.LANCZOS)


def outlined(glyph_ss, outline_px):
    """Ring the silhouette with ink. A true dilation of the alpha, so the
    outline never bleeds inside the shape."""
    r = int(round(outline_px * SS))
    if r <= 0:
        return glyph_ss
    w, h = glyph_ss.size
    pad = Image.new("RGBA", (w + 2 * r, h + 2 * r), (0, 0, 0, 0))
    pad.paste(glyph_ss, (r, r))
    a = pad.split()[3].point(lambda v: 255 if v >= 128 else 0)
    d = a.filter(ImageFilter.MaxFilter(2 * r + 1))
    ring = Image.new("RGBA", pad.size, INK + (255,))
    ring.putalpha(d)
    ring.alpha_composite(pad)
    return ring


def place(base_ss, glyph_ss, w, h, dy=0.0):
    """Centre the glyph on the plate (both at SS resolution)."""
    gw, gh = glyph_ss.size
    ox = (w * SS - gw) // 2
    oy = (h * SS - gh) // 2 + int(round(dy * SS))
    out = base_ss.copy()
    out.alpha_composite(glyph_ss, (ox, oy))
    return out


def downsample(img_ss, w, h):
    """Area average, premultiplied so the rounded plate corners do not bleed
    black into the edge pixels."""
    import numpy as np

    a = np.asarray(img_ss, dtype=np.float64) / 255.0
    al = a[..., 3:4]
    pm = np.concatenate([a[..., :3] * al, al], axis=-1)
    pm = pm.reshape(h, SS, w, SS, 4).mean(axis=(1, 3))
    out_a = pm[..., 3:4]
    rgb = np.where(out_a > 1e-6, pm[..., :3] / np.maximum(out_a, 1e-6), 0.0)
    res = np.concatenate([rgb, out_a], axis=-1)
    return Image.fromarray((np.clip(res, 0, 1) * 255 + 0.5).astype("uint8"), "RGBA")


def frames_on_plate(glyph, w, h, box, outline_px, states, dy=0.0):
    g = outlined(fit(glyph, *box, outline_px=outline_px), outline_px)
    out = []
    for gain, warm in states:
        plate = plate_ss(w, h, gain, warm)
        out.append(downsample(place(plate, g, w, h, dy), w, h))
    return out


def frames_bare(glyphs, w, h, box, outline_px):
    out = []
    for glyph in glyphs:
        g = outlined(fit(glyph, *box, outline_px=outline_px), outline_px)
        base = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        out.append(downsample(place(base, g, w, h), w, h))
    return out


# ------------------------------------------------------------------- I/O ---
def write_ozt(path, frames):
    """frames: list of RGBA images (visual top-down). File rows are bottom-up."""
    w = frames[0].width
    total_h = sum(f.height for f in frames)
    blob = bytearray(22)
    blob[2] = 2
    blob[16:18] = struct.pack("<H", w)
    blob[18:20] = struct.pack("<H", total_h)
    blob[20] = 32
    blob[21] = 8

    rows = []
    for im in frames:
        d = im.tobytes()
        for y in range(im.height):
            rows.append(d[y * w * 4:(y + 1) * w * 4])
    rows.reverse()
    for row in rows:
        for x in range(w):
            i = x * 4
            blob += bytes((row[i + 2], row[i + 1], row[i], row[i + 3]))
    blob += b"\x00" * 26
    path.write_bytes(blob)
    print(f"{path.name:22s} {w}x{total_h}  {len(blob)} bytes")


def write_png(path, im):
    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    im = im.convert("RGBA")
    w, h = im.size
    src = im.tobytes()
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += src[y * w * 4:(y + 1) * w * 4]
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    Path(path).write_bytes(png)


# ------------------------------------------------------------- the icon set ---
# name -> (canva render, isolate() overrides)
STRIP_SRC = {
    "helper_settings": ("config_2", {}),
    "helper_play": ("play_2", {}),
    "helper_stop": ("stop_1", dict(ring=0.045)),
    "helper_market": (None, {}),      # drawn here, see _market
    "helper_auto": (None, {}),        # derived from helper_play
}
TOOLBAR_SRC = {
    # cashshop_3 is lit by a hard spotlight cone the polynomial cannot model,
    # and its export leaves a near-white margin in the corners. The white is
    # keyed out explicitly; the cone is cut by raising the matte threshold, and
    # what is left of it above the gem's apex is severed by the opening and
    # then dropped by keep_largest.
    "toolbar_cashshop": ("cashshop_3",
                         dict(t_lo=17.0, t_hi=27.0, open_px=3, keep_largest=True)),
    "toolbar_character": ("personagem_2", {}),
    "toolbar_inventory": ("inventario_2", {}),
    "toolbar_friends": ("amigos_1", {}),
    "toolbar_menu": ("menu_2", {}),
}
VOICE_SRC = {
    "voice_mic": ("microfone_3", {}),
    "voice_sound": ("som_2", {}),
}

# name -> (frame w, frame h, glyph box, outline) for the glyph-only renders
# the comparison tool measures (the plate is not part of the icon art).
GEOM = {}
for _k in STRIP_SRC:
    GEOM[_k] = (STRIP_W, STRIP_FRAME_H, STRIP_BOX, STRIP_OUTLINE)
for _k in TOOLBAR_SRC:
    GEOM[_k] = (TOOL_W, TOOL_FRAME_H, TOOL_BOX, TOOL_OUTLINE)
GEOM["voice_mic"] = (MIC_W, MIC_FRAME_H, MIC_BOX, MIC_OUTLINE)
GEOM["voice_sound"] = (SND_W, SND_FRAME_H, SND_BOX, SND_OUTLINE)

# (gain, warm) per frame. The helper strip has no fourth frame.
STRIP_STATES = ((GAIN_UP, 0.0), (GAIN_OVER, 0.0), (GAIN_DOWN, 0.0))
TOOL_STATES = ((GAIN_UP, 0.0), (GAIN_OVER, 0.0), (GAIN_DOWN, 0.0),
               (GAIN_ALERT, ALERT_SAT))


def install_reference():
    """Install the Webzen value distribution grade() matches against."""
    import io

    ozj = _ozj

    def ozt(p):
        im = Image.open(io.BytesIO(p.read_bytes()[4:]))
        im.load()
        return im.convert("RGBA")

    ref = []
    for n in ("01", "02", "03", "04"):
        col = ozj(IFACE / f"newui_menu_Bt{n}.OZJ").crop((0, 0, 38, 42))
        msk = ozt(IFACE / f"newui_menu_Bt{n}.OZT").crop((0, 0, 38, 42))
        a = msk.split()[0].load()
        p = col.load()
        for y in range(42):
            for x in range(38):
                if a[x, y] >= 128:
                    ref.append(p[x, y])
    b5 = ozj(IFACE / "partCharge1" / "newui_menu_Bt05.OZJ").crop((0, 0, 30, 41))
    ref += [c for c in b5.getdata() if max(c) >= 40 and min(c) <= 235]
    wg.set_reference_v([max(c) / 255.0 for c in ref])
    return ref


def build():
    install_reference()
    glyphs = {}
    for name, (src, kw) in list(STRIP_SRC.items()) + list(TOOLBAR_SRC.items()) \
            + list(VOICE_SRC.items()):
        if src:
            glyphs[name] = graded(src, **kw)
    glyphs["helper_market"] = synthetic(_market)
    glyphs["helper_auto"] = derive_auto(glyphs["helper_play"])

    out = {}
    for name in STRIP_SRC:
        out[name] = frames_on_plate(glyphs[name], STRIP_W, STRIP_FRAME_H,
                                    STRIP_BOX, STRIP_OUTLINE, STRIP_STATES)
    for name in TOOLBAR_SRC:
        out[name] = frames_on_plate(glyphs[name], TOOL_W, TOOL_FRAME_H,
                                    TOOL_BOX, TOOL_OUTLINE, TOOL_STATES)
    out["voice_mic"] = frames_bare(
        [glyphs["voice_mic"], mute(glyphs["voice_mic"])], MIC_W, MIC_FRAME_H,
        MIC_BOX, MIC_OUTLINE)
    out["voice_sound"] = frames_bare(
        [glyphs["voice_sound"], mute(glyphs["voice_sound"])], SND_W,
        SND_FRAME_H, SND_BOX, SND_OUTLINE)
    return out, glyphs


def glyph_only(glyphs):
    """Each icon's art alone at final size, no plate. This is what is
    comparable to the Webzen reference, whose OZT alpha also masks the glyph
    and not the frame around it."""
    out = {}
    for name, g in glyphs.items():
        w, h, box, ol = GEOM[name]
        out[name] = frames_bare([g], w, h, box, ol)[0]
    return out


def write_preview(sets):
    """Contact sheet: every frame at 1x and 4x over a HUD-dark ground."""
    bg = (0x14, 0x16, 0x1A, 255)
    pad, gap, zoom = 10, 6, 4
    cols = list(sets.items())
    colw = max(im.width for fr in sets.values() for im in fr) * (zoom + 1) + gap
    colh = max(sum(im.height for im in fr) for fr in sets.values()) * zoom
    sheet = Image.new("RGBA", (pad + len(cols) * (colw + gap), pad * 2 + colh), bg)
    for ci, (name, frames) in enumerate(cols):
        ox = pad + ci * (colw + gap)
        oy = pad
        for im in frames:
            sheet.alpha_composite(im, (ox, oy))
            big = im.resize((im.width * zoom, im.height * zoom), Image.NEAREST)
            sheet.alpha_composite(big, (ox + im.width + gap, oy))
            oy += im.height * zoom
    write_png(OUT / "_preview.png", sheet)
    print("preview:", OUT / "_preview.png")


# What the call sites read back. Asserted on every run: these numbers are
# baked into NewUIHeroPositionInfo, NewUIMainFrameWindow and VoiceIcons, and a
# silent change here is a silently broken HUD.
EXPECT = {k: (STRIP_W, STRIP_FRAME_H, STRIP_FRAMES) for k in STRIP_SRC}
EXPECT.update({k: (TOOL_W, TOOL_FRAME_H, TOOL_FRAMES) for k in TOOLBAR_SRC})
EXPECT["voice_mic"] = (MIC_W, MIC_FRAME_H, MIC_FRAMES)
EXPECT["voice_sound"] = (SND_W, SND_FRAME_H, SND_FRAMES)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    sets, _ = build()
    assert set(sets) == set(EXPECT), sorted(set(sets) ^ set(EXPECT))
    for name, frames in sets.items():
        w, fh, nf = EXPECT[name]
        assert len(frames) == nf, f"{name}: {len(frames)} frames, want {nf}"
        for f in frames:
            assert f.size == (w, fh), f"{name}: frame {f.size}, want {(w, fh)}"
        write_ozt(OUT / (name + ".OZT"), frames)
    write_preview(sets)


if __name__ == "__main__":
    main()
