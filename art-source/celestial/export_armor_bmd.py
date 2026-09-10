"""Export independently authored armor with the player's native attachment indices."""
import struct

from mathutils import Matrix

from export_prop_bmd import TRIANGLES_PER_MESH, c_string, encode_mesh, triangle_groups


def skeleton_bytes(model):
    result = bytearray(struct.pack('<hB', 1, 0))
    for bone in model['bones']:
        if bone.get('dummy'):
            result.extend(b'\1')
            continue
        result.extend(b'\0' + c_string(bone['name']))
        clip = bone['clips'][0]
        result.extend(struct.pack('<h6f', bone['parent'], *clip['positions'][0], *clip['rotations'][0]))
    return result


def export(collection, bind, model, path):
    grouped = triangle_groups(collection.objects, Matrix.Identity(4), bind)
    meshes = []
    for texture, triangles in sorted(grouped.items()):
        for start in range(0, len(triangles), TRIANGLES_PER_MESH):
            meshes.append(encode_mesh(triangles[start:start + TRIANGLES_PER_MESH], texture, len(meshes)))
    if len(meshes) > 50:
        raise ValueError('Armor exceeds the client mesh limit')
    result = bytearray(b'BMD\x0a' + c_string(path.stem))
    result.extend(struct.pack('<3h', len(meshes), len(model['bones']), 1))
    for mesh in meshes:
        result.extend(mesh)
    result.extend(skeleton_bytes(model))
    path.write_bytes(result)
    return dict(file=path.name, meshes=len(meshes), triangles=sum(map(len, grouped.values())),
                textures=sorted(grouped), bones=len(model['bones']))
