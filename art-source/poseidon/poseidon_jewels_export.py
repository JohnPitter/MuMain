"""Rigid item-space export for the Poseidon jewelry prototypes.

The pendant and rings are character items, not Class305-skinned equipment:
they follow the audited native item contract (Data/Item/saint.bmd in
native-reference-audit.json - rigid meshes, single static pose, geometry on
bone 0). Anchor bones name the mount points a future equip/integration pass
will consume. The pendant reuses the Celestial pendant item-space convention
(long axis into -Y, face toward +Z with a small lift); the rings are authored
already in display space (band circle in XZ, finger axis Y, crest up) and
export with an identity transform.
"""
import struct

from mathutils import Matrix, Vector

from export_prop_bmd import TRIANGLES_PER_MESH, c_string, encode_mesh, triangle_groups

ITEM_TRANSFORMS = {
    'Pendant': Matrix(((1, 0, 0, 0), (0, 0, -1, 5), (0, 1, 0, 1), (0, 0, 0, 1))),
    'RingTide': Matrix.Identity(4),
    'RingEmperor': Matrix.Identity(4),
}
# Authored Z-up anchor positions per piece, in the same space as the shapes.
AUTHORED_ANCHORS = {
    'Pendant': (('Seat', (0, -0.9, 0)), ('Bail', (0, -0.9, 16.9)),
                ('Drop', (0, -1.7, -15.6))),
    'RingTide': (('Band', (0, 0, 0)), ('Gem', (0, -3.5, 9.6))),
    'RingEmperor': (('Band', (0, 0, 0)), ('Gem', (0, -3.6, 9.2))),
}


def export_jewel(collection, name, destination):
    transform = ITEM_TRANSFORMS[name]
    grouped = triangle_groups(collection.objects, transform)
    meshes = []
    for texture, triangles in sorted(grouped.items()):
        for start in range(0, len(triangles), TRIANGLES_PER_MESH):
            meshes.append(encode_mesh(triangles[start:start + TRIANGLES_PER_MESH],
                                      texture, len(meshes)))
    anchors = [(label, transform @ Vector(point)) for label, point in AUTHORED_ANCHORS[name]]
    result = bytearray(b'BMD\x0a' + c_string('Poseidon_' + name))
    result.extend(struct.pack('<3h', len(meshes), len(anchors), 1))
    for mesh in meshes:
        result.extend(mesh)
    result.extend(struct.pack('<hB', 1, 0))
    for index, (label, position) in enumerate(anchors):
        result.extend(b'\0' + c_string(f'Poseidon_{name}_{label}'))
        result.extend(struct.pack('<h6f', -1 if index == 0 else 0,
                                  *position, 0, 0, 0))
    destination.write_bytes(result)
    return dict(file=destination.name, meshes=len(meshes), bones=len(anchors),
                action_frames=[1], triangles=sum(map(len, grouped.values())),
                textures=sorted(grouped),
                anchors={label: [round(value, 4) for value in position]
                         for label, position in anchors},
                status='PROTOTYPE_NOT_INSTALLABLE; definitive atlases authored in art-source')
