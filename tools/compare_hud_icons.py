"""Build the approval sheet: Webzen original vs raw Canva vs graded Canva.

Everything is rendered at the SAME final button size and the SAME 3x zoom, on
the same dark ground, so the only difference between the two Canva rows is the
grade. Below the rows, the dominant palette and the mean saturation of each of
the three sets, which is the number the grade is aimed at.

    py -3.12 tools/compare_hud_icons.py [out.png]
"""
from __future__ import annotations

import io
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
import make_hud_button_icons as gen  # noqa: E402
import webzen_grade as wg  # noqa: E402

ZOOM = 3
BG = (0x14, 0x16, 0x1A)
FG = (0xD8, 0xD4, 0xCC)
DIM = (0x8A, 0x88, 0x82)


def _font(sz, bold=False):
    for n in (("arialbd.ttf", "seguisb.ttf") if bold else ("arial.ttf", "segoeui.ttf")):
        try:
            return ImageFont.truetype(n, sz)
        except OSError:
            pass
    return ImageFont.load_default()


F_H = _font(19, True)
F_L = _font(12)
F_S = _font(11)


def webzen_buttons():
    def ozj(p):
        im = Image.open(io.BytesIO(p.read_bytes()[24:]))
        im.load()
        return im.convert("RGB")

    def ozt(p):
        im = Image.open(io.BytesIO(p.read_bytes()[4:]))
        im.load()
        return im.convert("RGBA")

    out = []
    for n in ("01", "02", "03", "04"):
        col = ozj(gen.IFACE / f"newui_menu_Bt{n}.OZJ").crop((0, 0, 38, 42))
        msk = ozt(gen.IFACE / f"newui_menu_Bt{n}.OZT").crop((0, 0, 38, 42))
        out.append((f"Bt{n}", Image.merge("RGBA", (*col.split(), msk.split()[0]))))
    b5 = ozj(gen.IFACE / "partCharge1" / "newui_menu_Bt05.OZJ").crop((0, 0, 30, 41))
    out.append(("Bt05", b5.convert("RGBA")))
    return out


def webzen_glyph_px():
    """The reference set the whole grade is aimed at: Bt01..04 through their
    real OZT alpha plus Bt05 with its near-black card keyed out. Same rule as
    tools/../scratchpad/wz/measure_temp.py, so the numbers printed here are
    directly comparable to the ones in the brief."""
    px = []
    for name, im in webzen_buttons():
        if name == "Bt05":
            px += [p[:3] for p in im.getdata()
                   if max(p[:3]) >= 40 and min(p[:3]) <= 235]
        else:
            px += [p[:3] for p in im.getdata() if p[3] >= 128]
    return px


def raw_build():
    """The same pipeline with the grade switched off -- isolate only. This is
    what the Canva art looks like once it is a button, ungraded."""
    real = wg.grade
    wg.grade = lambda im, mask, **kw: Image.merge(
        "RGBA", (*im.convert("RGB").split(), mask))
    gen._cache.clear()
    try:
        return gen.build()
    finally:
        wg.grade = real
        gen._cache.clear()


def opaque(im, amin=200):
    return [p[:3] for p in im.convert("RGBA").getdata() if p[3] >= amin]


def zoom(im):
    return im.resize((im.width * ZOOM, im.height * ZOOM), Image.NEAREST)


def main(out_path):
    graded, graded_g = gen.build()
    raw, raw_g = raw_build()
    wz = webzen_buttons()

    order = ["helper_settings", "helper_play", "helper_stop", "helper_auto",
             "helper_market", "toolbar_cashshop", "toolbar_character",
             "toolbar_inventory", "toolbar_friends", "toolbar_menu",
             "voice_mic", "voice_sound"]
    label = {"helper_settings": "config", "helper_play": "play",
             "helper_stop": "stop", "helper_auto": "auto",
             "helper_market": "mercado", "toolbar_cashshop": "cash shop",
             "toolbar_character": "personagem", "toolbar_inventory": "inventario",
             "toolbar_friends": "amigos", "toolbar_menu": "menu",
             "voice_mic": "microfone", "voice_sound": "som"}

    # every cell is the same box: the widest/tallest of anything we show
    cw = max([im.width for _, im in wz]
             + [graded[k][0].width for k in order]) * ZOOM
    ch = max([im.height for _, im in wz]
             + [graded[k][0].height for k in order]) * ZOOM
    pad, gap, lab_h, head_h = 22, 14, 18, 34

    ncol = max(len(wz), len(order))
    W = pad * 2 + ncol * (cw + gap) - gap
    rows_y = []
    y = pad
    for _ in range(3):
        rows_y.append(y)
        y += head_h + ch + lab_h + gap * 2
    pal_y = y + 16
    H = pal_y + 3 * 46 + 40

    sheet = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(sheet)

    def row(title, subtitle, items, ry):
        d.text((pad, ry), title, font=F_H, fill=FG)
        tw = d.textlength(title, font=F_H)
        d.text((pad + tw + 12, ry + 5), subtitle, font=F_S, fill=DIM)
        top = ry + head_h
        for i, (name, im) in enumerate(items):
            x = pad + i * (cw + gap)
            big = zoom(im)
            cell = Image.new("RGB", (cw, ch), (0x0E, 0x10, 0x13))
            cell.paste(big, ((cw - big.width) // 2, (ch - big.height) // 2),
                       big if big.mode == "RGBA" else None)
            sheet.paste(cell, (x, top))
            d.rectangle([x, top, x + cw - 1, top + ch - 1], outline=(0x2B, 0x2E, 0x34))
            w = d.textlength(name, font=F_L)
            d.text((x + (cw - w) / 2, top + ch + 4), name, font=F_L, fill=DIM)

    # measured on the GLYPH only, no plate, so the three sets are comparable:
    # the Webzen OZT alpha masks the glyph, not the frame around it.
    wz_px = webzen_glyph_px()
    raw_only = gen.glyph_only(raw_g)
    gr_only = gen.glyph_only(graded_g)
    raw_px, gr_px = [], []
    for k in order:
        raw_px += opaque(raw_only[k], 200)
        gr_px += opaque(gr_only[k], 200)

    row("WEBZEN ORIGINAL", "newui_menu_Bt01..05 compostos com o alfa real",
        wz, rows_y[0])
    row("CANVA CRU", "arte do Canva recortada, sem correcao de cor",
        [(label[k], raw[k][0]) for k in order], rows_y[1])
    row("CANVA GRADUADO (novo)", "mesma arte apos o grade medido da Webzen",
        [(label[k], graded[k][0]) for k in order], rows_y[2])

    d.text((pad, pal_y - 30), "PALETA DOMINANTE E SATURACAO MEDIA", font=F_H, fill=FG)
    tw = d.textlength("PALETA DOMINANTE E SATURACAO MEDIA", font=F_H)
    d.text((pad + tw + 14, pal_y - 25),
           "medido so no glifo (sem a placa), como o alfa real da Webzen",
           font=F_S, fill=DIM)
    sw, sh = 96, 30
    for r, (title, px) in enumerate((("WEBZEN", wz_px), ("CANVA CRU", raw_px),
                                     ("CANVA GRADUADO", gr_px))):
        yy = pal_y + r * 46
        sat, dom = wg.stats(px)
        d.text((pad, yy + 8), f"{title}", font=F_L, fill=FG)
        d.text((pad, yy + 22), f"sat media {sat:.2f}%", font=F_S, fill=DIM)
        x = pad + 130
        for c, f in dom[:7]:
            d.rectangle([x, yy, x + sw - 4, yy + sh], fill=tuple(c))
            hexs = "#%02X%02X%02X" % tuple(c)
            d.text((x + 3, yy + sh + 2), f"{hexs} {f*100:.0f}%", font=F_S, fill=DIM)
            x += sw
    sheet.save(out_path)
    print("->", out_path)
    for title, px in (("WEBZEN", wz_px), ("CANVA CRU", raw_px),
                      ("CANVA GRADUADO", gr_px)):
        sat, dom = wg.stats(px)
        vs = sorted(max(q) / 255 * 100 for q in px)
        n = len(vs)
        print(f"{title:16s} sat={sat:6.2f}%  n={n:6d}  "
              f"V p1={vs[n//100]:.0f} p25={vs[n//4]:.0f} p50={vs[n//2]:.0f} "
              f"p75={vs[3*n//4]:.0f} p95={vs[int(.95*n)]:.0f} p100={vs[-1]:.0f}")
        print(f"{'':16s} " + " · ".join(
            "#%02X%02X%02X %.0f%%" % (*c, f * 100) for c, f in dom))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "icons-webzen-grade.png")
