"""Export single-influence equipment and its first animation into native BMD v10."""
from dataclasses import dataclass
from pathlib import Path
import struct

from mathutils import Matrix

from export_prop_bmd import TRIANGLES_PER_MESH, c_string, encode_mesh, triangle_groups


@dataclass(frozen=True)
class ExportSpec:
    model: dict
    path: Path
    frames: int = 1


def skeleton_bytes(model, frames):
    if not 1 <= frames <= model['action_frames'][0]:
        raise ValueError('Animation frame count outside the supplied clip')
    locked = frames > 1 and model['action_locks'][0]
    result = bytearray(struct.pack('<hB', frames, locked))
    if locked:
        for position in model['action_positions'][0][:frames]:
            result.extend(struct.pack('<3f', *position))
    for bone in model['bones']:
        if bone.get('dummy'):
            result.extend(b'\1')
            continue
        result.extend(b'\0' + c_string(bone['name']))
        clip = bone['clips'][0]
        result.extend(struct.pack('<h', bone['parent']))
        for channel in ('positions', 'rotations'):
            for value in clip[channel][:frames]:
                result.extend(struct.pack('<3f', *value))
    return result


def export(collection, bind, spec):
    model, path = spec.model, spec.path
    grouped = triangle_groups(collection.objects, Matrix.Identity(4), bind)
    meshes = []
    for texture, triangles in sorted(grouped.items()):
        for start in range(0, len(triangles), TRIANGLES_PER_MESH):
            meshes.append(encode_mesh(triangles[start:start + TRIANGLES_PER_MESH], texture, len(meshes)))
    if len(meshes) > 50:
        raise ValueError('Equipment exceeds the client mesh limit')
    result = bytearray(b'BMD\x0a' + c_string(path.stem))
    result.extend(struct.pack('<3h', len(meshes), len(model['bones']), 1))
    for mesh in meshes:
        result.extend(mesh)
    result.extend(skeleton_bytes(model, spec.frames))
    path.write_bytes(result)
    return dict(file=path.name, meshes=len(meshes), triangles=sum(map(len, grouped.values())),
                textures=sorted(grouped), bones=len(model['bones']))
