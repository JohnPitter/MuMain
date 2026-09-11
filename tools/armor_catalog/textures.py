"""Read native texture wrappers and measure unmodified atlas pixels."""
import colorsys
import hashlib
import re
from io import BytesIO
from pathlib import PurePosixPath

from PIL import Image


PALETTE_SIZE = 6
SAMPLE_EDGE = 128
THUMBNAIL_EDGE = 160


def open_texture(relative, raw):
    suffix = PurePosixPath(relative).suffix.lower()
    if suffix == '.ozj':
        for marker in re.finditer(b'\xff\xd8\xff', raw[:4096]):
            try:
                image = Image.open(BytesIO(raw[marker.start():]))
                image.load()
                return image.convert('RGBA')
            except (OSError, ValueError):
                continue
        raise ValueError('OZJ without a decodable JPEG stream')
    else:
        image = Image.open(BytesIO(raw[4:] if suffix == '.ozt' else raw))
    image.load()
    return image.convert('RGBA')


def color_name(rgb):
    hue, saturation, value = colorsys.rgb_to_hsv(*(c / 255 for c in rgb))
    if value < .18:
        return 'preto/escuro'
    if saturation < .14:
        return 'branco/prata' if value > .7 else 'cinza'
    if value < .45 and hue < .17:
        return 'marrom/bronze escuro'
    if hue < .04 or hue >= .96:
        return 'vermelho'
    if hue < .10:
        return 'laranja/bronze'
    if hue < .19:
        return 'amarelo/ouro'
    if hue < .45:
        return 'verde'
    if hue < .55:
        return 'ciano'
    if hue < .73:
        return 'azul'
    return 'violeta/rosa'


def palette(image):
    sample = image.copy()
    sample.thumbnail((SAMPLE_EDGE, SAMPLE_EDGE), Image.Resampling.LANCZOS)
    samples = sample.get_flattened_data() if hasattr(sample, 'get_flattened_data') else sample.getdata()
    pixels = [rgb[:3] for rgb in samples if rgb[3] >= 128]
    if not pixels:
        return []
    strip = Image.new('RGB', (len(pixels), 1))
    strip.putdata(pixels)
    reduced = strip.quantize(colors=PALETTE_SIZE, method=Image.Quantize.MEDIANCUT)
    colors = reduced.getpalette()
    return [dict(hex='#' + ''.join(f'{channel:02X}' for channel in colors[index * 3:index * 3 + 3]),
                 share=round(count / len(pixels), 4), label=color_name(colors[index * 3:index * 3 + 3]))
            for count, index in sorted(reduced.getcolors(), reverse=True)]


def texture_record(relative, raw, output):
    digest = hashlib.sha256(raw).hexdigest()
    image = open_texture(relative, raw)
    record = dict(sha256=digest, bytes=len(raw), width=image.width, height=image.height,
                  alpha_min=image.getchannel('A').getextrema()[0], palette=palette(image))
    image.thumbnail((THUMBNAIL_EDGE, THUMBNAIL_EDGE), Image.Resampling.LANCZOS)
    thumbnail = output / 'texture-thumbnails' / f'{digest[:16]}.png'
    thumbnail.parent.mkdir(parents=True, exist_ok=True)
    image.save(thumbnail)
    record['thumbnail'] = thumbnail.relative_to(output).as_posix()
    return record


def resolve_texture(source, model_path, texture_name):
    texture = PurePosixPath(texture_name.replace('\\', '/'))
    directory = PurePosixPath(model_path).parent
    expected_suffix = '.ozt' if texture.suffix.lower() in ('.tga', '.ozt') else '.ozj'
    candidates = [directory / texture.with_suffix(expected_suffix), directory / texture]
    for relative in candidates:
        if str(relative).lower() in source.entries:
            return str(relative).lower(), 'same_model_directory'
    return None, 'unresolved_in_same_source'


def elsewhere_candidates(source, texture_name):
    if source.texture_names is None:
        source.texture_names = {}
        for relative in source.entries:
            if PurePosixPath(relative).suffix in ('.ozj', '.ozt', '.jpg', '.jpeg', '.png', '.tga'):
                source.texture_names.setdefault(PurePosixPath(relative).stem, []).append(relative)
    stem = PurePosixPath(texture_name.lower()).stem
    return sorted(source.texture_names.get(stem, []))


def runtime_semantics(name):
    return dict(hidden_sentinel=name.startswith('hid'), skin=name.startswith('ski') or name.lower().startswith('level'),
                hair=name.startswith('hair'), source='src/source/Data/DataHandler/LoadData.cpp:72-96')
