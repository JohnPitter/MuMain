"""High-zoom render of the Stadium cage region: precise object rects,
current ATT overlay, and official door markers. Panel x0..x1, y0..y1.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

REF = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_arena-ref")
sys.path.insert(0, str(REF))
sys.path.insert(0, str(Path(__file__).parent))
import muatt  # noqa: E402
from arena_round3 import load_models, load_objects, load_ground, read_att_current, CLASS_COLOR, klass  # noqa: E402

WORLD = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_wt-arena3\src\bin\Data\World7")

DOORS = [
    (16, 37, 16, 39), (16, 55, 16, 57), (16, 73, 16, 75), (16, 89, 18, 92),
    (23, 37, 24, 39), (23, 55, 24, 57), (23, 71, 24, 74), (23, 88, 24, 91),
    (41, 37, 41, 39), (41, 55, 41, 58), (41, 69, 41, 72), (41, 79, 41, 82),
    (60, 73, 64, 75),
]
BOXES = [
    (9, 35, 15, 41), (9, 53, 15, 59), (9, 71, 15, 77), (9, 88, 15, 94),
    (25, 35, 31, 41), (25, 53, 31, 59), (25, 70, 31, 76), (25, 87, 31, 93),
    (45, 35, 51, 41), (45, 54, 51, 60), (45, 69, 51, 74), (45, 78, 51, 84),
    (65, 71, 71, 77),
]


def object_rects(models, objects):
    rects = []
    for typ, x, y, yaw in objects:
        if typ < 0 or typ not in models:
            continue
        m = models[typ]
        (x0, y0), (x1, y1) = m["min"], m["max"]
        rects.append((typ, x + x0, y + y0, x + x1, y + y1))
    return rects


def main() -> None:
    x0, y0, x1, y1, scale = (int(a) for a in sys.argv[2:7])
    models = load_models()
    objects = load_objects()
    rects = object_rects(models, objects)
    walls = read_att_current()

    S = scale
    img = Image.new("RGB", ((x1 - x0) * S, (y1 - y0) * S), (0, 0, 0))
    dr = ImageDraw.Draw(img, "RGBA")

    def px(v):
        return (v - x0) * S

    def py(v):
        return (v - y0) * S

    # ground: dark checker so objects pop
    for ty in range(y0, y1):
        for tx in range(x0, x1):
            c = (52, 58, 48) if (tx + ty) % 2 == 0 else (46, 52, 43)
            dr.rectangle([px(tx), py(ty), px(tx) + S - 1, py(ty) + S - 1], fill=c)

    # hunting boxes (yellow outline) + doors (green fill)
    for bx0, by0, bx1, by1 in BOXES:
        dr.rectangle([px(bx0), py(by0), px(bx1 + 1) - 1, py(by1 + 1) - 1], outline=(255, 230, 60, 255), width=2)
    for dx0, dy0, dx1, dy1 in DOORS:
        dr.rectangle([px(dx0), py(dy0), px(dx1 + 1) - 1, py(dy1 + 1) - 1], fill=(60, 255, 60, 200), outline=(0, 90, 0, 255))

    # ATT: red hatch where blocked, green tint where walkable
    for ty in range(y0, y1):
        for tx in range(x0, x1):
            v = walls[tx + (ty << 8)]
            if v not in (0, 1):
                dr.rectangle([px(tx) + 2, py(ty) + 2, px(tx) + S - 3, py(ty) + S - 3], fill=(255, 30, 30, 90))
            else:
                dr.rectangle([px(tx), py(ty), px(tx) + 1, py(ty) + 1], fill=(40, 255, 40, 120))

    # object rects on top
    for typ, rx0, ry0, rx1, ry1 in rects:
        k = klass(typ)
        if k == "decor":
            col = (120, 160, 120, 90)
        elif k == "rails":
            col = (255, 150, 30, 220)
        elif k == "blocky":
            col = (150, 90, 40, 200)
        else:
            col = (230, 60, 230, 220)
        dr.rectangle([px(rx0 / 100), py(ry0 / 100), px(rx1 / 100) - 1, py(ry1 / 100) - 1],
                     outline=col, width=1)
        if k != "decor":
            dr.rectangle([px(rx0 / 100) + 2, py(ry0 / 100) + 2, px(rx1 / 100) - 3, py(ry1 / 100) - 3],
                         fill=(col[0], col[1], col[2], 40))
        dr.text((px(rx0 / 100), py(ry0 / 100)), str(typ), fill=(255, 255, 255, 180))

    for ty in range(y0, y1, 4):
        for tx in range(x0, x1, 8):
            dr.text((px(tx) + 2, py(ty) + 1), f"{tx},{ty}", fill=(255, 255, 255, 160))

    out = REF / f"arena_zoom_{x0}_{y0}.png"
    img.save(out)
    print("wrote", out)


if __name__ == "__main__":
    main()
