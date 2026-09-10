"""Rotate the radio HUD button's native plate 180 degrees.

Rodada 2, correção do dono: the LEFT-edge radio button plate kept the
voice-dock's depth/lighting ("abertura") reading wrong. A horizontal
mirror was tried first (kept the vertical shading, still read inverted);
what the owner actually wants is the plate **rotated 180 degrees** —
depth/lighting must say the button comes OUT of the left screen edge.
The radio GLYPH (Radio_icon.OZT) is a separate texture and stays
upright, untouched.

This tool derives `Interface/Radio_btn_plate.OZT` from the native
`Interface/newui_btn_empty_very_small.OZT` by rotating EACH of the three
54x23 state frames 180 degrees, preserving the frame ORDER (the button
widget addresses states by frame index: 0 = up, 1 = over, 2 = down).
Same file format: 22-byte TGA-ish header (uncompressed truecolor, 32bpp,
descriptor 0x08 bottom-left origin), BGRA bottom-up payload, 26-byte
zero footer.

The client loads it as `Interface\\Radio_btn_plate.tga`
(CGlobalBitmap::OpenTga exchanges the extension to .OZT) under
BITMAP_LUXUI_RADIO_PLATE — see UI/Radio/RadioHud.cpp.

Usage:
    py -3.12 tools/rotate_radio_plate.py
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src" / "bin" / "Data" / "Interface" / "newui_btn_empty_very_small.OZT"
OUT = REPO / "src" / "bin" / "Data" / "Interface" / "Radio_btn_plate.OZT"

HEADER_BYTES = 22
FOOTER_BYTES = 26
FRAME_COUNT = 3  # CNewUIButton with overflg=true: 0 = up, 1 = over, 2 = down


def read_ozt(path: Path) -> Image.Image:
    """Decodes the sheet to a visual TOP-DOWN RGBA image."""
    blob = path.read_bytes()
    if len(blob) < HEADER_BYTES:
        raise SystemExit(f"{path.name}: truncated header")
    width, height = struct.unpack("<HH", blob[16:20])
    if blob[20] != 32:
        raise SystemExit(f"{path.name}: expected 32bpp, got {blob[20]}")
    payload = blob[HEADER_BYTES:HEADER_BYTES + width * height * 4]
    if len(payload) < width * height * 4:
        raise SystemExit(f"{path.name}: truncated payload")
    # BGRA bottom-up -> RGBA top-down
    img = Image.frombytes("RGBA", (width, height), payload, "raw", "BGRA")
    return img.transpose(Image.Transpose.FLIP_TOP_BOTTOM)


def write_ozt(path: Path, img: Image.Image) -> None:
    w, h = img.size
    top_down = img.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    blob = bytearray(HEADER_BYTES)
    blob[2] = 2
    blob[16:18] = struct.pack("<H", w)
    blob[18:20] = struct.pack("<H", h)
    blob[20] = 32
    blob[21] = 8
    blob += top_down.tobytes("raw", "BGRA")
    blob += b"\x00" * FOOTER_BYTES
    path.write_bytes(blob)
    print(f"{path.name:22s} {w}x{h}  {len(blob)} bytes")


def main() -> int:
    sheet = read_ozt(SRC)
    w, h = sheet.size
    if h % FRAME_COUNT != 0:
        raise SystemExit(f"{SRC.name}: height {h} is not {FRAME_COUNT} equal frames")
    frame_h = h // FRAME_COUNT

    # Rotate each state frame 180° IN PLACE, keeping the frame order so the
    # widget's up/over/down indices still address the same states.
    out = Image.new("RGBA", (w, h))
    for i in range(FRAME_COUNT):
        frame = sheet.crop((0, i * frame_h, w, (i + 1) * frame_h))
        out.paste(frame.transpose(Image.Transpose.ROTATE_180), (0, i * frame_h))
    write_ozt(OUT, out)

    preview = Image.new("RGBA", (w * 2 + 12, h), (40, 40, 46, 255))
    preview.paste(sheet, (0, 0), sheet)
    preview.paste(out, (w + 12, 0), out)
    preview = preview.resize((preview.width * 4, preview.height * 4), Image.NEAREST)
    preview_path = OUT.with_suffix(".png")
    preview.save(preview_path)
    print(f"preview -> {preview_path.relative_to(REPO)} (left = native, right = rotated 180)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
