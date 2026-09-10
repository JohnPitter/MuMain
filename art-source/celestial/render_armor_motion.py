"""Render a short real mesh-animation review from the verified Blender source."""
import json
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent / 'authored-armor'


def main():
    report = json.loads((ROOT / 'armor-report.json').read_text())
    clip = next(clip for clip in report['clips'] if clip['name'] == 'PLAYER_WALK_WAND')
    bpy.ops.wm.open_mainfile(filepath=str(ROOT / 'celestial-authored-armor.blend'))
    scene = bpy.context.scene
    scene.cycles.samples = 8
    scene.render.resolution_percentage = 45
    scene.frame_start, scene.frame_end = clip['start'], clip['end'] - 1
    scene.render.fps = 24
    scene.render.image_settings.file_format = 'FFMPEG'
    scene.render.ffmpeg.format = 'MPEG4'
    scene.render.ffmpeg.codec = 'H264'
    scene.render.ffmpeg.constant_rate_factor = 'HIGH'
    scene.render.filepath = str(ROOT / 'Celestial_Armor_walk_preview.mp4')
    bpy.ops.render.render(animation=True)


if __name__ == '__main__':
    main()
