"""Prototype-only v10 export using the project's golden BMD mesh encoder."""
import struct

from mathutils import Matrix, Vector

from export_prop_bmd import TRIANGLES_PER_MESH, c_string, encode_mesh, triangle_groups

# Authoring stays Z-up; this converts the long axis to the usual MU hand-local -Y.
# Final scepter/trident attachment needs a separate in-client animation acceptance.
HAND_TRANSFORM = Matrix(((0, -1, 0, 0), (0, 0, -1, 0), (1, 0, 0, 0), (0, 0, 0, 1)))
ANCHORS = {'Trident': ((0, 0, 0), (0, -2.2, 70), (0, 0, 136)),
           'Scepter': ((0, 0, 0), (0, -2.2, 70), (0, 0, 110))}


def export_weapon(collection, name, destination):
    grouped = triangle_groups(collection.objects, HAND_TRANSFORM)
    meshes = []
    for texture, triangles in sorted(grouped.items()):
        for start in range(0, len(triangles), TRIANGLES_PER_MESH):
            meshes.append(encode_mesh(triangles[start:start + TRIANGLES_PER_MESH], texture, len(meshes)))
    result = bytearray(b'BMD\x0a' + c_string('Poseidon_' + name))
    result.extend(struct.pack('<3h', len(meshes), 3, 1))
    for encoded in meshes:
        result.extend(encoded)
    result.extend(struct.pack('<hB', 1, 0))
    for index, (label, point) in enumerate(zip(('Grip', 'Core', 'Tip'), ANCHORS[name])):
        result.extend(b'\0' + c_string(f'Poseidon_{name}_{label}'))
        position = HAND_TRANSFORM @ Vector(point)
        result.extend(struct.pack('<h6f', -1 if index == 0 else 0, *position, 0, 0, 0))
    destination.write_bytes(result)
    return dict(file=destination.name, mesh_count=len(meshes), bones=3, action_frames=[1],
                triangles=sum(map(len, grouped.values())), textures=sorted(grouped),
                status='PROTOTYPE_NOT_INSTALLABLE_MISSING_TEXTURE_ATLASES')
