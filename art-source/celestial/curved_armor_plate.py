"""Closed, rounded metal shells with an actual curved crown instead of a pyramid."""
import math

from mathutils import Vector

from celestial_geometry import mesh


def rounded_outline(outline, corner_fraction=.14):
    points = [Vector(point) for point in outline]
    result = []
    for index, corner in enumerate(points):
        previous, following = points[index - 1], points[(index + 1) % len(points)]
        start = corner.lerp(previous, corner_fraction)
        end = corner.lerp(following, corner_fraction)
        for t in (0, .25, .5, .75, 1):
            result.append((1 - t) ** 2 * start + 2 * (1 - t) * t * corner + t * t * end)
    return result


def curved_plate(name, outline, shape, material):
    depth, rise = shape
    points = [Vector(point) for point in outline]
    center = sum(points, Vector((0, 0))) / len(points)
    count, rings = len(points), 6
    vertices, faces = [], []
    for row in range(rings):
        radius = 1 - row / rings
        crown = rise * math.sin((1 - radius) * math.pi / 2)
        vertices.extend((point.x, depth - crown, point.y)
                        for point in (center.lerp(point, radius) for point in points))
    for row in range(rings - 1):
        for index in range(count):
            a, b = row * count + index, row * count + (index + 1) % count
            faces.append((a, b, b + count, a + count))
    top = len(vertices)
    vertices.append((center.x, depth - rise, center.y))
    faces.extend(((rings - 1) * count + index, (rings - 1) * count + (index + 1) % count, top)
                 for index in range(count))
    front_faces = len(faces)
    close_shell(vertices, faces, points, depth)
    obj = mesh(name, vertices, faces, material)
    for face in obj.data.polygons[:front_faces]:
        face.use_smooth = True
    obj['celestial_surface'] = 'curved_front_plate'
    return obj


def close_shell(vertices, faces, outline, depth):
    count, back = len(outline), len(vertices)
    vertices.extend((point.x, depth + .6, point.y) for point in outline)
    center = sum(outline, Vector((0, 0))) / count
    vertices.append((center.x, depth + .6, center.y))
    for index in range(count):
        following = (index + 1) % count
        faces.append((index, back + index, back + following, following))
        faces.append((back + following, back + index, back + count))
