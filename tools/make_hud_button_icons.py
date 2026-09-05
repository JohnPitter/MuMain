"""Build Data/Interface/LuxUI/*.OZT button faces for the Mu Helper strip and
the main toolbar.

Each OZT is a 32bpp BGRA blob with a 22-byte TGA-like header (width at 16..17,
height at 18..19, depth 32 at 20), rows bottom-up -- exactly what the client's
OpenTga loader reads when a .tga path is requested (it exchanges the extension
to .OZT on disk). Every file stacks the button state frames vertically:
frame 0 = up, 1 = hover, 2 = pressed (and 3 = attention blink on the toolbar).

Art: a neutral steel plate in MU's grey palette with a white action glyph
(1px dark outline). Same visual language as the mic/som buttons on the right
screen edge, which use the native empty-button box texture.
"""
from __future__ import annotations

import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "bin" / "Data" / "Interface" / "LuxUI"

# Steel plate palette (RGB).
BORDER = (16, 17, 20)
FACE = (58, 60, 66)
FACE_HOVER = (72, 74, 82)
FACE_DOWN = (44, 46, 52)
BEVEL_L = (88, 90, 96)
BEVEL_D = (36, 38, 44)
GLYPH = (224, 227, 233)
GLYPH_DOWN = (198, 202, 208)
OUTLINE = (12, 13, 15)
A_BORDER = (168, 132, 52)
A_FACE = (74, 64, 38)
A_GLYPH = (242, 212, 130)

STRIP_W, STRIP_FRAME_H, STRIP_FRAMES = 18, 13, 3
TOOL_W, TOOL_FRAME_H, TOOL_FRAMES = 30, 41, 4


class Canvas:
    def __init__(self, w, h, face):
        self.w = w
        self.h = h
        self.px = [[face for _ in range(w)] for _ in range(h)]

    def put(self, x, y, color):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = color

    def rect(self, x0, y0, x1, y1, color):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, color)

    def disc(self, cx, cy, r, color):
        r2 = r * r + r * 0.5
        for y in range(int(cy - r) - 1, int(cy + r) + 2):
            for x in range(int(cx - r) - 1, int(cx + r) + 2):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r2:
                    self.put(x, y, color)


def glyph_masks():
    m = {}

    # wrench: diagonal shaft + jaw block at the top-right end
    w = []
    for i in range(9):
        w.append((4 + i, 10 - i))
        w.append((5 + i, 10 - i))
    w += [(10, 2), (11, 2), (12, 2), (10, 3), (11, 3), (12, 3), (13, 3), (12, 4), (13, 4)]
    m["settings"] = w

    # play: right-pointing triangle
    p = []
    for x in range(0, 8):
        half = x * 4 // 7
        for y in range(4 - half, 5 + half + 1):
            p.append((x + 1, y))
    m["play"] = p

    # stop: pause bars
    s = []
    for y in range(0, 8):
        s += [(1, y), (2, y), (4, y), (5, y)]
    m["stop"] = s

    # auto battle: crossed swords with hilt feet
    a = []
    for i in range(9):
        a.append((1 + i, 1 + i))
        a.append((2 + i, 1 + i))
        a.append((7 - i, 1 + i))
        a.append((6 - i, 1 + i))
    a += [(0, 1), (1, 0), (8, 1), (7, 0)]
    a += [(0, 8), (1, 8), (8, 8), (7, 8)]
    m["auto"] = a

    # market: coin disc (local 9x9)
    c = []
    for y in range(-5, 6):
        for x in range(-5, 6):
            if x * x + y * y <= 19:
                c.append((x + 4, y + 4))
    m["market"] = c
    return m


MASKS = glyph_masks()


def frame_canvas(w, h, face, border):
    cv = Canvas(w, h, face)
    cv.rect(0, 0, w - 1, 0, border)
    cv.rect(0, h - 1, w - 1, h - 1, border)
    cv.rect(0, 0, 0, h - 1, border)
    cv.rect(w - 1, 0, w - 1, h - 1, border)
    cv.put(1, 1, BEVEL_L)
    cv.rect(1, h - 2, w - 2, h - 2, BEVEL_D)
    cv.rect(w - 2, 1, w - 2, h - 2, BEVEL_D)
    return cv


def draw_strip_frame(cv, name, frame):
    face = FACE_DOWN if frame == 2 else (FACE_HOVER if frame == 1 else FACE)
    glyph = GLYPH_DOWN if frame == 2 else GLYPH
    oy = 2
    mask = [(x, y) for (x, y) in MASKS[name] if 0 <= x < STRIP_W and 0 <= y < STRIP_FRAME_H]
    pts = set(mask)
    for (x, y) in mask:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                if (x + dx, y + dy) not in pts:
                    cv.put(x + dx + 3, y + dy + oy, OUTLINE)
    for (x, y) in mask:
        cv.put(x + 3, y + oy, glyph)


def draw_toolbar_frame(cv, name, frame, face, glyph):
    cx, cy = 15, 21
    if name == "cashshop":
        cv.disc(cx, cy, 8.5, glyph)
        cv.disc(cx, cy, 5.6, face)
        cv.disc(cx, cy, 4.2, glyph)
        cv.rect(cx - 1, cy - 4, cx, cy + 4, face)
    elif name == "character":
        cv.disc(cx, cy - 6, 4.4, glyph)
        cv.rect(cx - 2, cy - 2, cx + 1, cy, glyph)
        for i, w in enumerate((10, 14, 17, 18, 18, 17, 14)):
            cv.rect(cx - w // 2, cy + 1 + i, cx + w // 2, cy + 1 + i, glyph)
    elif name == "inventory":
        cv.rect(cx - 3, cy - 8, cx + 2, cy - 4, glyph)
        cv.rect(cx - 2, cy - 7, cx + 1, cy - 5, face)
        for i, w in enumerate((12, 14, 15, 15, 15, 15, 14, 12)):
            cv.rect(cx - w // 2, cy - 3 + i, cx + w // 2, cy - 3 + i, glyph)
        cv.rect(cx - 1, cy, cx, cy + 1, OUTLINE)
    elif name == "friends":
        cv.disc(cx - 4, cy - 5, 4.2, glyph)
        for i, w in enumerate((9, 12, 13, 13, 12)):
            cv.rect(cx - 9, cy - 1 + i, cx - 9 + w, cy - 1 + i, glyph)
        cv.disc(cx + 5, cy - 3, 3.4, glyph)
        for i, w in enumerate((7, 9, 10, 10)):
            cv.rect(cx + 2, cy + 2 + i, cx + 2 + w, cy + 2 + i, glyph)
    elif name == "menu":
        cv.disc(cx, cy, 9.5, glyph)
        cv.disc(cx, cy, 6.4, face)
        cv.rect(cx - 5, cy - 10, cx + 4, cy - 3, face)
        cv.rect(cx - 1, cy - 9, cx, cy, glyph)
    else:
        raise ValueError(name)


def make_strip(name):
    frames = []
    for frame in range(STRIP_FRAMES):
        face = FACE_DOWN if frame == 2 else (FACE_HOVER if frame == 1 else FACE)
        cv = frame_canvas(STRIP_W, STRIP_FRAME_H, face, BORDER)
        draw_strip_frame(cv, name, frame)
        frames.append(cv)
    return frames


def make_toolbar(name):
    frames = []
    for frame in range(TOOL_FRAMES):
        alert = frame == 3
        face = A_FACE if alert else (FACE_DOWN if frame == 2 else (FACE_HOVER if frame == 1 else FACE))
        glyph = A_GLYPH if alert else GLYPH
        cv = frame_canvas(TOOL_W, TOOL_FRAME_H, face, A_BORDER if alert else BORDER)
        draw_toolbar_frame(cv, name, frame, face, glyph)
        frames.append(cv)
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
        for (r, g, b) in row:
            blob += bytes((b, g, r, 255))
    blob += b"\x00" * 26
    path.write_bytes(blob)
    print(path.name, w, "x", total_h, len(blob), "bytes")


def write_preview(strip_frames, tool_frames):
    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    scale = 4
    gap = 2 * scale
    strip_w = 3 * (STRIP_W + 1) * scale + gap
    tool_w = 5 * (TOOL_W + 1) * scale + gap
    sheet_w = gap + max(strip_w, tool_w) + gap
    strip_col_h = STRIP_FRAMES * (STRIP_FRAME_H + 1) * scale + gap
    tool_col_h = TOOL_FRAMES * (TOOL_FRAME_H + 1) * scale + gap
    sheet_h = gap + strip_col_h + gap + tool_col_h + gap
    sheet = Canvas(sheet_w, sheet_h, (24, 26, 30))

    def blit(cv, ox, oy):
        for y in range(cv.h):
            for x in range(cv.w):
                r, g, b = cv.px[y][x]
                for sy in range(scale):
                    for sx in range(scale):
                        sheet.put(ox + x * scale + sx, oy + y * scale + sy, (r, g, b))

    ox = gap
    for key, frames in strip_frames.items():
        for fi, cv in enumerate(frames):
            blit(cv, ox + fi * (STRIP_W + 1) * scale, gap)
        ox += (STRIP_W + 1) * scale * 3 + gap
    ox = gap
    for key, frames in tool_frames.items():
        for fi, cv in enumerate(frames):
            blit(cv, ox + fi * (TOOL_W + 1) * scale, gap + strip_col_h + gap)
        ox += (TOOL_W + 1) * scale + gap

    rows = []
    for y in range(sheet.h):
        row = bytearray()
        for x in range(sheet.w):
            r, g, b = sheet.px[y][x]
            row += bytes((r, g, b, 255))
        rows.append(bytes(row))
    raw = b"".join(b"\x00" + r for r in rows)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", sheet.w, sheet.h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw))
    png += chunk(b"IEND", b"")
    (OUT / "_preview.png").write_bytes(png)
    print("preview:", (OUT / "_preview.png"))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    strip = {
        "helper_settings": "settings",
        "helper_play": "play",
        "helper_stop": "stop",
        "helper_auto": "auto",
        "helper_market": "market",
    }
    toolbar = {
        "toolbar_cashshop": "cashshop",
        "toolbar_character": "character",
        "toolbar_inventory": "inventory",
        "toolbar_friends": "friends",
        "toolbar_menu": "menu",
    }
    strip_frames = {k: make_strip(g) for k, g in strip.items()}
    tool_frames = {k: make_toolbar(g) for k, g in toolbar.items()}
    for key, frames in strip_frames.items():
        write_ozt(OUT / (key + ".OZT"), frames)
    for key, frames in tool_frames.items():
        write_ozt(OUT / (key + ".OZT"), frames)
    write_preview(strip_frames, tool_frames)


if __name__ == "__main__":
    main()
