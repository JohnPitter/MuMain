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
  helper     the black gutter and the GOLD RULE at 1:1 (rows 0 and 1), then
             the plate below the rule (slot rows 2..40) box-filtered into the
             remaining 11 rows.

The helper strip gets its life back (v8)
----------------------------------------
v7 shipped a helper strip that was correct and dead. Measured on the shipped
v7 bytes: the plate was exactly neutral on every row (mean R-B = 0.0, no gold
rule anywhere), the whole 18x13 frame carried 8 to 33 pixels of real warmth
(R-B >= 8) out of 234, the glyph highlights landed at p88 = 0.51 to 0.62 of
full value against the toolbar's 0.60 to 0.71, and the glyph/plate luminance
ratio was 1.35 (settings) to 1.71 (play) against the toolbar's 1.82 to 2.65.
Four things were wrong and all four are measurable.

1. NO GOLD RULE. v7 dropped it on the grounds that the Minimap_position bar
   has none. It does, in fact, have a rule -- Minimap_positionB row 1 is a
   flat neutral grey 127 running the width of the bar, and the 13 px buttons
   sit right on top of it. So the rule is not an invention, only a recolour of
   a line that is already there, at the row it is already on. It is lifted
   from the same shipped gold rule the toolbar plate carries, but sampled at
   x=96 where the rule is flat and bright (L~190) instead of at x=0 where it
   is the bar's dim left end cap: the five helper buttons are drawn edge to
   edge at an 18 px pitch, so the end cap's 0->161 ramp would have tiled into
   a sawtooth. Same pixels, a representative slice of them.

2. NO WARM ACCENT. grade() locks hue to 41.5 deg and reads saturation off the
   measured S(V) curve, so warmth is a function of value -- and the strip's
   art, being a fifth of the toolbar's area, box-filters far enough down the
   curve that the brass never arrives. warm_accent() moves the focal pixels of
   each glyph UP that same curve (it adds exactly sat_at(v + shift) -
   sat_at(v), no new colour model) at the gear's rim and teeth, the whole play
   triangle, both auto triangles and the market pans. helper_stop is left cool
   on purpose: it is the "stop", and warm reads as "go".

3. NOT ENOUGH CONTRAST. top_light() lifts the upper half of the glyph and
   settles the lower half, and the plate is taken down by STRIP_PLATE_GAIN.

4. NOT SHARP ENOUGH. On a 14x9 face a half-covered pixel is a wasted pixel, so
   the strip composite is snapped to the pixel grid: the outline is one whole
   final pixel (STRIP_OUTLINE 1.0, i.e. exactly SS at supersample resolution),
   the fitted glyph is rounded to a whole number of final pixels and the paste
   offset is a multiple of SS. Every silhouette and ink edge therefore lands
   on a pixel boundary and the area average stops smearing them across two.
   The toolbar keeps the unsnapped path -- at 26x37 the box filter is an
   anti-aliaser, not a blur, and snapping there would only cost subpixel
   placement.

The voice glyphs get the same treatment (v9)
--------------------------------------------
Same recipe as v8, on voice_mic and voice_sound. Four of the six things v8 did
transfer unchanged; two cannot, and it is worth writing down which and why,
because the reason is structural rather than a matter of taste.

WHAT TRANSFERS

- The focal brass, on the two parts the owner named: the microphone's grille
  and the speaker's waves, with the handle and the cone left as steel. Both
  regions are read off the graded alpha rather than eyeballed. voice_mic's
  bbox is 74x164 and its row-occupancy profile is a ball 46..69 px wide down to
  row 58 (t = 0.36), a 4..20 px collar to row 88, then a handle that holds
  14..19 px to the bottom: so band(-0.10, 0.36) is the head and band(0.38,
  1.10) is the handle. voice_sound's bbox is 158x111 and its column profile
  ramps 1..109 px over columns 0..90 (the cone, t <= 0.575), drops to exactly
  zero for columns 91..115, and returns as two arcs over 116..157 (t >= 0.735):
  so column(-0.10, 0.68) is the cone and column(0.62, 1.10) is the waves, both
  feathers landing inside the 25 empty columns.

  Both glyphs need cool_down() as well as warm_accent(), which helper_stop did
  not: microfone_3 and som_2 arrive with the tan on the wrong half (the mic's
  handle, the speaker's cone) and warming the right half alone would have left
  two warm areas instead of moving the one. cool_down() therefore takes the
  region parameter warm_accent() already had. helper_stop passes none and is
  byte-identical.

- match_highlights() against the toolbar's own p88, the same number in the same
  run. The voice glyphs came in at 0.639 (mic) and 0.609 (sound) against the
  toolbar's 0.672 -- a smaller gap than the strip's 0.51..0.62, because these
  are 11x19 and 20x15 rather than 12x8, but the same gap.

- top_light(), same 0.10.

- The pixel-grid snap. The outline was already one whole final pixel here, but
  the fit and the paste were not: voice_mic came out 85x168 at SS, i.e. 10.6
  final pixels wide pasted at x = 2.625, so every vertical edge of a 16 px icon
  was averaged across two columns. Snapped it is 11 px at x = 2. This matters
  more here than the geometry suggests: of the three call sites, Chat.cpp and
  VoiceSpeakingIndicator draw these at scale 1.0, i.e. genuinely 1:1, and only
  MiniMapCorner's HUD button scales them (0.80, GL_LINEAR) -- and a bilinear
  resample of a hard edge still beats one of an edge that was pre-blurred.

WHAT DOES NOT, AND WHY

- THE GOLD RULE. Not dropped for the reason v7 dropped the strip's; dropped
  because there is nothing here to put it on. The voice glyphs are the only two
  icons in this file that are frames_bare() -- they carry no plate, and the
  plate they sit on is Interface/newui_btn_empty_very_small.OZT, which the game
  draws itself. That art measures exactly as neutral as Minimap_positionB did
  (mean R-B = 0.00 on all 23 rows, 0 warm pixels out of 784) and it carries its
  own rail at row 1 (flat grey, mean L = 112.5, peak 186) -- so the v8 argument
  "this is a recolour of a line that is already there" holds. What does not
  hold is the ability to make it: that OZT is a shipped shared asset, read by
  the castle, duel, guard, gateman, marketplace, auto-battler and cursed-temple
  windows among others, so recolouring the rule recolours all of them. Baking
  one into the voice OZT instead does not work either -- MiniMapCorner draws
  the frame centred at 0.80, which puts OZT row 0 at button row 2.30 (mic) and
  4.70 (sound), i.e. a soft second line at a fractional offset a pixel or four
  BELOW the sharp one that is already there -- and the same frame is drawn in
  the world by VoiceSpeakingIndicator with no button behind it at all, where a
  gold line would be a gold line floating over the terrain.

- THE 10% PLATE DROP, for the same reason: no plate of ours to drop. Its
  purpose -- glyph sitting ON the plate rather than IN it -- is a contrast
  ratio, and the only lever we own for it is raising the glyph, which is what
  match_highlights() already does against the same toolbar target.

One thing v8 did not have to think about: MUTE_SLASH. mute() rewrites the body
to neutral-cold luminance, so the accent's hue is gone from frame 1 by
construction and "muted is cold" needs no second set of numbers -- except that
the slash itself was drawn 0xC6B699, a tan bar, and was therefore the only warm
thing left in either muted frame. It is now neutral at the same luminance.

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

# The helper plate keeps those two rows at 1:1, but takes the gold from a flat
# stretch of the SAME rule instead of the bar's dim left end cap -- see the
# module docstring. Row 1 of newui_menu01.OZJ is L~190 and flat from x=64 to
# x=128; x=96 is the middle of that.
STRIP_GOLD_SRC_X = 96
# The lifted rule is brighter than the toolbar's slice of it (L~190 vs ~161),
# which on an 18 px face would out-shout the glyph. Trimmed to sit just above
# the toolbar's own rule rather than a third above it.
STRIP_GOLD_GAIN = 0.86

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
# STRIP_* was (13.0, 9.0) / 0.7 through v7. The outline is now exactly one
# final pixel (0.7 * SS = 5.6, i.e. three quarters of a pixel smeared across
# two) and the box grew with it, so the glyph inside is 14 - 2*1 = 12 wide
# against the old 13 - 2*0.7 = 11.6: wider, not narrower, which is what the
# two glyphs at the edge of legibility (settings, market) needed. The box
# stays 9 tall and STRIP_DY drops it a row, clear of the gold rule, which
# leaves a dark row of plate above and below the ink instead of running the
# glyph into the rule and into the bottom bracket.
STRIP_BOX, STRIP_OUTLINE = (14.0, 9.0), 1.0
STRIP_DY = 1.0
# The plate is taken down a touch so the glyph sits on it rather than in it.
# Applied on top of the measured per-frame gains, not instead of them.
STRIP_PLATE_GAIN = 0.90
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


def gold_rule(w):
    """One row of the shipped gold top rule, w px wide, taken from the flat
    part of it.

    The helper buttons are drawn edge to edge at an 18 px pitch, so a slice of
    the bar's left end cap -- which is what the slot itself carries, ramping
    0 -> 161 -- would tile into a sawtooth with a black notch every 18 px. The
    same rule at x=96 is flat (L~190 from x=64 to x=128), so five of them in a
    row read as one line.
    """
    import numpy as np

    bar = np.asarray(_ozj(IFACE / "newui_menu01.OZJ"), dtype=np.float64)
    x0 = STRIP_GOLD_SRC_X
    return bar[1, x0:x0 + w].copy() * STRIP_GOLD_GAIN


def plate_rgb(w, h):
    """The plate at the frame size, before shading.

    Toolbar (30x41): the slot is already 41 tall, so only the width changes,
    and it changes by dropping columns rather than by resampling.
    Helper strip (18x13): row 0 is the slot's black gutter and row 1 the gold
    rule, both at 1:1 -- the Minimap_position bar carries a rule of its own at
    exactly that row (a flat grey 127) and the buttons cover it, so this is a
    recolour of a line that is there, not a new one. The plate below the rule
    (slot rows 2..40) is box-filtered into the 11 rows that are left.
    """
    import numpy as np

    a = slot_art()
    if h == a.shape[0]:
        return _narrow(a, w)
    head = _narrow(a[:SLOT_GOLD_RULE], w)
    head[1] = gold_rule(w)
    body = a[SLOT_GOLD_RULE:]
    im = Image.fromarray(np.clip(body, 0, 255).astype("uint8"))
    body = np.asarray(im.resize((w, h - SLOT_GOLD_RULE), Image.BOX),
                      dtype=np.float64)
    return np.concatenate([head, body * STRIP_PLATE_GAIN], axis=0)


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


# The muted slash. Was 0xC6B699 -- a tan bar, i.e. R-B = +45, and on the
# shipped v8 bytes it was the ONLY warm thing left in either muted frame (28 of
# voice_mic[1]'s 160 lit pixels and 37 of voice_sound[1]'s 216, since the body
# loop below rewrites every other pixel to a neutral-cold grey). Warm reads as
# "go" -- that is the whole argument helper_stop is cooled on -- so a warm bar
# across an off glyph said the opposite of what it is there to say. It is now
# neutral at the SAME luminance (0.299R + 0.587G + 0.114B = 183.5 either way,
# so nothing is lost on the one axis that carries the reading at 14 px) with
# the same slight blue tilt the body gets.
MUTE_SLASH = (0xB7, 0xB7, 0xBE)


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
               fill=MUTE_SLASH + (255,), width=t)
    return im


# ------------------------------------------------------------ warm accent ---
def _unit_chroma():
    """The unit-saturation RGB direction of the Webzen highlight hue, used to
    give a perfectly neutral pixel a hue to be saturated along."""
    import colorsys

    import numpy as np

    return 1.0 - np.array(colorsys.hsv_to_rgb(wg.WZ_HUE, 1.0, 1.0))


def _smoothstep(t):
    import numpy as np

    t = np.clip(t, 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def annulus(inner=0.45, outer=0.68):
    """Weight 1 on the rim of the glyph's bounding box, 0 in the middle: the
    gear's aro and teeth, not its hub."""
    def f(nx, ny):
        import numpy as np

        return _smoothstep((np.hypot(nx, ny) - inner) / max(1e-6, outer - inner))
    return f


def band(y0, y1, feather=0.10):
    """Weight 1 inside a horizontal band of the bounding box (ny 0 = top)."""
    def f(nx, ny):
        t = (ny + 1.0) * 0.5
        return _smoothstep((t - y0) / feather) * _smoothstep((y1 - t) / feather)
    return f


def column(x0, x1, feather=0.10):
    """band()'s mirror: weight 1 inside a VERTICAL band of the bounding box
    (nx 0 = left).

    voice_sound is the one glyph whose two parts separate in x rather than in
    y -- the speaker cone occupies columns 0..90 of the 158 px bbox and the two
    wave arcs columns 116..157, with 25 empty columns between them -- so the
    same smoothstep with the same feather, read off the other axis.
    """
    def f(nx, ny):
        t = (nx + 1.0) * 0.5
        return _smoothstep((t - x0) / feather) * _smoothstep((x1 - t) / feather)
    return f


def _shade(glyph, sat_delta=None, vscale=None):
    """Rewrite a graded glyph's saturation and/or value in place-ish.

    Hue is never touched: grade() already locked it to 41.5 deg, and a pixel
    that came out perfectly neutral is saturated along that same hue's unit
    chroma direction rather than given a hue of its own.
    """
    import numpy as np

    a = np.asarray(glyph, dtype=np.float64) / 255.0
    rgb, al = a[..., :3].copy(), a[..., 3:4]
    v = rgb.max(axis=-1)
    mn = rgb.min(axis=-1)
    s = np.where(v > 1e-9, (v - mn) / np.maximum(v, 1e-9), 0.0)

    # unit chroma direction per pixel, falling back to the Webzen hue
    k = np.where(v[..., None] > 1e-9, 1.0 - rgb / np.maximum(v[..., None], 1e-9), 0.0)
    ku = np.where(s[..., None] > 1e-4, k / np.maximum(s[..., None], 1e-9),
                  _unit_chroma()[None, None, :])

    h, w = v.shape
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
    nx = (xx / max(1, w - 1)) * 2.0 - 1.0
    ny = (yy / max(1, h - 1)) * 2.0 - 1.0

    if vscale is not None:
        v = np.clip(v * vscale(nx, ny), 0.0, wg.WZ_V_HI)
    s_new = s if sat_delta is None else np.clip(s + sat_delta(v, nx, ny),
                                               0.0, wg.WZ_SAT_CAP)
    out = np.clip(v[..., None] * (1.0 - ku * s_new[..., None]), 0.0, 1.0)
    return Image.fromarray(
        (np.concatenate([out, al], axis=-1) * 255 + 0.5).astype("uint8"), "RGBA")


def warm_accent(glyph, shift=0.26, region=None):
    """Push the glyph's focal pixels UP the measured Webzen S(V) ramp.

    No new colour model: the added saturation is exactly the ramp's own
    difference between this pixel's value and a value `shift` higher, so a lit
    pixel picks up the brass the toolbar's own highlights have and a shadow
    pixel -- where the measured curve is 1.8% -- stays neutral. That split is
    the whole point of the grade and it survives here.
    """
    import numpy as np

    sat_v = np.vectorize(wg._sat_at)

    def delta(v, nx, ny):
        d = (sat_v(np.minimum(1.0, v + shift)) - sat_v(v)) * wg.SAT_TRIM
        return d if region is None else d * region(nx, ny)

    return _shade(glyph, sat_delta=delta)


def cool_down(glyph, amount=0.55, region=None):
    """The opposite, for helper_stop: pull the warmth back out so it reads as
    clear silver. "Stop" in a warm brass is a mixed message, and stop was in
    fact the warmest of the five (10.5% mean saturation against play's 5.8).

    `region` is warm_accent()'s parameter with warm_accent()'s meaning, and it
    exists for the same reason the accent needs one. On the helper strip a
    glyph is either the "go" or it is not, so cooling is whole-glyph and the
    default None reproduces v8 byte for byte. The two voice glyphs are not like
    that: each is ONE object with two parts, and the owner's direction puts the
    brass on one part and steel on the other. Warming the focal part alone is
    only half of that -- microfone_3 and som_2 arrive from Canva with the
    warmth already on the WRONG part (the mic's handle is tan and its grille
    silver; the speaker's cone is tan and its waves silver), so the steel half
    has to be cooled as explicitly as the brass half is warmed, or the accent
    just adds a second warm area next to the one that should not be there.
    """
    import numpy as np

    sat_v = np.vectorize(wg._sat_at)

    def delta(v, nx, ny):
        d = -amount * sat_v(v) * wg.SAT_TRIM
        return d if region is None else d * region(nx, ny)

    return _shade(glyph, sat_delta=delta)


def top_light(glyph, amount=0.10):
    """One warm key light from above: lift the top of the glyph, settle the
    bottom. The Webzen art has exactly this and the v7 strip had none."""
    def vs(nx, ny):
        return 1.0 + amount * -ny
    return _shade(glyph, vscale=vs)


HL_PCT = 88.0        # "the highlights" = the 88th percentile of HSV value


def highlight_level(img, pct=HL_PCT):
    """Where this art's highlights sit, on the pixels that actually ship."""
    import numpy as np

    a = np.asarray(img, dtype=np.float64) / 255.0
    v = a[..., :3].max(axis=-1)[a[..., 3] >= 0.5]
    return float(np.percentile(v, pct)) if v.size else 0.0


def match_highlights(glyph, geom, target, pct=HL_PCT, iters=2,
                     lo=0.70, hi=1.70):
    """Scale the glyph's value so that, AT FINAL SIZE, its highlights land
    where the toolbar's do.

    grade() matches the value histogram exactly -- at 240 px. The strip then
    throws away 99.8% of those pixels, and what a box filter leaves behind is
    a cell mean, which for a posterized distribution sits well inside the band
    rather than at its ends. Measured on the v7 art the strip's highlights
    came out at p88 = 0.51 to 0.62 of full value against the toolbar's 0.60 to
    0.71, and that, not the hue, was most of why the strip read flat.

    The target is not a taste number and not the raw Webzen ceiling either
    (WZ_V_HI is a p100; aiming the strip at it overshoots every toolbar glyph
    by a fifth). It is the toolbar's OWN measured level, computed from the
    toolbar renders in the same run -- so the strip is normalised to the art
    it has to sit next to, and if the toolbar ever changes the strip follows.

    Iterated, because the map from source value to final value is not a
    scaling: the ink ring is a fixed 0x11 and it is half of a 12x8 glyph, so
    one step lands short. Clamped both ways -- this equalises, it does not
    rescue a glyph that is wrong.
    """
    w, h, box, ol = geom
    total = 1.0
    for _ in range(max(1, iters)):
        probe = frames_bare([glyph], w, h, box, ol, snap=True)[0]
        p = highlight_level(probe, pct)
        if p <= 1e-6:
            break
        g = min(hi / total, max(lo / total, target / p))
        if abs(g - 1.0) < 0.01:
            break
        total *= g
        glyph = _shade(glyph, vscale=lambda nx, ny, _g=g: _g)
    return glyph


# -------------------------------------------------------------- composing ---
def fit(glyph, box_w, box_h, outline_px=0.0, snap=False):
    """Scale to fit the glyph box (in final pixels) at SS resolution, aspect
    preserved. The box includes the ink outline the caller will add.

    With `snap`, the result is a whole number of FINAL pixels in both axes, so
    that the silhouette's bounding edges land on pixel boundaries instead of
    being averaged across two. Costs up to half a final pixel of aspect; buys
    back a hard edge, which on a 14x9 face is the better trade.
    """
    w, h = glyph.size
    box_w -= 2.0 * outline_px
    box_h -= 2.0 * outline_px
    k = min(box_w * SS / w, box_h * SS / h)
    tw, th = max(1, int(round(w * k))), max(1, int(round(h * k)))
    if snap:
        tw = max(SS, int(round(tw / float(SS))) * SS)
        th = max(SS, int(round(th / float(SS))) * SS)
    return glyph.resize((tw, th), Image.LANCZOS)


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


def place(base_ss, glyph_ss, w, h, dy=0.0, snap=False):
    """Centre the glyph on the plate (both at SS resolution).

    With `snap` the offset is a whole number of final pixels, which is what
    actually makes the snapped fit() pay off: a pixel-sized glyph landed on a
    half-pixel offset is exactly as blurred as an unsnapped one.
    """
    gw, gh = glyph_ss.size
    ox = (w * SS - gw) // 2
    oy = (h * SS - gh) // 2 + int(round(dy * SS))
    if snap:
        ox = int(round(ox / float(SS))) * SS
        oy = int(round(oy / float(SS))) * SS
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


def frames_on_plate(glyph, w, h, box, outline_px, states, dy=0.0, snap=False):
    g = outlined(fit(glyph, *box, outline_px=outline_px, snap=snap), outline_px)
    out = []
    for gain, warm in states:
        plate = plate_ss(w, h, gain, warm)
        out.append(downsample(place(plate, g, w, h, dy, snap=snap), w, h))
    return out


def frames_bare(glyphs, w, h, box, outline_px, snap=False):
    out = []
    for glyph in glyphs:
        g = outlined(fit(glyph, *box, outline_px=outline_px, snap=snap),
                     outline_px)
        base = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        out.append(downsample(place(base, g, w, h, snap=snap), w, h))
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

# The focal accent per strip glyph. The toolbar needs none: at 26x37 the
# grade's own warm highlights survive the downsample, which is exactly what
# they stop doing at 12x8.
#   settings  the aro and the teeth, not the hub -- a gear lit all over is a
#             blob at this size, lit only on the rim it still reads as a gear.
#   play      the whole triangle. It is one shape and it is the "go".
#   auto      inherited: derive_auto() copies the already-warmed play.
#   market    the pans and what is in them.
#   stop      cooled instead. Warm reads as "go".
STRIP_ACCENT = {
    "helper_settings": lambda g: warm_accent(g, 0.28, annulus(0.42, 0.70)),
    "helper_play": lambda g: warm_accent(g, 0.26),
    # 0.85 rather than a token amount: stop_1 is a framed window whose top
    # edge is the source's own lit rim, and at 12x7 that rim is the only thing
    # left of the frame -- a tan bar across the top of a silver square, which
    # reads as grime, not as light.
    "helper_stop": lambda g: cool_down(g, 0.85),
    "helper_market": lambda g: warm_accent(g, 0.30, band(0.40, 0.74)),
}
STRIP_TOP_LIGHT = 0.10

# The same accent for the two voice glyphs, on the two parts the owner named:
# the brass on the microphone's grille and on the speaker's waves, steel on the
# handle and on the cone. Both regions are MEASURED off the graded alpha, not
# guessed -- the row/column occupancy profiles are in the module docstring.
#
#   voice_mic     the head is rows 0..58 of the 74x164 bbox, i.e. t 0.00..0.36,
#                 and it is one solid ball: a band, not an annulus, because
#                 unlike the gear there is no hub to keep dark. y0 is negative
#                 so band()'s 0.10 feather finishes ramping BEFORE the crown
#                 instead of on it -- the top of the ball is the lit part and
#                 fading it in would put the accent's own edge across the
#                 highlight. The handle (t >= 0.38, a steady 14..19 px wide) is
#                 cooled; the collar between them, t 0.36..0.48, is where the
#                 two feathers cross, which is exactly where the object's own
#                 silhouette narrows.
#   voice_sound   the arcs are columns 116..157 of the 158x111 bbox, t >= 0.735,
#                 and the cone is columns 0..90, t <= 0.575. The 25 empty
#                 columns between them (t 0.58..0.72) mean both feathers land
#                 in transparent space: the cone is fully steel and both arcs
#                 fully brass, with no gradient across either. That gap is why
#                 this glyph can take a hard split and the mic cannot.
#
# Frame 1 needs no entry. mute() rewrites every pixel to its own luminance, so
# whatever the accent did to the hue is gone by construction -- "muted is cold"
# is structural here rather than a second set of numbers to keep in step.
VOICE_ACCENT = {
    "voice_mic": lambda g: cool_down(
        warm_accent(g, 0.28, band(-0.10, 0.36)), 0.70, band(0.38, 1.10)),
    "voice_sound": lambda g: cool_down(
        warm_accent(g, 0.30, column(0.62, 1.10)), 0.70, column(-0.10, 0.68)),
}
VOICE_TOP_LIGHT = 0.10

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
    # Accent BEFORE deriving auto, so its two triangles are the warmed play
    # rather than a second, differently coloured one.
    for name, fn in STRIP_ACCENT.items():
        glyphs[name] = fn(glyphs[name])
    glyphs["helper_auto"] = derive_auto(glyphs["helper_play"])
    # Where the toolbar's highlights land once it is down at final size. The
    # strip is normalised to this, so "as bright as the menus" is a
    # measurement rather than an opinion.
    target = sum(highlight_level(frames_bare([glyphs[n]], *GEOM[n][:2],
                                             GEOM[n][2], GEOM[n][3])[0])
                 for n in TOOLBAR_SRC) / len(TOOLBAR_SRC)
    for name in STRIP_SRC:
        glyphs[name] = top_light(
            match_highlights(glyphs[name], GEOM[name], target),
            STRIP_TOP_LIGHT)
    # The voice glyphs go through the same three steps against the same
    # measured target. They are normalised to the toolbar rather than to each
    # other for the same reason the strip is: they are read next to it.
    for name in VOICE_SRC:
        glyphs[name] = top_light(
            match_highlights(VOICE_ACCENT[name](glyphs[name]), GEOM[name],
                             target),
            VOICE_TOP_LIGHT)

    out = {}
    for name in STRIP_SRC:
        out[name] = frames_on_plate(glyphs[name], STRIP_W, STRIP_FRAME_H,
                                    STRIP_BOX, STRIP_OUTLINE, STRIP_STATES,
                                    dy=STRIP_DY, snap=True)
    for name in TOOLBAR_SRC:
        out[name] = frames_on_plate(glyphs[name], TOOL_W, TOOL_FRAME_H,
                                    TOOL_BOX, TOOL_OUTLINE, TOOL_STATES)
    out["voice_mic"] = frames_bare(
        [glyphs["voice_mic"], mute(glyphs["voice_mic"])], MIC_W, MIC_FRAME_H,
        MIC_BOX, MIC_OUTLINE, snap=True)
    out["voice_sound"] = frames_bare(
        [glyphs["voice_sound"], mute(glyphs["voice_sound"])], SND_W,
        SND_FRAME_H, SND_BOX, SND_OUTLINE, snap=True)
    return out, glyphs


def glyph_only(glyphs):
    """Each icon's art alone at final size, no plate. This is what is
    comparable to the Webzen reference, whose OZT alpha also masks the glyph
    and not the frame around it."""
    out = {}
    for name, g in glyphs.items():
        w, h, box, ol = GEOM[name]
        out[name] = frames_bare([g], w, h, box, ol,
                                snap=name in STRIP_SRC or name in VOICE_SRC)[0]
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
