"""Turn a Canva icon render into Webzen-finish MU interface art.

Why this module exists
----------------------
The Canva sheet gives good, readable silhouettes with the wrong finish: a 3D
render on a black card -- strong gradient, a specular hotspot, a projected
shadow, saturated brass. The shipped Webzen art is the opposite: flat 2D, one
warm light, no specular, low contrast, matte and slightly dirty. This module
measures the second and forces the first into it.

The Webzen reference, measured (tools/../scratchpad/wz/measure_temp.py)
-----------------------------------------------------------------------
newui_menu_Bt01..04 composited through their real OZT alpha, plus
partCharge1/newui_menu_Bt05, 1040 opaque pixels:

    luma bin      n    sat%    R-B   hue   hue-concentration
      0- 50     149    2.46   +0.6  44.2   0.40
     51-101     259    3.10   +2.4  46.2   0.60
    102-152     328    6.51   +9.8  42.3   0.95
    153-203     304   17.84  +37.0  42.0   1.00
    ALL        1040    8.39  +14.6

Two facts fall out of that table and they decide the whole grade.

1. THE GREYS ARE WARM, BUT ONLY IN THE LIGHT.  An earlier analysis concluded
   the greys are strictly neutral (saturation 0.00) and it was not wrong about
   what it looked at -- it averaged the frame plates, whose pixels are 78%
   below luma 102, where saturation really is 0.40%. Composite through the real
   alpha and the picture changes: saturation climbs monotonically with
   luminance, 2.5% -> 3.1% -> 6.5% -> 17.8%.  Shadows neutral, highlights warm
   is what a warm key light on grey metal does, and it is what the dominant
   palette shows: #373737 and #2A2A2A are exactly neutral (R-B = 0) while
   #918E87 (+10), #A9A49A (+15) and #D2BF95 (+61) are not.
2. IT IS NOT JPEG NOISE.  OZJ is JPEG, so a warm bias could have been chroma
   ringing -- except ringing has no preferred hue. The hue concentration of the
   lit bins is 0.95..1.00 around 41..42 deg, i.e. a single hue, and a 4x box
   downsample (which averages 8x8 DCT ringing away) keeps hue 41..44 with
   concentration 0.74..1.00. The chroma is in the art, not in the codec.

So the target is a one-hue warm ramp at 41 deg whose saturation is a function
of value, not a flat grey and not a flat brass:

    V%     0-20  20-40  40-60  60-70  70-80  80-90
    S%      1.8    2.6    4.2    7.2   24.1   33.1

and a deliberately compressed tonal range -- the Webzen pixels run V 15.7
(p1) to 87.1 (p100). Nothing is black and NOTHING IS WHITE: the absence of a
specular is measurable, not a matter of taste.
"""
from __future__ import annotations

import colorsys
from collections import deque

import numpy as np
from PIL import Image, ImageFilter

# ------------------------------------------------------- measured constants ---
WZ_HUE = 41.5 / 360.0          # circular-mean hue of every lit Webzen bin
WZ_HUE_SPREAD = 7.0 / 360.0    # how much of the source hue variation survives

# saturation as a function of HSV value, from the decile table above
WZ_SAT_CURVE = [
    (0.00, 0.018), (0.20, 0.019), (0.30, 0.023), (0.40, 0.031),
    (0.50, 0.042), (0.60, 0.045), (0.70, 0.075), (0.80, 0.245),
    (0.90, 0.331), (1.00, 0.360),
]
WZ_SAT_CAP = 0.52              # p99 of the Webzen distribution is 50.5%
# V_LIFT (below) moves pixels up into the warm end of the S(V) curve, which
# overshoots the 8.39% target by about a point. Trimmed back here rather than
# by bending the measured curve.
SAT_TRIM = 0.86
WZ_V_LO, WZ_V_HI = 0.157, 0.871  # p1 / p100 of the Webzen value distribution

POSTER_LEVELS = 9              # "2D chapado": few value steps, no smooth ramp
# The histogram match is exact at authoring resolution, but the shipped pixel
# is not the authoring pixel: the ink outline is about a fifth of a 30x41 icon
# and every silhouette edge box-filters into it, which drags the whole
# distribution down ~13 points of value. This gamma lift is measured against
# the FINAL art (tools/compare_hud_icons.py), which is what the player sees.
V_LIFT = 0.74
FLATTEN = 0.62                 # how much of the broad render shading is removed
GRIME = 0.030                  # matte/dirty amplitude (multiplicative on V)


def _sat_at(v):
    c = WZ_SAT_CURVE
    if v <= c[0][0]:
        return c[0][1]
    for i in range(1, len(c)):
        if v <= c[i][0]:
            v0, s0 = c[i - 1]
            v1, s1 = c[i]
            t = (v - v0) / (v1 - v0)
            return s0 + (s1 - s0) * t
    return c[-1][1]


# ---------------------------------------------------------- background kill ---
def _lab(arr):
    """L*a*b* (D65) for an (h, w, 3) uint8 array. Only distances matter."""
    c = arr.astype(np.float64) / 255.0
    X = 0.4124 * c[..., 0] + 0.3576 * c[..., 1] + 0.1805 * c[..., 2]
    Y = 0.2126 * c[..., 0] + 0.7152 * c[..., 1] + 0.0722 * c[..., 2]
    Z = 0.0193 * c[..., 0] + 0.1192 * c[..., 1] + 0.9505 * c[..., 2]

    def f(t):
        return np.where(t > 0.008856, np.cbrt(np.maximum(t, 1e-12)),
                        7.787 * t + 16.0 / 116.0)

    fx, fy, fz = f(X / 0.9505), f(Y), f(Z / 1.089)
    return np.stack([116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)], axis=-1)


def _basis(x, y):
    """Bi-cubic 2D polynomial terms, x/y normalised to [-1, 1]."""
    return np.stack([np.ones_like(x), x, y, x * x, x * y, y * y,
                     x ** 3, x * x * y, x * y * y, y ** 3], axis=-1)


def isolate(im, t_lo=7.0, t_hi=15.0, white_bg=236, ring=0.07, passes=4,
            open_px=0, hole_max=0.10, keep_largest=False):
    """Return an alpha mask (PIL 'L') isolating the object from the Canva card.

    The card is never a flat colour: these renders sit on vignettes, spotlight
    cones and projected shadows, so a colour key eats the object's dark side
    and keeps the shadow. A connectivity flood fill is worse -- an
    anti-aliased edge is a chain of small steps, so the fill walks straight
    into the object and hollows it out.

    Instead the backdrop is MODELLED: a bi-cubic polynomial per channel fitted
    to the border ring by iteratively reweighted least squares, so a vignette
    or a top spotlight is reproduced across the whole frame while object
    pixels that intrude into the ring are rejected as outliers. Alpha is then
    a smoothstep on the Lab distance between each pixel and the modelled
    backdrop underneath it -- no connectivity, nothing to leak through.
    """
    w, h = im.size
    rgb = np.asarray(im.convert("RGB"), dtype=np.uint8)
    lab = _lab(rgb)

    yy, xx = np.mgrid[0:h, 0:w]
    nx = (xx / (w - 1.0)) * 2.0 - 1.0
    ny = (yy / (h - 1.0)) * 2.0 - 1.0
    B = _basis(nx, ny)

    m = max(2, int(round(min(w, h) * ring)))
    sel = np.zeros((h, w), dtype=bool)
    sel[:m, :] = sel[-m:, :] = True
    sel[:, :m] = sel[:, -m:] = True
    # the near-white export margin (the cashshop_3 corner bug) is not backdrop
    sel &= rgb.min(axis=2) < white_bg

    fit = sel.copy()
    coef = None
    for _ in range(passes):
        A = B[fit]
        coef = np.linalg.lstsq(A, lab[fit], rcond=None)[0]
        pred = B @ coef
        res = np.linalg.norm(lab - pred, axis=-1)
        keep = res[sel]
        cut = np.percentile(keep, 72.0)
        fit = sel & (res <= max(cut, 1e-6))
        if fit.sum() < 64:
            fit = sel
            break

    dist = np.linalg.norm(lab - (B @ coef), axis=-1)
    a = np.clip((dist - t_lo) / max(1e-6, t_hi - t_lo), 0.0, 1.0)
    a = a * a * (3.0 - 2.0 * a)  # smoothstep
    a[rgb.min(axis=2) >= white_bg] = 0.0

    # `a` is already a decent matte; it just has interior holes (a shadowed
    # face that happens to match the card) and stray crumbs. Solidify with a
    # hole-filled, speck-free binary and use that binary, dilated, as a gate
    # so the soft edge survives but the crumbs do not.
    mask = Image.fromarray((a * 255.0 + 0.5).astype(np.uint8), "L")
    mask = mask.point(lambda v: 255 if v >= 128 else 0)
    if open_px:
        k = 2 * open_px + 1
        # morphological opening severs the wispy spotlight plume that the
        # polynomial cannot model (cashshop) without touching the body
        mask = mask.filter(ImageFilter.MinFilter(k)).filter(ImageFilter.MaxFilter(k))
    mask = _drop_specks(mask, 1.0 if keep_largest else 0.004)
    mask = _fill_holes(mask, hole_max)
    hard = np.asarray(mask, dtype=np.float64) / 255.0
    gate = np.asarray(mask.filter(ImageFilter.MaxFilter(5)),
                      dtype=np.float64) / 255.0
    out = np.clip(np.maximum(a * gate, hard), 0.0, 1.0)
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8), "L")


def _fill_holes(mask, hole_max=0.10):
    """Close enclosed gaps that are only there because the shaded interior
    happened to match the card (the dark bars inside the menu plate), but keep
    the ones that are part of the drawing: a hole bigger than `hole_max` of the
    object is a real bore, like the gear's, and gets the ink outline instead.
    """
    w, h = mask.size
    m = bytearray(mask.tobytes())
    area = max(1, sum(1 for v in m if v))
    outside = bytearray(w * h)
    q = deque()
    for x in range(w):
        for y in (0, h - 1):
            i = y * w + x
            if not m[i] and not outside[i]:
                outside[i] = 1
                q.append(i)
    for y in range(h):
        for x in (0, w - 1):
            i = y * w + x
            if not m[i] and not outside[i]:
                outside[i] = 1
                q.append(i)
    while q:
        i = q.popleft()
        x, y = i % w, i // w
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h:
                j = ny * w + nx
                if not m[j] and not outside[j]:
                    outside[j] = 1
                    q.append(j)
    limit = area * hole_max
    seen = bytearray(w * h)
    for s in range(w * h):
        if m[s] or outside[s] or seen[s]:
            continue
        comp = []
        seen[s] = 1
        q = deque([s])
        while q:
            i = q.popleft()
            comp.append(i)
            x, y = i % w, i // w
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    j = ny * w + nx
                    if not m[j] and not outside[j] and not seen[j]:
                        seen[j] = 1
                        q.append(j)
        if len(comp) <= limit:
            for i in comp:
                m[i] = 255
    return Image.frombytes("L", (w, h), bytes(m))


def _drop_specks(mask, min_frac=0.004):
    """Kill islands smaller than min_frac of the largest one -- vignette
    corners and JPEG-ish crumbs that survived the flood fill."""
    w, h = mask.size
    m = bytearray(mask.tobytes())
    lab = [0] * (w * h)
    comps = []
    cur = 0
    for s in range(w * h):
        if not m[s] or lab[s]:
            continue
        cur += 1
        cnt = 0
        q = deque([s])
        lab[s] = cur
        while q:
            i = q.popleft()
            cnt += 1
            x, y = i % w, i // w
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    j = ny * w + nx
                    if m[j] and not lab[j]:
                        lab[j] = cur
                        q.append(j)
        comps.append(cnt)
    if not comps:
        return mask
    keep = max(comps) * min_frac
    for i in range(w * h):
        if lab[i] and comps[lab[i] - 1] < keep:
            m[i] = 0
    return Image.frombytes("L", (w, h), bytes(m))


# ----------------------------------------------------------------- grading ---
def _cdf(vals, bins=256):
    hist = [0] * bins
    for v in vals:
        hist[min(bins - 1, int(v * (bins - 1) + 0.5))] += 1
    tot = sum(hist) or 1
    out = []
    acc = 0
    for c in hist:
        acc += c
        out.append(acc / tot)
    return out


WZ_V_CDF = None


def set_reference_v(values):
    """Install the Webzen value distribution used for histogram matching."""
    global WZ_V_CDF
    WZ_V_CDF = _cdf(values)


def _match_v(v, src_cdf):
    """Map v through the source CDF onto the Webzen CDF (classic histogram
    matching). Falls back to a plain linear squeeze into [WZ_V_LO, WZ_V_HI]
    when no reference is installed."""
    p = src_cdf[min(255, int(v * 255 + 0.5))]
    if WZ_V_CDF is None:
        return WZ_V_LO + (WZ_V_HI - WZ_V_LO) * p
    lo, hi = 0, 255
    while lo < hi:
        mid = (lo + hi) // 2
        if WZ_V_CDF[mid] < p:
            lo = mid + 1
        else:
            hi = mid
    return lo / 255.0


def grade(im, mask, flatten=FLATTEN, levels=POSTER_LEVELS, grime=GRIME, seed=0):
    """Apply the measured Webzen grade to `im` inside `mask`.

    Order matters:
      1. flatten  -- subtract the render's broad shading (a heavy blur of V),
                     which is where the specular hotspot, the top-light ramp
                     and the vignette all live. What survives is the drawing.
      2. match    -- push V through the Webzen value CDF, so the result cannot
                     reach white (the Webzen max is V=87%) or black (min 16%).
      3. lift     -- V_LIFT, the one number tuned against the FINAL pixels
                     rather than these, because the ink outline and the
                     silhouette edges box-filter the shipped art ~13 points of
                     value darker than what leaves this function.
      4. posterize-- quantise V to `levels` steps: 2D and chapado, no ramp.
      5. saturate -- S = curve(V) * the pixel's own relative saturation, hue
                     locked near 41 deg. Neutral shadows, warm highlights.
      6. grime    -- a little multiplicative value noise so the metal reads
                     matte and slightly dirty rather than injection-moulded.
    """
    w, h = im.size
    rgb = im.convert("RGB")
    px = rgb.load()
    mk = mask.load()

    hsv = [None] * (w * h)
    vs = []
    ss = []
    for y in range(h):
        for x in range(w):
            if not mk[x, y]:
                continue
            r, g, b = px[x, y]
            hh, sv, vv = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            hsv[y * w + x] = [hh, sv, vv]
            vs.append(vv)
            ss.append(sv)
    if not vs:
        return im.convert("RGBA")

    # 1. flatten: V minus a heavy blur of V, recentred
    vimg = Image.new("L", (w, h), 0)
    vpx = vimg.load()
    for i, t in enumerate(hsv):
        if t:
            vpx[i % w, i // w] = int(t[2] * 255 + 0.5)
    # inpaint the background with the object mean so the blur is not dragged
    vmean = sum(vs) / len(vs)
    for i, t in enumerate(hsv):
        if not t:
            vpx[i % w, i // w] = int(vmean * 255 + 0.5)
    blur = vimg.filter(ImageFilter.GaussianBlur(w / 7.0)).load()

    for i, t in enumerate(hsv):
        if not t:
            continue
        x, y = i % w, i // w
        t[2] = max(0.0, min(1.0, t[2] - flatten * (blur[x, y] / 255.0 - vmean)))

    src_cdf = _cdf([t[2] for t in hsv if t])
    smean = sum(ss) / len(ss) or 1.0

    rnd = _lcg(seed)
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    op = out.load()
    q = max(2, levels) - 1
    for i, t in enumerate(hsv):
        if not t:
            continue
        x, y = i % w, i // w
        hh, sv, vv = t
        # 2. value histogram match onto the Webzen distribution
        vv = _match_v(vv, src_cdf)
        # 3. posterize inside the Webzen band
        n = (vv - WZ_V_LO) / (WZ_V_HI - WZ_V_LO)
        n = 0.0 if n < 0 else (1.0 if n > 1 else n)
        n = n ** V_LIFT
        n = round(n * q) / q
        vv = WZ_V_LO + n * (WZ_V_HI - WZ_V_LO)
        # 5. grime (applied on the quantised value so it survives posterizing)
        vv *= 1.0 + grime * (next(rnd) * 2.0 - 1.0)
        vv = max(0.0, min(WZ_V_HI, vv))
        # 4. saturation from the measured S(V) curve, modulated by how
        #    saturated this pixel was relative to its own image (so the brass
        #    parts stay hotter than the steel parts, just far less so)
        rel = 0.65 + 0.35 * (sv / smean if smean else 1.0)
        sat = min(WZ_SAT_CAP, _sat_at(vv) * min(1.8, rel) * SAT_TRIM)
        # hue locked to the Webzen 41.5 deg, keeping a sliver of the original
        dh = ((hh - WZ_HUE + 0.5) % 1.0) - 0.5
        hue = (WZ_HUE + max(-WZ_HUE_SPREAD, min(WZ_HUE_SPREAD, dh))) % 1.0
        r, g, b = colorsys.hsv_to_rgb(hue, sat, vv)
        op[x, y] = (int(r * 255 + 0.5), int(g * 255 + 0.5), int(b * 255 + 0.5),
                    mk[x, y])
    return out


def _lcg(seed):
    s = (seed * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
    while True:
        s = (s * 6364136223846793005 + 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        yield ((s >> 33) & 0xFFFFFF) / float(0xFFFFFF)


# ------------------------------------------------------------- measurement ---
def stats(pixels):
    """(mean saturation %, dominant palette) for a list of (r, g, b)."""
    if not pixels:
        return 0.0, []
    tot = 0.0
    for r, g, b in pixels:
        mx, mn = max(r, g, b), min(r, g, b)
        tot += 0.0 if mx == 0 else (mx - mn) / mx
    return tot / len(pixels) * 100.0, dominant(pixels)


def dominant(pixels, k=7):
    from collections import Counter
    step = max(1, len(pixels) // 60000)
    p = pixels[::step]
    t = Image.new("RGB", (len(p), 1))
    t.putdata(p)
    q = t.quantize(colors=k, method=Image.Quantize.MEDIANCUT,
                   dither=Image.Dither.NONE)
    pal = q.getpalette()[: k * 3]
    c = Counter(q.getdata())
    tot = sum(c.values())
    return [(tuple(pal[i * 3: i * 3 + 3]), n / tot) for i, n in c.most_common()]
