"""Mirror the radio HUD button's native plate horizontally.

Rodada 2 (owner request): the radio button plate on the LEFT screen edge kept
the voice-dock's right-facing bevel ("abertura" para a direita), which reads
wrong on that side — the owner wants it to read as coming OUT of the left
screen edge. This tool derives `Interface/Radio_btn_plate.OZT` from the native
`Interface/newui_btn_empty_very_small.OZT` by flipping it horizontally, same
format: 22-byte TGA-ish header (uncompressed truecolor, 32bpp, descriptor 0x08
bottom-left origin), BGRA bottom-up payload, 26-byte zero footer. The 3-frame
vertical state sheet (up/over/down) survives intact: frames are full-width, so
a whole-texture mirror flips each state consistently.

The client loads it as `Interface\\Radio_btn_plate.tga` (CGlobalBitmap::OpenTga
exchanges the extension to .OZT) under BITMAP_LUXUI_RADIO_PLATE — see
UI/Radio/RadioHud.cpp.

Usage:
    py -3.12 tools/mirror_radio_plate.py
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


def read_ozt(path: Path) -> tuple[int, int, Image.Image]:
    blob = path.read_bytes()
    if len(blob) < HEADER_BYTES:
        raise SystemExit(f"{path.name}: truncated header")
    width, height = struct.unpack("<HH", blob[16:20])
    depth = blob[20]
    if depth != 32:
        raise SystemExit(f"{path.name}: expected 32bpp, got {depth}")
    payload = blob[HEADER_BYTES:HEADER_BYTES + width * height * 4]
    if len(payload) < width * height * 4:
        raise SystemExit(f"{path.name}: truncated payload")
    # BGRA bottom-up -> RGBA top-down
    img = Image.frombytes("RGBA", (width, height), payload, "raw", "BGRA")
    return img.transpose(Image.Transpose.FLIP_TOP_BOTTOM), width, height


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
    img, w, h = read_ozt(SRC)
    mirrored = img.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    write_ozt(OUT, mirrored)

    preview = Image.new("RGBA", (w * 2 + 12, h), (40, 40, 46, 255))
    preview.paste(img, (0, 0), img)
    preview.paste(mirrored, (w + 12, 0), mirrored)
    preview_path = OUT.with_suffix(".png")
    preview.save(preview_path)
    print(f"preview -> {preview_path.relative_to(REPO)} (left = native, right = mirrored)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
