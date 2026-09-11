"""Studio framing helpers shared by the mount and eagle prototypes."""
import math

import bpy

from poseidon_armor_preview import point_at


def frame_target(scene, center, azimuth_deg, distance=420, scale=210, elevation=0):
    """Orbit the orthographic inspection camera around a world-space target."""
    angle = math.radians(azimuth_deg)
    location = (center[0] + distance * math.sin(angle),
                center[1] - distance * math.cos(angle),
                center[2] + distance * math.sin(math.radians(elevation)) * distance)
    scene.camera.location = location
    point_at(scene.camera, center)
    scene.camera.data.ortho_scale = scale
    scene.camera.data.clip_end = 5000
