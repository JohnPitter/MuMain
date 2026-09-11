"""Author the definitive Zeus atlases: Blue, Platina and Emissive.

Deterministic Pillow source for the three atlas files every prototype BMD of
the set references (``Zeus_Blue.jpg``, ``Zeus_Platina.jpg`` and
``Zeus_Emissive.jpg``), closing the ``texture-dependencies.json`` contract with
authored, regenerable art instead of the provisional flat PBR materials.

Design language follows ``design-spec.md`` and the concept board
(``scratchpad/concept-herdeiro-de-zeus-mg-20260911.png``, "HERDEIRO DE ZEUS,
versao magica azul"):

- **Azul Celeste** (armor mass, dominant): moderately dark celestial-blue
  metal with the relief engraved into it - angular lightning zigzags, "V"
  chevron facets and bolt-tip wedges - plus cold machined grain and painted
  pale highlights. The Legendary +15 Excellent study
  (``research/legendary-excellent-blue.md``) measured that a set "reads blue"
  through the upgrade passes, so the mass stays a moderately dark celeste
  (never live neon) with bright highlights the passes will saturate.
- **Branco Platinado** (trims/details): the owner-approved platinum-white
  finish. Tone anchors are exactly the sampled ``white-platina`` values
  (shadow ``#545156``, mid ``#d7d3d4``, highlight ``#faf8f6``) that already
  passed visual approval; the relief stays soft polished metal (satin swells,
  bead fillets, silk sheen, faint wide chevrons) and the height field uses the
  approved P05-P95 normalization window that put the platina masters at
  contrast span 166/154.
- **Emissive** (storm channels/gems/runes): legible celestial energy - deep
  blue field, bright engraved zigzag storm channels with glow halos, energy
  cores and angular rune strokes. The full field also covers the cloth-grid
  role documented in ``cape-cloth-contract.md``.

Family motifs are strictly Zeus: sharp polyline bolts, chevron "V" facets,
multi-point stars, wedge teeth. No Poseidon wave/trident curves and no
Celestial halos. The UV layout (``prototype/uv-region-report.json``) is a
per-component planar projection covering the whole 0-1 square, so each
component samples the entire atlas: masters are full-field engraved finishes,
not spatially partitioned charts. Masters are 1024 px (detail headroom that
survives the game downsample); the game export is 512 JPEG RGB q95, packed as
OZJ (24-byte zero header + JPEG) with the same wrapper the Celestial pipeline
uses.

Usage::

    python generate_zeus_atlases.py            # write/verify everything
    python generate_zeus_atlases.py --check    # determinism gate only
    python generate_zeus_atlases.py --force    # overwrite guarded masters
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

# Sampled from the concept board with Pillow (2026-09-11): the declared swatch
# panel tones plus median body tones of the armor/wing/cape masses and the
# storm-energy glow. The white platina anchors are the owner-approved ones
# (art-source/celestial/textures/masters/white-platina), not re-invented.
CONCEPT_PALETTE = {
    'azul_celeste_principal': '#328efa',
    'azul_celeste_massa': '#3a62a0',
    'azul_celeste_profundo': '#16325e',
    'energia_emissiva': '#2579d8',
    'branco_platinado_principal': '#d7d3d4',
    'branco_platinado_brilho': '#faf8f6',
}
ANCHORS = {
    'Blue': dict(shadow='#0a274e', mid='#2f66b8', highlight='#d8e9ff'),
    # Owner rule (2026-09-11): the detail white is the approved platinum-white
    # finish, anchors unchanged from the approved white-platina round.
    'Platina': dict(shadow='#545156', mid='#d7d3d4', highlight='#faf8f6'),
    'Emissive': dict(shadow='#04214e', mid='#1f74d8', highlight='#c2e6ff'),
}
SEEDS = {'Blue': 20260921, 'Platina': 20260922, 'Emissive': 20260923}
TAU = math.tau


def hex_rgb(value):
    value = value.lstrip('#')
    return tuple(int(value[i:i + 2], 16) for i in (0, 2, 4))


# ---------------------------------------------------------------- primitives
def value_noise(size, cells, seed, octaves=4, gain=0.5):
    """Fractal value noise as an 'L' image; deterministic via seeded grid."""
    result = Image.new('L', (size, size), 0)
    amplitude, total = 1.0, 0.0
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
    return result


def bolt(size, start, end, drop, thickness, seed):
    """One sharp lightning polyline: horizontal run with sudden vertical drops.

    Zeus language is angular - segments meet at hard angles, never a sine
    wave. Returns the list of points for drawing.
    """
    rng = random.Random(seed)
    x0, y0 = start
    x1, y1 = end
    points = [(x0, y0)]
    steps = max(3, int(abs(x1 - x0) / max(drop, 1)))
    step_x = (x1 - x0) / steps
    step_y = (y1 - y0) / steps
    y = y0
    for index in range(1, steps + 1):
        y += step_y
        jitter = rng.uniform(-drop * .3, drop * .3) if 0 < index < steps else 0
        points.append((x0 + step_x * index, y + jitter))
    return points


def bolt_rows(size, rows, run, drop, thickness=6, seed=0, drift=0.0):
    """Rows of engraved lightning zigzags as a height ridge field."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    rng = random.Random(seed * 131 + rows)
    for row in range(rows):
        y = (row + 0.5) * size / rows
        start_x = rng.uniform(-size * .2, size * .2)
        points = bolt(size, (start_x, y), (start_x + run, y + size * drift * (rng.random() - .5)),
                      drop, thickness, seed * 977 + row)
        draw.line(points, fill=255, width=thickness, joint='curve')
        # Branch spike: a short sharp offshoot, as on a real bolt.
        fork = points[len(points) // 2]
        draw.line([fork, (fork[0] + drop * rng.choice((1, -1)), fork[1] + size * .045)],
                  fill=210, width=max(2, thickness // 2), joint='curve')
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.55))


def chevrons(size, bands, thickness=7, phase=0.0):
    """Wide "V" chevron facets - the Zeus plate-facet motif - as ridges."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    offset = phase * size / bands / 2
    for band in range(-1, bands + 1):
        y = band * size / bands + offset
        half = size / bands * 0.9
        draw.line([(0, y - half * .28), (size / 2, y + half * .22), (size, y - half * .28)],
                  fill=255, width=thickness, joint='curve')
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.6))


def wedge_teeth(size, rows, teeth, height_px, thickness=4):
    """Rows of bolt-tip wedges (sharp triangles), the angular plate comb."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    for row in range(rows):
        base = (row + 0.5) * size / rows
        step = size / teeth
        for tooth in range(teeth):
            x = tooth * step + step / 2
            draw.polygon([(x - step * .3, base), (x + step * .3, base),
                          (x, base - height_px)], fill=255)
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.7))


def star_sparks(size, centers, points, outer, inner, thickness=5, seed=3):
    """Multi-point star outlines - the Zeus axial ornament - as ridge strokes."""
    image = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(image)
    rng = random.Random(seed * 313 + centers)
    for index in range(centers):
        cx, cy = rng.uniform(size * .1, size * .9), rng.uniform(size * .1, size * .9)
        phase = rng.uniform(0, TAU)
        outline = []
        for corner in range(points * 2):
            radius = outer if corner % 2 == 0 else inner
            angle = phase + math.pi * corner / points
            outline.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
        draw.line(outline + [outline[0]], fill=255, width=thickness, joint='curve')
    return image.filter(ImageFilter.GaussianBlur(thickness * 0.5))


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
                          fill=110 + rng.randrange(60))
        core_draw.ellipse([x - core_radius, y - core_radius, x + core_radius, y + core_radius],
                          fill=255)
    return (halo.filter(ImageFilter.GaussianBlur(halo_radius * 0.55)),
            core.filter(ImageFilter.GaussianBlur(core_radius * 0.8)))


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


def storm_runes(size, rows, glyphs, thickness=5, seed=2):
    """Angular storm runes: stems, chevrons and zigs only - no curves."""
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
                [(x, y - span * 1.15), (x, y + span * 1.15)],                     # stem
                [(x - span, y - span), (x, y - span * 1.1), (x, y)],              # drop hook
                [(x - span * .8, y + span * .2), (x, y + span * .9),
                 (x + span * .8, y + span * .2)],                                 # chevron
                [(x - span, y + span * .6), (x, y - span * .2), (x + span, y + span * .6)],  # zig
                [(x - span * .7, y - span * .3), (x + span * .7, y - span * .3)],  # bar
                [(x + span * .6, y - span), (x + span * .6, y + span * .4)],      # side stem
            ]
            for mark in rng.sample(range(len(strokes)), 3):
                points = strokes[mark]
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
def blue_master():
    """Celestial-blue armor mass: engraved bolt zigzags, chevrons and wedges."""
    size = MASTER_SIZE
    swells = value_noise(size, 3, SEEDS['Blue'], octaves=3, gain=0.6)
    grain = value_noise(size, 96, SEEDS['Blue'] + 1, octaves=2, gain=0.5)
    bolts = bolt_rows(size, 6, size * 1.1, size / 14, thickness=6,
                      seed=SEEDS['Blue'], drift=0.5)
    facets = chevrons(size, 4, thickness=7, phase=0.3)
    wedges = wedge_teeth(size, 5, 7, size / 22, thickness=4)
    height = combine((swells, 1.0), (bolts, 0.5), (facets, 0.34), (wedges, 0.24),
                     (grain, 0.15), size=size)
    return height.filter(ImageFilter.GaussianBlur(0.55))


def platina_master():
    """Approved platinum-white finish: bright polished metal, neutral tint.

    The approved white-platina masters read as a BRIGHT field (p50 ~204)
    whose darks are the engraved relief itself, never large blotches - so the
    mass swells stay shallow and the relief is carried by bead fillets, fine
    satin grain and wide faint chevrons. Anchors and the P05-P95 normalization
    window are exactly the approved ones (master P95-P05 166, game 154), and
    the same field survives being stretched across the cape cloth grid UV
    0..1 without visible tiling seams.
    """
    size = MASTER_SIZE
    swells = value_noise(size, 6, SEEDS['Platina'], octaves=3, gain=0.5)
    sheen = value_noise(size, 64, SEEDS['Platina'] + 1, octaves=2, gain=0.5)
    facets = chevrons(size, 3, thickness=6, phase=0.5)
    pearls = beads(size, 5, size / 10.5, radius=8)
    height = combine((swells, 0.3), (pearls, 0.9), (facets, 0.55), (sheen, 0.3), size=size)
    height = height.filter(ImageFilter.GaussianBlur(0.5))
    height = engrave(height, strength=0.45, offset=3)
    # Smoothstep S-curve: value noise concentrates midtones, and the approved
    # platinum-white span (P95-P05 154-166) needs real mass at both tails. The
    # curve keeps the percentile normalization (anchors still land exactly).
    height = height.point(lambda v: int(255 * (v / 255) ** 2 * (3 - 2 * v / 255)))
    return height.filter(ImageFilter.GaussianBlur(0.4))


def emissive_master():
    """Storm energy: bright zigzag channels, glow cores and angular runes."""
    size = MASTER_SIZE
    deep = value_noise(size, 3, SEEDS['Emissive'], octaves=3, gain=0.6)
    channels = bolt_rows(size, 7, size * 1.15, size / 11, thickness=7,
                         seed=SEEDS['Emissive'], drift=0.8)
    halos, cores = glow_cores(size, 8, 15, 84, seed=SEEDS['Emissive'])
    runes = storm_runes(size, 4, 7, thickness=5, seed=SEEDS['Emissive'])
    sparks = star_sparks(size, 5, 8, size / 16, size / 26, thickness=4,
                         seed=SEEDS['Emissive'])
    height = combine((deep, 0.5), (channels, 0.66), (halos, 0.58), (cores, 0.9),
                     (runes, 0.5), (sparks, 0.34), size=size)
    # The channel network cuts bright OUT of the deep field: energy sits in
    # grooves here (opposite of the Blue relief), then light finishes it.
    height = ImageChops.subtract(height, deep.point(lambda v: v * 0.18))
    height = height.filter(ImageFilter.GaussianBlur(0.7))
    return engrave(height, strength=0.22, offset=3)


BUILDERS = {'Blue': blue_master, 'Platina': platina_master, 'Emissive': emissive_master}


# ------------------------------------------------------------------ pipeline
# The approved white-platina finish stretches the height field between its own
# P05 and P95 so the palette anchors land exactly on the 5th/95th percentile
# (that is how the platina masters reached span 166/154). Blue and Emissive
# keep the original P02/P98 window; Platina uses the approved window.
NORMALIZE_FRACTIONS = {'Platina': (0.05, 0.95)}


def render_master(role):
    """Deterministic master: normalize the height field, then palette colorize."""
    lo, hi = NORMALIZE_FRACTIONS.get(role, (0.02, 0.98))
    return colorize(normalize(BUILDERS[role](), lo, hi), ANCHORS[role])


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


def sorted_values(image):
    gray = image.convert('L')
    return sorted(gray.get_flattened_data() if hasattr(gray, 'get_flattened_data')
                  else gray.getdata())


def percentiles(image):
    values = sorted_values(image)
    pick = lambda fraction: values[int((len(values) - 1) * fraction)]
    return dict(luminance_p05=pick(.05), luminance_p50=pick(.50), luminance_p95=pick(.95))


def normalize(height, lo_fraction=0.02, hi_fraction=0.98):
    """Percentile stretch so the palette anchors land exactly (white-platina style)."""
    values = sorted_values(height)
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
    return dict(master=MASTERS / f'Zeus_{role}.png', jpeg=TEXTURES / f'Zeus_{role}.jpg',
                ozj=OZJ / f'Zeus_{role}.OZJ')


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
    """Reference panel: master thumbnails plus the concept swatches."""
    tile, pad, label_h = 300, 24, 46
    count = len(artworks)
    width = pad * (count + 1) + tile * count
    height = pad + tile + label_h + 34 * 4 + pad
    panel = Image.new('RGB', (width, height), '#0b0d12')
    draw = ImageDraw.Draw(panel)
    try:
        font = ImageFont.load_default(22)
        small = ImageFont.load_default(17)
    except TypeError:
        font = small = ImageFont.load_default()
    draw.text((pad, 14), 'ZEUS - DEFINITIVE ATLASES (masters 1024, game 512 JPEG q95)',
              fill='#328efa', font=font)
    for index, (role, master) in enumerate(artworks.items()):
        x = pad + index * (tile + pad)
        panel.paste(master.resize((tile, tile), Image.Resampling.LANCZOS), (x, pad + 34))
        draw.text((x + 4, pad + 34 + tile + 8), f'Zeus_{role}.jpg  P95-P05='
                  f'{percentiles(master)["luminance_p95"] - percentiles(master)["luminance_p05"]}',
                  fill='#9aa4b4', font=small)
    rows = [('Azul Celeste', 'massa principal (relevo de raios gravado)', '#2f66b8'),
            ('Branco Platinado', 'trims/detalhes (branco platina aprovado)', '#d7d3d4'),
            ('Emissive', 'canaletas de tempestade / gemas / runas', '#1f74d8')]
    top = pad + 34 + tile + label_h + 18
    for index, (name, role, hexcolor) in enumerate(rows):
        y = top + index * 34
        draw.ellipse([pad, y, pad + 22, y + 22], fill=hexcolor, outline='#328efa')
        draw.text((pad + 34, y + 2), f'{name}  {hexcolor}  ({role})', fill='#c8ccd4', font=small)
    RENDERS.mkdir(parents=True, exist_ok=True)
    panel.save(RENDERS / 'Zeus_atlas_palette.png', format='PNG')


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
    report = dict(round='zeus-definitive-atlases',
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
        print('ZEUS_ATLASES_AUTHORED', json.dumps(summary), flush=True)
    else:
        print('ZEUS_ATLASES_CHECK_OK', flush=True)


if __name__ == '__main__':
    main()
