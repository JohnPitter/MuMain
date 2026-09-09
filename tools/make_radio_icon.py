"""Author Interface/Radio_icon.OZT for the client-side internet radio HUD.

File format
-----------
Same writer as tools/make_hud_button_icons.py's write_ozt(): what
CGlobalBitmap::OpenTga reads (Render/Sprites/GlobalBitmap.cpp) — a 22-byte
TGA-ish header (uncompressed truecolor type 2, width @16..17, height @18..19,
depth 32 @20, descriptor 0x08 = bottom-left origin + 8-bit alpha @21), followed
by width*height*4 BGRA bytes stored bottom-up, then a 26-byte zero footer.

Sheet layout: two 256x256 frames stacked vertically (total 256x512),
frame 0 = radio ON, frame 1 = radio OFF (dimmed metal + slash), matching the
voice_mic.OZT / voice_sound.OZT active/muted convention. The HUD draws both at
15x15 px (UI/Radio/RadioHud.cpp), so the 256px art is a 17x box-filtered
downsample — silky edges in game.

Art: a MU-style table radio drawn at 8x supersample and finished in the same
steel-and-gold material as the LuxUI voice glyphs: flat 2D shapes, one warm
diffuse light from the top, matte slightly-dirty metal, mean saturation well
under 20%, ink outline as a true dilation of the alpha mask (so the outline
rings the silhouette exactly, like make_hud_button_icons.py).

Usage
-----
    py -3.12 tools/make_radio_icon.py
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

REPO = Path(__file__).resolve().parents[1]
OUT = REPO / "src" / "bin" / "Data" / "Interface" / "Radio_icon.OZT"

FRAME = 256          # per-frame art size (visual top-down square)
SS = 8               # supersample factor
OUTLINE = 5 * SS + 1  # MaxFilter needs an odd kernel; ~5px outline after downsample

# Steel-and-gold LuxUI material (measured family of the voice glyph palette:
# neutral shadow steel, warm highlight steel, brass accents, near-black ink).
INK = (24, 22, 20, 255)
STEEL_DARK = (74, 74, 82, 255)
STEEL = (126, 126, 134, 255)
STEEL_LIGHT = (168, 168, 174, 255)
BRASS = (196, 158, 82, 255)
BRASS_LIGHT = (228, 198, 122, 255)
GRILLE_DARK = (52, 52, 58, 255)

RED_DOT_ON = (196, 70, 58, 255)
RED_DOT_OFF = (120, 62, 56, 255)


def rounded(draw: ImageDraw.ImageDraw, box, radius, fill):
    draw.rounded_rectangle(box, radius=radius, fill=fill)


def compose_radio() -> Image.Image:
    """Draws the radio glyph once at supersample resolution (RGBA)."""
    size = FRAME * SS
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Antenna: a thin diagonal rod with a ball tip, top-left leaning right.
    d.line([(size * 0.62, size * 0.30), (size * 0.86, size * 0.06)],
           fill=STEEL_LIGHT, width=int(3.2 * SS))
    r = int(2.2 * SS)
    d.ellipse([size * 0.86 - r, size * 0.06 - r, size * 0.86 + r, size * 0.06 + r],
              fill=BRASS_LIGHT)

    # Body: rounded rectangle, steel face with a slightly darker bottom band.
    body = [size * 0.10, size * 0.28, size * 0.90, size * 0.78]
    radius = int(6 * SS)
    rounded(d, body, radius, STEEL)

    # Warm top light: a lighter band across the upper face (same "one warm
    # diffuse light" rule as the other LuxUI glyphs).
    d.rounded_rectangle([body[0] + int(2.5 * SS), body[1] + int(2.5 * SS),
                         body[2] - int(2.5 * SS), body[1] + int(9 * SS)],
                        radius=int(4 * SS), fill=STEEL_LIGHT)

    # Bottom shadow band keeps the plate from floating.
    d.rounded_rectangle([body[0] + int(2.5 * SS), body[3] - int(6 * SS),
                         body[2] - int(2.5 * SS), body[3] - int(2.5 * SS)],
                        radius=int(3 * SS), fill=STEEL_DARK)

    # Speaker grille: left block of horizontal dark slots.
    grille = [size * 0.15, size * 0.36, size * 0.52, size * 0.70]
    d.rounded_rectangle(grille, radius=int(3 * SS), fill=STEEL_DARK)
    slot_h = int(2.2 * SS)
    gap = int(4.4 * SS)
    y = grille[1] + int(3 * SS)
    while y + slot_h < grille[3] - int(3 * SS):
        d.rounded_rectangle([grille[0] + int(2.5 * SS), y,
                             grille[2] - int(2.5 * SS), y + slot_h],
                            radius=int(1 * SS), fill=GRILLE_DARK)
        y += slot_h + gap

    # Tuning dial: brass ring on the right with a needle, plus a small knob.
    dial_cx, dial_cy = size * 0.70, size * 0.53
    dial_r = size * 0.115
    d.ellipse([dial_cx - dial_r, dial_cy - dial_r, dial_cx + dial_r, dial_cy + dial_r],
              fill=BRASS)
    d.ellipse([dial_cx - dial_r * 0.62, dial_cy - dial_r * 0.62,
               dial_cx + dial_r * 0.62, dial_cy + dial_r * 0.62],
              fill=STEEL_DARK)
    # Needle pointing up-right (approx. 1 o'clock).
    import math
    ang = math.radians(-60)
    d.line([dial_cx, dial_cy,
            dial_cx + dial_r * 0.55 * math.cos(ang),
            dial_cy + dial_r * 0.55 * math.sin(ang)],
           fill=BRASS_LIGHT, width=int(1.8 * SS))

    # Power dot bottom-right of the face.
    dot_r = int(2.6 * SS)
    d.ellipse([size * 0.86 - dot_r, size * 0.735 - dot_r,
               size * 0.86 + dot_r, size * 0.735 + dot_r], fill=RED_DOT_ON)

    # Feet: two dark stubs under the body.
    foot_w, foot_h = int(7 * SS), int(4 * SS)
    for fx in (size * 0.20, size * 0.72):
        d.rounded_rectangle([fx, body[3] - int(1 * SS), fx + foot_w, body[3] + foot_h],
                            radius=int(1.5 * SS), fill=STEEL_DARK)

    return img


def ink_outline(alpha: Image.Image) -> Image.Image:
    """True dilation of the alpha mask, filled with ink — the silhouette
    outline technique of make_hud_button_icons.py."""
    mask = alpha.filter(ImageFilter.MaxFilter(OUTLINE))
    outline = Image.new("RGBA", alpha.size, INK)
    outline.putalpha(mask)
    return outline


def finish_frame(img: Image.Image, dim: bool) -> Image.Image:
    """Outline + optional dim-to-off grade, box-filtered down to FRAME."""
    outlined = Image.alpha_composite(ink_outline(img.split()[3]), img)
    out = outlined.resize((FRAME, FRAME), Image.BOX)

    if dim:
        # Off frame: muted metal plus a diagonal slash, the same language as
        # the muted voice glyphs.
        grey = out.convert("L").point(lambda v: int(v * 0.55))
        dimmed = Image.merge("RGBA", (grey, grey, grey, out.split()[3]))
        d = ImageDraw.Draw(dimmed)
        d.line([(int(FRAME * 0.14), int(FRAME * 0.86)),
                (int(FRAME * 0.86), int(FRAME * 0.14))],
               fill=INK, width=7)
        return dimmed
    return out


def write_ozt(path: Path, frames: list[Image.Image]) -> None:
    """frames: RGBA images (visual top-down). File rows are bottom-up."""
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
        data = im.tobytes()
        for y in range(im.height):
            rows.append(data[y * w * 4:(y + 1) * w * 4])
    rows.reverse()
    for row in rows:
        for x in range(w):
            i = x * 4
            blob += bytes((row[i + 2], row[i + 1], row[i], row[i + 3]))
    blob += b"\x00" * 26
    path.write_bytes(blob)
    print(f"{path.name:22s} {w}x{total_h}  {len(blob)} bytes")


def main() -> int:
    art = compose_radio()
    on = finish_frame(art, dim=False)
    off = finish_frame(art, dim=True)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    write_ozt(OUT, [on, off])

    preview = Image.new("RGBA", (FRAME * 2 + 12, FRAME), (40, 40, 46, 255))
    preview.paste(on, (0, 0), on)
    preview.paste(off, (FRAME + 12, 0), off)
    preview_path = OUT.with_suffix(".png")
    preview.save(preview_path)
    print(f"preview -> {preview_path.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
