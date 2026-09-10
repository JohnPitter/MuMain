"""Extract native MU BMD skeletons and animation evidence without changing assets."""
import argparse
import hashlib
import json
import struct
from pathlib import Path


class Reader:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def take(self, size):
        end = self.offset + size
        if size < 0 or end > len(self.data):
            raise ValueError(f"Truncated BMD at {self.offset}, requested {size}")
        result = self.data[self.offset:end]
        self.offset = end
        return result

    def unpack(self, fmt):
        return struct.unpack('<' + fmt, self.take(struct.calcsize('<' + fmt)))

    def name(self):
        return self.take(32).split(b'\0', 1)[0].decode('ascii', errors='replace')


def payload(raw):
    if raw[:3] != b'BMD' or len(raw) < 4:
        raise ValueError('Invalid BMD header')
    if raw[3] == 10:
        return raw[4:]
    if raw[3] != 12:
        raise ValueError(f'Unsupported BMD version: {raw[3]}')
    size, = struct.unpack('<I', raw[4:8])
    if size != len(raw) - 8:
        raise ValueError('Encrypted payload length mismatch')
    key = bytes.fromhex('d17352f6d29acb273eaf593137b3e7a2')
    state = 0x5e
    result = bytearray(size)
    for index, value in enumerate(raw[8:]):
        result[index] = ((value ^ key[index % 16]) - state) & 255
        state = (value + 0x3d) & 255
    return result


def read_mesh(reader, full=False):
    vertices, normals, uv, triangles, texture = reader.unpack('5h')
    if min(vertices, normals, uv, triangles) < 0:
        raise ValueError('Negative mesh element count')
    used_bones = set()
    points = []
    for _ in range(vertices):
        node, *position = reader.unpack('h2x3f')
        used_bones.add(node)
        points.append((node, position))
    normal_vectors = [reader.unpack('h2x3fh2x') for _ in range(normals)]
    texcoords = [reader.unpack('2f') for _ in range(uv)]
    faces = []
    normal_indices = []
    for _ in range(triangles):
        record = reader.take(64)
        corners = record[0]
        if corners not in (3, 4):
            raise ValueError('Invalid polygon')
        faces.append((struct.unpack_from('<4h', record, 2)[:corners],
                      struct.unpack_from('<4h', record, 18)[:corners]))
        normal_indices.append(struct.unpack_from('<4h', record, 10)[:corners])
    result = dict(vertices=vertices, triangles=triangles, texture=reader.name(),
                  texture_index=texture, used_bones=sorted(used_bones))
    if full:
        result.update(points=points, texcoords=texcoords, faces=faces,
                      normal_vectors=normal_vectors, normal_indices=normal_indices)
    return result


def read_bone(reader, actions, full=False):
    dummy, = reader.unpack('B')
    if dummy:
        return dict(dummy=True)
    name = reader.name()
    parent, = reader.unpack('h')
    clips = []
    for frames in actions:
        positions = [reader.unpack('3f') for _ in range(frames)]
        rotations = [reader.unpack('3f') for _ in range(frames)]
        clips.append(dict(first_position=positions[0] if frames else None,
                          first_rotation=rotations[0] if frames else None,
                          moves=len(set(positions)) > 1,
                          rotates=len(set(rotations)) > 1))
        if full:
            clips[-1].update(positions=positions, rotations=rotations)
    return dict(name=name, parent=parent, clips=clips)


def inspect(path, full=False):
    raw = path.read_bytes()
    reader = Reader(payload(raw))
    name = reader.name()
    mesh_count, bone_count, action_count = reader.unpack('3H')
    if max(mesh_count, bone_count, action_count) > 4096:
        raise ValueError('Unexpected model counts')
    meshes = [read_mesh(reader, full) for _ in range(mesh_count)]
    actions, locks, root_positions = [], [], []
    for _ in range(action_count):
        frames, locked = reader.unpack('hB')
        if frames < 0:
            raise ValueError('Negative frame count')
        positions = [reader.unpack('3f') for _ in range(frames)] if locked else []
        actions.append(frames)
        locks.append(bool(locked))
        root_positions.append(positions)
    bones = [read_bone(reader, actions, full) for _ in range(bone_count)]
    if reader.offset != len(reader.data):
        raise ValueError(f'Unread payload bytes: {len(reader.data) - reader.offset}')
    return dict(file=str(path.resolve()), sha256=hashlib.sha256(raw).hexdigest(),
                name=name, meshes=meshes, action_frames=actions, action_locks=locks,
                action_positions=root_positions, bones=bones)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', type=Path, nargs='+')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = [inspect(path) for path in args.files]
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    for model in report:
        animated = sum(any(c['moves'] or c['rotates'] for c in b.get('clips', []))
                       for b in model['bones'])
        print(Path(model['file']).name, len(model['bones']), 'bones;',
              model['action_frames'], 'frames;', animated, 'animated bones')
