"""Render a real mesh sequence without modifying its saved authoring project."""
import bpy


def render_preview(blend, destination, frames, percentage=45):
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    scene = bpy.context.scene
    scene.cycles.samples = 8
    scene.render.resolution_percentage = percentage
    scene.frame_start, scene.frame_end = frames
    scene.render.fps = 24
    scene.render.image_settings.file_format = 'FFMPEG'
    scene.render.ffmpeg.format = 'MPEG4'
    scene.render.ffmpeg.codec = 'H264'
    scene.render.ffmpeg.constant_rate_factor = 'HIGH'
    scene.render.filepath = str(destination)
    bpy.ops.render.render(animation=True)
