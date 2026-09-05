"""Build Data/Interface/LuxUI/*.OZT button faces for the Mu Helper strip and
the main toolbar.

Each OZT is a 32bpp BGRA blob with a 22-byte TGA-like header (width at 16..17,
height at 18..19, depth 32 at 20), rows bottom-up -- exactly what the client's
OpenTga loader reads when a .tga path is requested (it exchanges the extension
to .OZT on disk). Every file stacks the button state frames vertically:
frame 0 = up, 1 = hover, 2 = pressed (and 3 = attention blink on the toolbar).

Art (v3): the owner wants the FIRST procedural generation back -- the
steel-and-gold glyphs of 3bf058aa (UI/NewUI/HUD/HudIcons.cpp, briefly live
before 48e000de replaced them with plainer drawings). The quad tables below
are ported verbatim from that commit and baked onto the button plate, so the
buttons render the 3bf058aa look through the native CNewUIButton texture flow
(the same pipeline as the mic/som buttons). Glyphs are authored on a 24-unit
grid and drawn 1:1 at the exact scales 3bf058aa used at runtime
(kStripGlyphScale 0.42, kToolbarGlyphScale 1.0) -- nothing is stretched.
"""
from __future__ import annotations

import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "bin" / "Data" / "Interface" / "LuxUI"

# -- steel-and-gold palette, verbatim from 3bf058aa HudIcons.cpp -------------
INK = (0.07, 0.08, 0.10)
STEEL_D = (0.36, 0.38, 0.42)
STEEL_M = (0.55, 0.57, 0.61)
STEEL_L = (0.76, 0.78, 0.82)
STEEL_H = (0.93, 0.95, 0.97)
GOLD = (0.91, 0.71, 0.31)
GOLD_L = (0.96, 0.86, 0.58)

FACE_NORMAL = (0.15, 0.16, 0.19)
FACE_HOVER = (0.27, 0.29, 0.34)
FACE_PRESSED = (0.10, 0.11, 0.13)
FACE_ALERT = (0.34, 0.27, 0.13)

OUTLINE_W = 1.0  # icon units
STRIP_W, STRIP_FRAME_H, STRIP_FRAMES = 18, 13, 3
TOOL_W, TOOL_FRAME_H, TOOL_FRAMES = 30, 41, 4
STRIP_GLYPH_SCALE = 0.42  # kStripGlyphScale
TOOL_GLYPH_SCALE = 1.0  # kToolbarGlyphScale

# Quads: (x, y, w, h, color) in icon units, origin at the glyph center,
# +y down -- the tables from 3bf058aa's HudIcons.cpp, unchanged.

SETTINGS = [
    (-3, -11, 6, 4.5, STEEL_D), (-3, 6.5, 6, 4.5, STEEL_D),
    (-11, -3, 4.5, 6, STEEL_D), (6.5, -3, 4.5, 6, STEEL_D),
    (-9.5, -9.5, 5, 5, STEEL_D), (4.5, -9.5, 5, 5, STEEL_D),
    (-9.5, 4.5, 5, 5, STEEL_D), (4.5, 4.5, 5, 5, STEEL_D),
    (-8, -6, 16, 12, STEEL_M), (-6, -8, 12, 16, STEEL_M),
    (-6, -7.5, 12, 3, STEEL_L), (-7.5, -6, 3, 12, STEEL_L),
    (4.5, -6, 3, 12, STEEL_D), (-6, 4.5, 12, 3, STEEL_D),
    (-4, -4, 8, 8, INK), (-3, -3, 6, 6, GOLD),
    (-3, -3, 6, 2, GOLD_L),
]

PLAY = [
    (-7, -9, 1.8, 2, STEEL_H), (-7, -7, 5.3, 2, STEEL_H),
    (-7, -5, 8.9, 2, STEEL_H), (-7, -3, 12.4, 2, STEEL_L),
    (-7, -1, 16, 2, STEEL_L), (-7, 1, 12.4, 2, STEEL_L),
    (-7, 3, 8.9, 2, STEEL_M), (-7, 5, 5.3, 2, STEEL_M),
    (-7, 7, 1.8, 2, STEEL_M),
    (-7, -7.6, 1.6, 15.2, GOLD),
]

STOP = [
    (-6, -9, 12, 1, STEEL_L), (-7, -8, 14, 1, STEEL_L),
    (-8, -7, 16, 14, STEEL_M),
    (-7, 7, 14, 1, STEEL_D), (-6, 8, 12, 1, STEEL_D),
    (-8, -7, 16, 2.5, STEEL_L), (-8, -7, 2.5, 14, STEEL_L),
    (5.5, -7, 2.5, 14, STEEL_D), (-8, 4.5, 16, 2.5, STEEL_D),
    (-4, -4, 8, 8, INK),
    (-3, -3, 6, 6, GOLD), (-3, -3, 6, 2, GOLD_L),
]

AUTO_BATTLE = [
    (5.4, 5.4, 3.2, 3.2, STEEL_D), (3.4, 3.4, 3.2, 3.2, STEEL_D),
    (1.4, 1.4, 3.2, 3.2, STEEL_D), (-0.6, -0.6, 3.2, 3.2, STEEL_D),
    (-2.6, -2.6, 3.2, 3.2, STEEL_D), (-4.6, -4.6, 3.2, 3.2, STEEL_D),
    (-6.6, -6.6, 3.2, 3.2, STEEL_D), (-8.6, -8.6, 3.2, 3.2, STEEL_M),
    (-8.6, 5.4, 3.2, 3.2, STEEL_M), (-6.6, 3.4, 3.2, 3.2, STEEL_L),
    (-4.6, 1.4, 3.2, 3.2, STEEL_L), (-2.6, -0.6, 3.2, 3.2, STEEL_L),
    (-0.6, -2.6, 3.2, 3.2, STEEL_L), (1.4, -4.6, 3.2, 3.2, STEEL_L),
    (3.4, -6.6, 3.2, 3.2, STEEL_L), (5.4, -8.6, 3.2, 3.2, STEEL_H),
    (-11, 6.2, 5.5, 3.4, GOLD), (-11, 6.2, 5.5, 1.2, GOLD_L),
    (5.5, 6.2, 5.5, 3.4, GOLD), (5.5, 6.2, 5.5, 1.2, GOLD_L),
]

MARKETPLACE = [
    (-2, -8, 4, 14, STEEL_M), (-2, -8, 1.5, 14, STEEL_L),
    (-5.5, 6, 11, 3, STEEL_D), (-5.5, 6, 11, 1.2, STEEL_M),
    (-11, -8, 22, 3, STEEL_L), (-11, -8, 22, 1.2, STEEL_H),
    (-3, -11.5, 6, 3.5, GOLD), (-3, -11.5, 6, 1.4, GOLD_L),
    (-8.4, -5, 1.8, 2.5, STEEL_D), (6.6, -5, 1.8, 2.5, STEEL_D),
    (-11, -2.5, 7, 2.5, STEEL_M), (-10, 0, 5, 2.5, STEEL_D),
    (4, -2.5, 7, 2.5, STEEL_M), (5, 0, 5, 2.5, STEEL_D),
]

CASH_SHOP = [
    (-5.5, -11.5, 2.5, 5.5, STEEL_L), (3, -11.5, 2.5, 5.5, STEEL_L),
    (-5.5, -11.5, 11, 2.5, STEEL_L),
    (-9.5, -6.5, 19, 17.5, STEEL_M),
    (-9.5, -6.5, 19, 3, STEEL_D),
    (-9.5, -3.5, 3, 14.5, STEEL_L),
    (6.5, -3.5, 3, 14.5, STEEL_D),
    (-9.5, 9, 19, 2, STEEL_D),
    (-4, -0.5, 8, 8, INK),
    (-3, 0.5, 6, 6, GOLD), (-3, 0.5, 6, 2, GOLD_L),
    (-1, 2, 2, 3, INK),
]

CHARACTER = [
    (-5, -12, 10, 2, STEEL_L),
    (-7, -10, 14, 2, STEEL_L),
    (-8.5, -8, 17, 3, STEEL_M),
    (-9, -5, 18, 12, STEEL_M),
    (-9, -5, 3, 12, STEEL_L),
    (6, -5, 3, 12, STEEL_D),
    (-7, 7, 14, 2, STEEL_D),
    (-5, 9, 10, 2, STEEL_D),
    (-7, -2.5, 14, 3, INK),
    (-5, 3, 2.5, 3, INK), (-1.25, 3, 2.5, 3, INK),
    (2.5, 3, 2.5, 3, INK),
    (-1.5, -13.5, 3, 9, GOLD), (-1.5, -13.5, 1.2, 9, GOLD_L),
]

INVENTORY = [
    (-2.5, -12.5, 5, 3.5, STEEL_D),
    (-11, -5, 2.5, 9, STEEL_D), (8.5, -5, 2.5, 9, STEEL_D),
    (-9.5, -6, 19, 17, STEEL_M),
    (-9.5, -6, 3, 17, STEEL_L), (6.5, -6, 3, 17, STEEL_D),
    (-9.5, 9, 19, 2, STEEL_D),
    (-6.5, -10.5, 13, 1.5, STEEL_L), (-8.5, -9, 17, 1.5, STEEL_L),
    (-9.5, -7.5, 19, 5.5, STEEL_L),
    (-6.5, -10.5, 13, 1.5, STEEL_H), (-8.5, -9, 17, 1, STEEL_H),
    (-9.5, -2, 19, 1.5, INK),
    (-6, 2.5, 12, 7, STEEL_D), (-6, 2.5, 12, 1.5, STEEL_M),
    (-2.5, -4, 5, 5, GOLD), (-2.5, -4, 5, 1.5, GOLD_L),
]

FRIENDS = [
    (2.75, -8.5, 3.5, 1, STEEL_D), (1.5, -7.5, 6, 1.5, STEEL_D),
    (0.5, -6, 8, 3.5, STEEL_D), (1.5, -2.5, 6, 1.5, STEEL_D),
    (0.5, 0, 8, 2, STEEL_D), (-0.5, 2, 10, 2, STEEL_D),
    (-1, 4, 11, 6, STEEL_D),
    (-7.5, -11.5, 6, 3, INK), (-9, -10.5, 9, 3.5, INK),
    (-10, -9, 11, 6, INK), (-9, -5, 9, 3.5, INK),
    (-7.5, -3.5, 6, 3, INK), (-9.5, -1.5, 10, 4, INK),
    (-11.5, 0.5, 14, 4, INK), (-12.5, 2.5, 16, 8.5, INK),
    (-6.5, -10.5, 4, 1, STEEL_L), (-8, -9.5, 7, 1.5, STEEL_L),
    (-9, -8, 9, 3.5, STEEL_L), (-8, -4, 7, 1.5, STEEL_M),
    (-6.5, -2.5, 4, 1, STEEL_M), (-8.5, -0.5, 8, 2, STEEL_M),
    (-10.5, 1.5, 12, 2, STEEL_M), (-11.5, 3.5, 14, 6.5, STEEL_M),
    (-11.5, 3.5, 3, 6.5, STEEL_L),
    (-8.5, 1.5, 8, 2, GOLD),
]

MENU = [
    (-11, -11, 22, 22, GOLD),
    (-11, -11, 22, 2, GOLD_L),
    (-9.5, -9.5, 19, 19, INK),
    (-7, -7, 14, 3.5, STEEL_L), (-7, -7, 14, 1.2, STEEL_H),
    (-7, -1.75, 14, 3.5, STEEL_L), (-7, -1.75, 14, 1.2, STEEL_H),
    (-7, 3.5, 14, 3.5, STEEL_L), (-7, 3.5, 14, 1.2, STEEL_H),
]


def rgb255(color):
    return tuple(round(v * 255) for v in color)


class Canvas:
    """RGBA pixels, top-down rows."""

    def __init__(self, w, h):
        self.w = w
        self.h = h
        self.px = [[(0, 0, 0, 255) for _ in range(w)] for _ in range(h)]

    def fill_rect(self, x, y, w, h, rgba):
        """Float rect, pixel-center coverage -- the GL quad rasterization rule."""
        for py in range(max(0, int(y + 0.5) - 1), min(self.h, int(y + h + 0.5) + 1)):
            cy = py + 0.5
            if not (y <= cy < y + h):
                continue
            row = self.px[py]
            for px in range(max(0, int(x + 0.5) - 1), min(self.w, int(x + w + 0.5) + 1)):
                cx = px + 0.5
                if x <= cx < x + w:
                    row[px] = rgba

    def fill_rects(self, rects, rgba):
        for (x, y, w, h) in rects:
            self.fill_rect(x, y, w, h, rgba)


def draw_plate(cv, face, bevel):
    """3bf058aa DrawButtonPlate: 1px ink rim, 1px bevel, face, lit/shaded edges."""
    w, h = cv.w, cv.h
    inset = 2
    cv.fill_rects([(0, 0, w, h)], rgb255(INK) + (255,))
    cv.fill_rects([(1, 1, w - 2, h - 2)], rgb255(bevel) + (255,))
    fw, fh = w - inset * 2, h - inset * 2
    cv.fill_rects([(inset, inset, fw, fh)], rgb255(face) + (255,))
    cv.fill_rects([(inset, inset, fw, 1)], rgb255(STEEL_D) + (255,))
    cv.fill_rects([(inset, inset + fh - 1, fw, 1)], rgb255(INK) + (255,))


def draw_glyph(cv, table, cx, cy, scale):
    """Sticker outline (every quad re-drawn ink at 8 offsets) + colored fills."""
    o = OUTLINE_W * scale
    offsets = [(-o, 0), (o, 0), (0, -o), (0, o), (-o, -o), (o, -o), (-o, o), (o, o)]
    outline = [
        (cx + (x * scale) + dx, cy + (y * scale) + dy, w * scale, h * scale)
        for (x, y, w, h, _c) in table
        for (dx, dy) in offsets
    ]
    cv.fill_rects(outline, rgb255(INK) + (255,))
    for (x, y, w, h, c) in table:
        cv.fill_rect(cx + x * scale, cy + y * scale, w * scale, h * scale, rgb255(c) + (255,))


def make_frame(w, h, face, bevel, table, cx, cy, glyph_scale):
    cv = Canvas(w, h)
    draw_plate(cv, face, bevel)
    draw_glyph(cv, table, cx, cy, glyph_scale)
    return cv


def make_strip(table):
    frames = []
    for face in (FACE_NORMAL, FACE_HOVER, FACE_PRESSED):
        frames.append(make_frame(STRIP_W, STRIP_FRAME_H, face, STEEL_D, table,
                                 STRIP_W / 2, STRIP_FRAME_H / 2, STRIP_GLYPH_SCALE))
    return frames


def make_toolbar(table):
    frames = []
    states = ((FACE_NORMAL, STEEL_D), (FACE_HOVER, STEEL_D),
              (FACE_PRESSED, STEEL_D), (FACE_ALERT, GOLD_L))
    for face, bevel in states:
        frames.append(make_frame(TOOL_W, TOOL_FRAME_H, face, bevel, table,
                                 TOOL_W / 2, TOOL_FRAME_H / 2, TOOL_GLYPH_SCALE))
    return frames


def write_ozt(path, frames):
    """frames: list of Canvas (visual top-down). File rows are bottom-up."""
    w = frames[0].w
    frame_h = frames[0].h
    total_h = frame_h * len(frames)
    all_rows = []
    for cv in frames:
        all_rows.extend(cv.px)
    all_rows.reverse()

    blob = bytearray(22)
    blob[2] = 2
    blob[16:18] = struct.pack("<H", w)
    blob[18:20] = struct.pack("<H", total_h)
    blob[20] = 32
    blob[21] = 8
    for row in all_rows:
        for (r, g, b, a) in row:
            blob += bytes((b, g, r, a))
    blob += b"\x00" * 26
    path.write_bytes(blob)
    print(path.name, w, "x", total_h, len(blob), "bytes")


def write_preview(strip_frames, tool_frames):
    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    scale = 4
    gap = 2 * scale
    strip_col = 3 * (STRIP_W + 1) * scale  # one key: its frames side by side
    tool_col = (TOOL_W + 1) * scale
    n_strip, n_tool = len(strip_frames), len(tool_frames)
    strip_w = n_strip * (strip_col + gap)
    tool_w = n_tool * (tool_col + gap)
    sheet_w = gap + max(strip_w, tool_w) + gap
    strip_col_h = STRIP_FRAMES * (STRIP_FRAME_H + 1) * scale
    tool_col_h = TOOL_FRAMES * (TOOL_FRAME_H + 1) * scale
    sheet_h = gap + strip_col_h + gap + tool_col_h + gap
    bg = (24, 26, 30)
    sheet = [[bg for _ in range(sheet_w)] for _ in range(sheet_h)]

    def put(x, y, color):
        if 0 <= x < sheet_w and 0 <= y < sheet_h:
            sheet[y][x] = color

    def blit(cv, ox, oy):
        for y in range(cv.h):
            for x in range(cv.w):
                r, g, b, a = cv.px[y][x]
                if a == 0:
                    continue
                for sy in range(scale):
                    for sx in range(scale):
                        put(ox + x * scale + sx, oy + y * scale + sy, (r, g, b))

    ox = gap
    for frames in strip_frames.values():
        for fi, cv in enumerate(frames):
            blit(cv, ox + fi * (STRIP_W + 1) * scale, gap)
        ox += strip_col + gap
    ox = gap
    for frames in tool_frames.values():
        for fi, cv in enumerate(frames):
            blit(cv, ox + fi * (TOOL_W + 1) * scale, gap + strip_col_h + gap)
        ox += tool_col + gap

    rows = []
    for y in range(sheet_h):
        row = bytearray()
        for x in range(sheet_w):
            r, g, b = sheet[y][x]
            row += bytes((r, g, b, 255))
        rows.append(bytes(row))
    raw = b"".join(b"\x00" + r for r in rows)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", sheet_w, sheet_h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw))
    png += chunk(b"IEND", b"")
    (OUT / "_preview.png").write_bytes(png)
    print("preview:", (OUT / "_preview.png"))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    strip = {
        "helper_settings": SETTINGS,
        "helper_play": PLAY,
        "helper_stop": STOP,
        "helper_auto": AUTO_BATTLE,
        "helper_market": MARKETPLACE,
    }
    toolbar = {
        "toolbar_cashshop": CASH_SHOP,
        "toolbar_character": CHARACTER,
        "toolbar_inventory": INVENTORY,
        "toolbar_friends": FRIENDS,
        "toolbar_menu": MENU,
    }
    strip_frames = {k: make_strip(t) for k, t in strip.items()}
    tool_frames = {k: make_toolbar(t) for k, t in toolbar.items()}
    for key, frames in strip_frames.items():
        write_ozt(OUT / (key + ".OZT"), frames)
    for key, frames in tool_frames.items():
        write_ozt(OUT / (key + ".OZT"), frames)
    write_preview(strip_frames, tool_frames)


if __name__ == "__main__":
    main()
