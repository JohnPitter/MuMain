"""Author the definitive Poseidon atlases: Black, Gold and Blue.

Deterministic Pillow source for the three atlas files the prototype BMDs
reference (``Poseidon_Black.jpg``, ``Poseidon_Gold.jpg``, ``Poseidon_Blue.jpg``),
closing the ``texture-dependencies.json`` contract with authored, regenerable
art instead of the provisional flat PBR materials.

Design language follows ``design-spec.md`` and the sampled concept palette
(``scratchpad/concept-manto-poseidon-dark-20260911.png``, swatch panel):

- **Preto Abissal** (armor/cloak mass): near-black field, broad ocean-swell
  undulations, faint engraved wave crests and cold machined grain. Highlights
  stay cold gray so the armor never reads gray-blue.
- **Dourado Real** (structure/fillets/edges): engraved antique gold with the
  Celestial relief grammar — wave fillets, triple-point combs, lozenge lattice,
  bead rows — and a tall P95-P05 contrast span like ``golden-metal``.
- **Azul Oceânico** (gems/runes/energy): legible ocean energy — glow cores,
  engraved lozenge lattice and angular rune strokes over a deep blue field.

The UV layout (``prototype/uv-region-report.json``) is a per-component planar
projection covering the whole 0-1 square, so each component samples the entire
atlas: masters are full-field engraved finishes, exactly like the Celestial
masters, not spatially partitioned charts. Masters are 1024 px (detail headroom
that survives the game downsample); the game export is 512 JPEG RGB q95, packed
as OZJ (24-byte zero header + JPEG) with the same wrapper the Celestial
pipeline uses. ``Branco Perolado`` exists in the concept as accessory/gleam;
the five modeled pieces own only the three declared roles, so pearl is carried
as the highlight anchor of Blue/Gold and recorded for future accessory pieces.

Usage::

    python generate_poseidon_atlases.py            # write/verify everything
    python generate_poseidon_atlases.py --check    # determinism gate only
    python generate_poseidon_atlases.py --force    # overwrite guarded masters
"""
import argparse
import hashlib
import io
import json
import math
import random
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parent
TEXTURES = ROOT / 'textures'
MASTERS = TEXTURES / 'masters'
OZJ = TEXTURES / 'ozj'
RENDERS = ROOT / 'prototype' / 'renders'

MASTER_SIZE = 1024
GAME_SIZE = 512
JPEG_PARAMETERS = dict(quality=95, subsampling=0, progressive=False, optimize=False)

# Sampled from the concept swatch panel with Pillow (2026-09-11): median tone
# of each declared swatch and its own dark body tone.
CONCEPT_PALETTE = {
    'dourado_real_principal': '#d8a066',
    'dourado_real_corpo': '#a07040',
    'preto_abissal_principal': '#111114',
    'preto_abissal_profundo': '#080808',
    'azul_oceanico_principal': '#3684dd',
    'azul_oceanico_profundo': '#104898',
    'branco_perolado_principal': '#e5e5ec',
    'branco_perolado_brilho': '#f8f8f8',
}
ANCHORS = {
    'Black': dict(shadow='#08080a', mid='#111114', highlight='#3b4150'),
    'Gold': dict(shadow='#503418', mid='#d8a066', highlight='#f7e3b4'),
    'Blue': dict(shadow='#0a2f66', mid='#3684dd', highlight='#d8efff'),
}
SEEDS = {'Black': 20260911, 'Gold': 20260912, 'Blue': 20260913}
TAU = math.tau


def hex_rgb(value):
    value = value.lstrip('#')
    return tuple(int(value[i:i + 2], 16) for i in (0, 2, 4))


# ---------------------------------------------------------------- primitives
def value_noise(size, cells, seed, octaves=4, gain=0.5):
    """Fractal value noise as an 'L' image; deterministic via seeded grid."""
    result = Image.new('L', (size, size), 0)
    amplitude, total, level = 1.0, 0.0, 0
    for octave in range(octaves):
        count = max(2, cells * 2 ** octave)
        rng = random.Random(seed * 7919 + octave)
        grid = Image.new('L', (count, count))
        grid.putdata([rng.randrange(256) for _ in range(count * count)])
        layer = grid.resize((size, size), Image.Resampling.BICUBIC)
        result = Image.blend(result, layer, amplitude / (total + amplitude)) \
            if total else layer.point(lambda v: v * amplitude)
        total += amplitude
        amplitude *= gain
        level += 1
    return result.point(lambda v: v)


def ridges(size, rows, wavelength, amplitude, phase=0.0, thickness=3, seed=0):
    """Horizontal engraved wave crest ridges (bright on the height field)."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    rng = random.Random(seed or None)
    for row in range(rows):
        base = (row + 0.5) * size / rows
        points = [(x, base + amplitude * math.sin(TAU * x / wavelength + phase + row * 1.3))
                  for x in range(0, size + 8, 8)]
        draw.line(points, fill=255, width=thickness, joint='curve')
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.6))


def combs(size, rows, teeth, height_px, thickness=4):
    """Triple-point combs: rows of repeated trident silhouettes, as ridges."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    for row in range(rows):
        base = (row + 0.5) * size / rows
        step = size / teeth
        for tooth in range(teeth):
            x = tooth * step + step / 2
            draw.line([(x - step * .22, base), (x - step * .22, base - height_px * .45)],
                      fill=255, width=thickness)
            draw.line([(x, base), (x, base - height_px)], fill=255, width=thickness)
            draw.line([(x + step * .22, base), (x + step * .22, base - height_px * .45)],
                      fill=255, width=thickness)
            draw.line([(x - step * .3, base), (x + step * .3, base)], fill=255, width=thickness)
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.7))


def lozenge_lattice(size, cell, thickness=5, phase=0.0):
    """Engraved diamond lattice returned as (grooves, outline highlights)."""
    grooves = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(grooves)
    offset = phase * cell / 2
    steps = max(4, int(cell // 2))
    for row in range(-1, size // steps + 1):
        y = row * cell / 2 + offset
        parity = (row + (1 if phase else 0)) % 2
        for column in range(-1, size // steps + 2):
            x = column * cell + (cell / 2 if parity else 0)
            draw.polygon([(x, y - cell / 2), (x + cell / 2, y),
                          (x, y + cell / 2), (x - cell / 2, y)], outline=255, width=thickness)
    grooves = grooves.filter(ImageFilter.GaussianBlur(thickness * 0.55))
    inner = grooves.filter(ImageFilter.GaussianBlur(thickness * 1.8))
    return grooves, inner


def beads(size, rows, spacing, radius=7):
    """Beaded fillet rows (bright pearls) as ridges."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    for row in range(rows):
        y = (row + 0.5) * size / rows
        step = int(spacing)
        for x in range(0, size + step, step):
            draw.ellipse([x - radius, y - radius, x + radius, y + radius], fill=255)
    return image.filter(ImageFilter.GaussianBlur(radius * 0.5))


def glow_cores(size, centers, core_radius, halo_radius, seed=1):
    """Radial energy cores: soft halo plus bright core, as an additive mask."""
    rng = random.Random(seed * 31 + centers)
    halo = Image.new('L', (size, size), 0)
    core = Image.new('L', (size, size), 0)
    halo_draw, core_draw = ImageDraw.Draw(halo), ImageDraw.Draw(core)
    for index in range(centers):
        x = rng.randrange(size)
        y = rng.randrange(size)
        halo_draw.ellipse([x - halo_radius, y - halo_radius, x + halo_radius, y + halo_radius],
                          fill=120 + rng.randrange(60))
        core_draw.ellipse([x - core_radius, y - core_radius, x + core_radius, y + core_radius],
                          fill=255)
    return (halo.filter(ImageFilter.GaussianBlur(halo_radius * 0.55)),
            core.filter(ImageFilter.GaussianBlur(core_radius * 0.8)))


def rune_rows(size, rows, glyphs, thickness=5, seed=2):
    """Asymmetric engraved rune glyphs (stem, chevrons, hooks) in loose rows."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    rng = random.Random(seed * 101 + rows * glyphs)
    cell = size / glyphs
    for row in range(rows):
        y = (row + 0.5) * size / rows + rng.uniform(-cell * .06, cell * .06)
        for glyph in range(glyphs):
            x = glyph * cell + cell / 2 + rng.uniform(-cell * .08, cell * .08)
            span = cell * rng.uniform(.2, .3)
            strokes = [
                [(x, y - span * 1.15), (x, y + span * 1.15)],                    # stem
                [(x - span, y - span * .9), (x, y - span * .9), (x + span * .5, y - span * .2)],  # upper hook
                [(x - span * .7, y + span * .3), (x, y + span * .95), (x + span * .7, y + span * .3)],  # lower chevron
                [(x - span, y + span * .2), (x + span, y - span * .6)],          # slash
                [(x - span * .6, y - span * .2), (x - span * .6, y + span)],     # side stem
                [(x - span, y - span), (x, y), (x - span * .2, y + span * .55)],  # zig
            ]
            for mark in rng.sample(range(len(strokes)), 3):
                points = strokes[mark]
                if len(points) == 2:
                    draw.line(points, fill=255, width=thickness)
                else:
                    draw.line(points, fill=255, width=thickness, joint='curve')
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.45))


def combine(*layers, size=MASTER_SIZE):
    """Screen-add 'L' layers with per-layer weights: add(a, b) saturating."""
    result = Image.new('L', (size, size), 0)
    for image, weight in layers:
        scaled = image.point(lambda v, w=weight: min(255, int(v * w)))
        result = ImageChops.add(result, scaled)
    return result


def engrave(height, strength=0.35, offset=3):
    """Directional light from the left: derivative shading over a height field."""
    left = height.transform(height.size, Image.AFFINE, (1, 0, offset, 0, 1, 0),
                            resample=Image.Resampling.BILINEAR)
    right = height.transform(height.size, Image.AFFINE, (1, 0, -offset, 0, 1, 0),
                             resample=Image.Resampling.BILINEAR)
    relief = ImageChops.subtract(left, right, 1, 128)
    return Image.blend(height, relief, strength)


# ------------------------------------------------------------------- masters
def black_master():
    """Abyssal black: broad swells, faint wave crests, cold machined grain."""
    size = MASTER_SIZE
    swells = value_noise(size, 3, SEEDS['Black'], octaves=3, gain=0.55)
    grain = value_noise(size, 96, SEEDS['Black'] + 1, octaves=2, gain=0.5)
    crests = ridges(size, 7, size / 2.6, 26, phase=0.9, thickness=3, seed=SEEDS['Black'])
    cross = ridges(size, 40, 24, 6, phase=0.0, thickness=2, seed=SEEDS['Black'] + 2)
    height = combine((swells, 1.0), (grain, 0.16), (crests, 0.30), (cross, 0.05), size=size)
    height = height.filter(ImageFilter.GaussianBlur(0.6))
    # Directional relief only; how black the field stays is decided by the
    # palette anchors (highlight is a cold sheen, never gray metal).
    return engrave(height, strength=0.30, offset=3)


def gold_master():
    """Royal gold: dense engraved ornament with a tall contrast span."""
    size = MASTER_SIZE
    base = value_noise(size, 4, SEEDS['Gold'], octaves=3, gain=0.6)
    burnish = value_noise(size, 18, SEEDS['Gold'] + 1, octaves=2, gain=0.55)
    waves = ridges(size, 6, size / 3.1, 34, phase=0.2, thickness=5, seed=SEEDS['Gold'])
    combed = combs(size, 5, 8, 46, thickness=5)
    grooves, inner = lozenge_lattice(size, size / 4.2, thickness=4)
    pearl = beads(size, 3, size / 11.5, radius=8)
    height = combine((base, 0.92), (waves, 0.42), (combed, 0.5), (inner, 0.22),
                     (pearl, 0.34), (burnish, 0.14), size=size)
    # Grooves cut dark into the metal, then light direction finishes the relief.
    height = ImageChops.subtract(height, grooves.point(lambda v: v * 0.35))
    height = height.filter(ImageFilter.GaussianBlur(0.5))
    return engrave(height, strength=0.42, offset=4)


def blue_master():
    """Ocean energy: glow cores, engraved lozenge lattice and rune strokes."""
    size = MASTER_SIZE
    deep = value_noise(size, 3, SEEDS['Blue'], octaves=3, gain=0.6)
    lattice, lattice_inner = lozenge_lattice(size, size / 3.8, thickness=3, phase=1)
    halos, cores = glow_cores(size, 7, 16, 90, seed=SEEDS['Blue'])
    runes = rune_rows(size, 4, 7, thickness=5, seed=SEEDS['Blue'])
    height = combine((deep, 0.55), (lattice_inner, 0.26), (halos, 0.62), (cores, 0.9),
                     (runes, 0.52), size=size)
    height = ImageChops.subtract(height, lattice.point(lambda v: v * 0.25))
    height = height.filter(ImageFilter.GaussianBlur(0.7))
    return engrave(height, strength=0.24, offset=3)


BUILDERS = {'Black': black_master, 'Gold': gold_master, 'Blue': blue_master}


# ------------------------------------------------------------------ pipeline
def render_master(role):
    """Deterministic master: normalize the height field, then palette colorize."""
    return colorize(normalize(BUILDERS[role]()), ANCHORS[role])


def colorize(height, anchors):
    """Map the height field onto shadow/mid/highlight palette anchors."""
    return ImageOps.colorize(height, (hex_rgb(anchors['shadow'])),
                             hex_rgb(anchors['highlight']), hex_rgb(anchors['mid']),
                             blackpoint=0, whitepoint=255, midpoint=140).convert('RGB')


def export_jpeg(master, target):
    image = master.resize((GAME_SIZE, GAME_SIZE), Image.Resampling.LANCZOS)
    image.save(target, format='JPEG', **JPEG_PARAMETERS)
    return image


def pack_ozj(jpeg_path, target):
    payload = jpeg_path.read_bytes()
    if payload[2:10] not in (b'\x00JFIF\x00\x01\x01',) and payload[2] != 0xFF:
        raise ValueError(f'Not a JPEG stream: {jpeg_path}')
    target.write_bytes(bytes(24) + payload)


def percentiles(image):
    values = sorted(image.convert('L').get_flattened_data()
                    if hasattr(image.convert('L'), 'get_flattened_data')
                    else image.convert('L').getdata())
    pick = lambda fraction: values[int((len(values) - 1) * fraction)]
    return dict(luminance_p05=pick(.05), luminance_p50=pick(.50), luminance_p95=pick(.95))


def normalize(height, lo_fraction=0.02, hi_fraction=0.98):
    """Percentile stretch so the palette anchors land exactly (white-platina style)."""
    values = sorted(height.getdata())
    pick = lambda fraction: values[int((len(values) - 1) * fraction)]
    lo, hi = pick(lo_fraction), pick(hi_fraction)
    span = max(1, hi - lo)
    return height.point(lambda v: max(0, min(255, (v - lo) * 255 // span)))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def png_bytes(image):
    buffer = io.BytesIO()
    image.save(buffer, format='PNG')
    return buffer.getvalue()


def atlas_names(role):
    return dict(master=MASTERS / f'Poseidon_{role}.png', jpeg=TEXTURES / f'Poseidon_{role}.jpg',
                ozj=OZJ / f'Poseidon_{role}.OZJ')


def portable(path):
    """Report paths relative to the repository root so they survive worktrees."""
    return path.relative_to(ROOT.parent.parent).as_posix()


def build_atlas(role, force):
    master = render_master(role)
    paths = atlas_names(role)
    paths['master'].parent.mkdir(parents=True, exist_ok=True)
    if paths['master'].exists() and not force:
        if digest(paths['master']) != hashlib.sha256(png_bytes(master)).hexdigest():
            raise ValueError(f'A different master already exists: {paths["master"]} (--force to replace)')
    else:
        paths['master'].write_bytes(png_bytes(master))
    game = export_jpeg(master, paths['jpeg'])
    pack_ozj(paths['jpeg'], paths['ozj'])
    stats = dict(master=dict(path=portable(paths['master']), size=MASTER_SIZE,
                             sha256=digest(paths['master']), **percentiles(master)),
                 game_jpeg=dict(path=portable(paths['jpeg']), size=GAME_SIZE,
                                sha256=digest(paths['jpeg']), **percentiles(game)),
                 ozj=dict(path=portable(paths['ozj']), sha256=digest(paths['ozj']),
                          bytes=paths['ozj'].stat().st_size))
    stats['game_jpeg']['contrast_span_p95_p05'] = \
        stats['game_jpeg']['luminance_p95'] - stats['game_jpeg']['luminance_p05']
    stats['master']['contrast_span_p95_p05'] = \
        stats['master']['luminance_p95'] - stats['master']['luminance_p05']
    return stats


def palette_panel(artworks):
    """Reference panel: master thumbnails plus the four concept swatches."""
    tile, pad, label_h = 300, 24, 46
    width = pad * 4 + tile * 3
    height = pad + tile + label_h + 34 * 5 + pad
    panel = Image.new('RGB', (width, height), '#0b0d12')
    draw = ImageDraw.Draw(panel)
    try:
        font = ImageFont.load_default(22)
        small = ImageFont.load_default(17)
    except TypeError:
        font = small = ImageFont.load_default()
    draw.text((pad, 14), 'POSEIDON - DEFINITIVE ATLASES (masters 1024, game 512 JPEG q95)',
              fill='#d8a066', font=font)
    for index, (role, master) in enumerate(artworks.items()):
        x = pad + index * (tile + pad)
        panel.paste(master.resize((tile, tile), Image.Resampling.LANCZOS), (x, pad + 34))
        draw.text((x + 4, pad + 34 + tile + 8), f'Poseidon_{role}.jpg  P95-P05='
                  f'{percentiles(master)["luminance_p95"] - percentiles(master)["luminance_p05"]}',
                  fill='#9aa4b4', font=small)
    rows = [('Dourado Real', 'estrutura / detalhes', '#d8a066'),
            ('Preto Abissal', 'armadura / manto', '#111114'),
            ('Azul Oceanico', 'energia / runas', '#3684dd'),
            ('Branco Perolado', 'acessorios / brilho (pecas futuras)', '#e5e5ec')]
    top = pad + 34 + tile + label_h + 18
    for index, (name, role, hexcolor) in enumerate(rows):
        y = top + index * 34
        draw.ellipse([pad, y, pad + 22, y + 22], fill=hexcolor, outline='#d8a066')
        draw.text((pad + 34, y + 2), f'{name}  {hexcolor}  ({role})', fill='#c8ccd4', font=small)
    RENDERS.mkdir(parents=True, exist_ok=True)
    panel.save(RENDERS / 'Poseidon_atlas_palette.png', format='PNG')


def build_all(force=False, check=False):
    if check:
        for role, paths in ((role, atlas_names(role)) for role in BUILDERS):
            for key in ('master', 'jpeg', 'ozj'):
                if not paths[key].exists():
                    raise ValueError(f'Missing atlas artifact: {paths[key]}')
            expected = render_master(role)
            if digest(paths['master']) != hashlib.sha256(png_bytes(expected)).hexdigest():
                raise ValueError(f'Master drifts from its deterministic source: {paths["master"]}')
            game = expected.resize((GAME_SIZE, GAME_SIZE), Image.Resampling.LANCZOS)
            buffer = io.BytesIO()
            game.save(buffer, format='JPEG', **JPEG_PARAMETERS)
            if hashlib.sha256(buffer.getvalue()).hexdigest() != digest(paths['jpeg']):
                raise ValueError(f'Game JPEG drifts from the deterministic export: {paths["jpeg"]}')
        return dict(checked=True)
    report = dict(round='poseidon-definitive-atlases',
                  uv_layout='prototype/uv-region-report.json',
                  palette=dict(sampled=CONCEPT_PALETTE, anchors=ANCHORS, seeds=SEEDS),
                  master_size=MASTER_SIZE, game_size=GAME_SIZE, jpeg_parameters=JPEG_PARAMETERS,
                  ozj_wrapper='24 zero bytes + RGB JPEG (Celestial contract, see package_candidate)',
                  atlases={})
    MASTERS.mkdir(parents=True, exist_ok=True)
    OZJ.mkdir(parents=True, exist_ok=True)
    artworks = {}
    for role in BUILDERS:
        artworks[role] = render_master(role)
    palette_panel(artworks)
    for role in BUILDERS:
        report['atlases'][role] = build_atlas(role, force)
    (TEXTURES / 'atlas-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--force', action='store_true', help='overwrite an existing master (guarded)')
    parser.add_argument('--check', action='store_true', help='verify determinism against written artifacts')
    args = parser.parse_args()
    report = build_all(args.force, args.check)
    if not args.check:
        summary = {role: dict(master_sha=stats['master']['sha256'][:16],
                              contrast_span=stats['game_jpeg']['contrast_span_p95_p05'])
                   for role, stats in report['atlases'].items()}
        print('POSEIDON_ATLASES_AUTHORED', json.dumps(summary), flush=True)
    else:
        print('POSEIDON_ATLASES_CHECK_OK', flush=True)


if __name__ == '__main__':
    main()
