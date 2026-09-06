"""Re-author Interface/newui_option_check.OZT at native 4x resolution.

Why this exists
----------------
The checkbox used by both the Options window (ESC) and the MU Helper window
(NewUIOptionWindow.cpp, NewUIMuHelper.cpp) is a 15x15-per-frame OZT rendered
at the UI's authored 640x480 reference resolution, then stretched by
g_fScreenRate_x/y to the real window (3.0x / 2.25x at 1080p,
ZzzOpenglUtil.cpp:641-674). A 15px sprite stretched ~3x with GL_LINEAR looks
soft/blurry ("painted in Paint") because there just aren't enough texels to
filter from.

The fix is the same one already applied to the HUD button icons
(make_hud_button_icons.py): ship the art at a higher native resolution so the
GPU is doing a near-1:1 (or downscaling) bilinear sample instead of a heavy
upscale. This script re-authors the sprite at 4x (60x60 per frame, 60x120
total) instead of drawing new pixel art.

What the source actually looks like (measured, not assumed)
-------------------------------------------------------------
An earlier audit assumed "box with bevel + gold check mark". Dumping the
shipped OZT (see checkbox_backup/ and the reference client copy at
C:\\Users\\joaop\\Downloads\\client_non sound_8.0\\Data\\Interface\\
newui_option_check.OZT, which is BYTE-IDENTICAL to the shipped one) shows the
real art is monochrome: a black-bordered grey/near-white plate with a solid
BLACK checkmark stroke on the checked frame -- no gold anywhere. This script
matches what is actually there, not the earlier assumption.

Frame layout (unchanged): 15x15 (now 60x60) frames stacked vertically,
frame 0 (top) = checked, frame 1 (bottom) = unchecked -- this is what
CNewUICheckBox::Render() and the 8 direct draws in NewUIOptionWindow.cpp
already select via sv=0 (checked) / sv=15 old-px (unchecked).

Method: supersampled reconstruction, not pixel-art rescale
------------------------------------------------------------
This is not a hand-redrawn vector icon and not a naive nearest/bilinear
stretch either. Each 15x15 frame already has soft, anti-aliased pixel
transitions (the border-to-fill blend, the checkmark stroke edges) that
encode the *intended* continuous shape at sub-pixel precision. The pipeline
recovers that shape at high resolution instead of inventing a new one:

  1. Lanczos-upscale each frame to a large supersampled canvas (16x, so the
     final 4x asset is a clean 1/4 box-filter downsample of it -- the same
     "work large, filter down" shape as make_hud_button_icons.py's SS/
     downsample()).
  2. Re-tighten the blur Lanczos introduces with a local unsharp pass (each
     channel is pushed away from its own Gaussian-blurred version), which
     restores crisp edges without the ringing a naive global sharpen would
     add to a mostly-flat, mostly-2-level (border/fill) image.
  3. Box-filter downsample (premultiplied, so the soft alpha edge doesn't
     fringe) to the shipped 60x60-per-frame size.

Colors and proportions therefore come directly from the official asset, not
from a re-guessed palette -- this IS that glyph, at native resolution.

File format
-----------
Same as tools/make_hud_button_icons.py's write_ozt(): what
CGlobalBitmap::OpenTga reads (Render/Sprites/GlobalBitmap.cpp) -- a 22-byte
TGA-ish header (uncompressed truecolor type 2, width @16..17, height @18..19,
depth 32 @20, descriptor 0x08 = bottom-left origin + 8-bit alpha @21),
followed by width*height*4 BGRA bytes stored bottom-up, then a 26-byte zero
footer. Frames are stacked vertically, visual top-down.

Idempotency
-----------
The 15x15 original is backed up once to checkbox_backup/ next to this script
(gitignored-style local artifact, not shipped) and every run reconstructs
from THAT backup, never from the shipped output path -- so re-running the
script after it has already written the 4x asset does not try to
"supersample" an already-4x file. Delete checkbox_backup/ only if you want to
force re-capturing the current shipped file as the new source of truth.

Usage
-----
    py -3.12 tools/make_checkbox_sprite.py

(py -3.12, not the bare `python` on PATH -- that's the hermes venv and has
neither Pillow nor numpy.)
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

TOOLS_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOLS_DIR.parent
SHIPPED_OZT = REPO_ROOT / "src" / "bin" / "Data" / "Interface" / "newui_option_check.OZT"
BACKUP_DIR = TOOLS_DIR / "checkbox_backup"
BACKUP_OZT = BACKUP_DIR / "newui_option_check.original_15x15.OZT"

SRC_FRAME = 15                  # official art, px per frame
SCALE = 4                       # shipped resolution multiplier
DST_FRAME = SRC_FRAME * SCALE   # 60
SUPERSAMPLE = 16                # working multiple of the source for reconstruction
WORK = SRC_FRAME * SUPERSAMPLE  # 240
SHARPEN = 0.6                   # unsharp strength of the edge re-tightening pass
BLUR_RADIUS = SUPERSAMPLE * 0.5

FRAME_NAMES = ["checked", "unchecked"]


# --------------------------------------------------------------------- I/O ---
def read_ozt(path: Path) -> Image.Image:
    """Inverse of write_ozt(): returns one RGBA image, frames stacked
    visual top-down (same convention write_ozt takes on the way in)."""
    data = path.read_bytes()
    w, h = struct.unpack("<HH", data[16:20])
    depth = data[20]
    if depth != 32:
        raise ValueError(f"{path}: expected 32bpp OZT, got {depth}bpp")
    pixel_bytes = w * h * 4
    pixels = data[22:22 + pixel_bytes]
    if len(pixels) != pixel_bytes:
        raise ValueError(f"{path}: truncated pixel data ({len(pixels)} of {pixel_bytes} bytes)")

    rows = [pixels[y * w * 4:(y + 1) * w * 4] for y in range(h)]
    rows.reverse()  # file is bottom-up -> rows[0] becomes the visual top row
    bgra = b"".join(rows)
    img = Image.frombuffer("RGBA", (w, h), bgra, "raw", "BGRA", 0, 1)
    return img.copy()


def write_ozt(path: Path, frames: list[Image.Image]) -> int:
    """frames: RGBA images, visual top-down. File rows are stored bottom-up."""
    w = frames[0].width
    total_h = sum(f.height for f in frames)
    blob = bytearray(22)
    blob[2] = 2                              # image type: uncompressed truecolor
    blob[16:18] = struct.pack("<H", w)
    blob[18:20] = struct.pack("<H", total_h)
    blob[20] = 32                            # bits per pixel
    blob[21] = 8                             # descriptor: bottom-left origin, 8-bit alpha

    rows: list[bytes] = []
    for im in frames:
        d = im.tobytes()
        for y in range(im.height):
            rows.append(d[y * w * 4:(y + 1) * w * 4])
    rows.reverse()
    for row in rows:
        for x in range(w):
            i = x * 4
            blob += bytes((row[i + 2], row[i + 1], row[i], row[i + 3]))  # RGBA -> BGRA
    blob += b"\x00" * 26
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(blob)
    return len(blob)


def split_frames(img: Image.Image, frame_size: int) -> list[Image.Image]:
    w, total_h = img.size
    n = total_h // frame_size
    return [img.crop((0, i * frame_size, w, (i + 1) * frame_size)) for i in range(n)]


# --------------------------------------------------------------- rebuild ---
def downsample(img_ss: Image.Image, w: int, h: int) -> Image.Image:
    """Area average, premultiplied so the soft alpha edge does not fringe."""
    arr = np.asarray(img_ss, dtype=np.float64) / 255.0
    alpha = arr[..., 3:4]
    premultiplied = np.concatenate([arr[..., :3] * alpha, alpha], axis=-1)
    step = img_ss.width // w
    reduced = premultiplied.reshape(h, step, w, step, 4).mean(axis=(1, 3))
    out_alpha = reduced[..., 3:4]
    rgb = np.where(out_alpha > 1e-6, reduced[..., :3] / np.maximum(out_alpha, 1e-6), 0.0)
    result = np.concatenate([rgb, out_alpha], axis=-1)
    return Image.fromarray((np.clip(result, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8))


def reconstruct_frame(frame_15: Image.Image) -> Image.Image:
    """Supersampled redraw of one 15x15 frame at the shipped 4x size.

    Lanczos-upscale to a large working canvas, re-tighten the resulting soft
    edges with a local unsharp pass, then box-filter down to DST_FRAME. See
    the module docstring for why this recovers the glyph's shape rather than
    inventing a new one.
    """
    work = frame_15.resize((WORK, WORK), Image.LANCZOS)
    sharp = np.asarray(work, dtype=np.float64) / 255.0
    blurred_img = work.filter(ImageFilter.GaussianBlur(BLUR_RADIUS))
    blurred = np.asarray(blurred_img, dtype=np.float64) / 255.0
    boosted = np.clip(blurred + (sharp - blurred) * (1.0 + SHARPEN), 0.0, 1.0)
    work_sharp = Image.fromarray((boosted * 255.0 + 0.5).astype(np.uint8))
    return downsample(work_sharp, DST_FRAME, DST_FRAME)


# ------------------------------------------------------------------ main ---
def ensure_backup() -> Image.Image:
    """Return the immutable 15x15 source, capturing it on first run.

    Always reconstructs from this backup, never from SHIPPED_OZT directly --
    once SHIPPED_OZT has been overwritten with the 4x art, it is no longer a
    valid source for this algorithm.
    """
    if not BACKUP_OZT.exists():
        if not SHIPPED_OZT.exists():
            raise FileNotFoundError(f"no shipped asset to back up at {SHIPPED_OZT}")
        original = read_ozt(SHIPPED_OZT)
        if original.size != (SRC_FRAME, SRC_FRAME * 2):
            raise ValueError(
                f"expected the current shipped asset to be the {SRC_FRAME}x{SRC_FRAME * 2} "
                f"original before backing it up, got {original.size}. Refusing to treat an "
                f"already-upscaled file as the source; restore the original 15x15 OZT first."
            )
        BACKUP_DIR.mkdir(parents=True, exist_ok=True)
        write_ozt(BACKUP_OZT, split_frames(original, SRC_FRAME))
        print(f"backed up original 15x{SRC_FRAME * 2} -> {BACKUP_OZT}")
    return read_ozt(BACKUP_OZT)


def build() -> None:
    original = ensure_backup()
    src_frames = split_frames(original, SRC_FRAME)
    if len(src_frames) != 2:
        raise ValueError(f"expected 2 frames in the backup, found {len(src_frames)}")

    out_frames = [reconstruct_frame(f) for f in src_frames]
    size = write_ozt(SHIPPED_OZT, out_frames)
    print(f"{SHIPPED_OZT.relative_to(REPO_ROOT)}  "
          f"{out_frames[0].width}x{sum(f.height for f in out_frames)}  {size} bytes")
    for name, frame in zip(FRAME_NAMES, out_frames):
        print(f"  frame {name}: {frame.size}")


if __name__ == "__main__":
    sys.exit(build() or 0)
