"""Export authored pets with the complete native multi-action skeleton intact.

Equipment lots export one bind pose; pets must keep every native action, so
this writer reproduces the full audited skeleton block: all action headers
(frame counts, locks, locked root positions) and every per-bone clip of every
action, bit-for-bit from the frozen reference. Only the meshes are authored;
the mount rides with its seven DarkHorse actions and the eagle flies with its
four DarkSpirit actions exactly as the client plays them.
"""
from dataclasses import dataclass
from pathlib import Path
import struct

from mathutils import Matrix

from export_prop_bmd import TRIANGLES_PER_MESH, c_string, encode_mesh, triangle_groups


@dataclass(frozen=True)
class PetExportSpec:
    model: dict   # native reference inspected with full=True
    path: Path


def skeleton_bytes(model):
    """Every action header followed by per-bone clips for every action."""
    actions = range(len(model['action_frames']))
    result = bytearray()
    for action in actions:
        result.extend(struct.pack('<hB', model['action_frames'][action],
                                  model['action_locks'][action]))
        if model['action_locks'][action]:
            for position in model['action_positions'][action]:
                result.extend(struct.pack('<3f', *position))
    for bone in model['bones']:
        if bone.get('dummy'):
            result.extend(b'\1')
            continue
        result.extend(b'\0' + c_string(bone['name']))
        result.extend(struct.pack('<h', bone['parent']))
        for action in actions:
            clip = bone['clips'][action]
            for channel in ('positions', 'rotations'):
                for value in clip[channel]:
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
        raise ValueError('Pet exceeds the client mesh limit')
    result = bytearray(b'BMD\x0a' + c_string(path.stem))
    result.extend(struct.pack('<3h', len(meshes), len(model['bones']), len(model['action_frames'])))
    for mesh in meshes:
        result.extend(mesh)
    result.extend(skeleton_bytes(model))
    path.write_bytes(result)
    return dict(file=path.name, meshes=len(meshes), triangles=sum(map(len, grouped.values())),
                textures=sorted(grouped), bones=len(model['bones']),
                actions=len(model['action_frames']),
                action_frames=list(model['action_frames']))
