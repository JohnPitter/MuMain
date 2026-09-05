"""Parse World7 Object*.bmd models (v0x0C) and report per-model XZ bounding
boxes + texture names, plus EncTerrain7.obj placement patterns per type.
Enough to decide which object types block walkability and how many tiles.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

REF = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_arena-ref")
sys.path.insert(0, str(REF))
import muatt  # noqa: E402

OBJ_DIR = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_wt-arena3\src\bin\Data\Object7")
WORLD = Path(r"C:\Users\joaop\Desenvolvimento\openmu\_wt-arena3\src\bin\Data\World7")


def bmd_aabb(path: Path) -> dict:
    raw = path.read_bytes()
    assert raw[:3] == b"BMD", path
    version = raw[3]
    if version == 0x0C:
        (enc_size,) = struct.unpack_from("<i", raw, 4)
        enc = raw[8:8 + enc_size]
        data = muatt.map_decrypt(enc)
    elif version == 0x0A:
        data = raw  # unencrypted classic layout; payload starts after "BMD"+ver
        ptr = 4
    else:
        raise SystemExit(f"{path.name}: version {version} not handled")
    ptr = 0 if version == 0x0C else 4
    ptr = 4 if version == 0x0A else 0
    name = data[ptr:ptr + 32].split(b"\x00")[0].decode("latin1")
    ptr += 32
    num_meshs, num_bones, num_actions = struct.unpack_from("<hhh", data, ptr)
    ptr += 6
    minx = miny = minz = 1e9
    maxx = maxy = maxz = -1e9
    texnames = []
    for _ in range(num_meshs):
        nv, nn, nt, ntri, tex = struct.unpack_from("<hhhhh", data, ptr)
        ptr += 10
        for v in range(nv):
            # Vertex_t: short Node (+2 pad), vec3 Position
            x, y, z = struct.unpack_from("<fff", data, ptr + 4)
            minx, maxx = min(minx, x), max(maxx, x)
            miny, maxy = min(miny, y), max(maxy, y)
            minz, maxz = min(minz, z), max(maxz, z)
            ptr += 16
        ptr += nn * 20          # Normal_t
        ptr += nt * 8           # TexCoord_t
        ptr += ntri * 64        # Triangle_t2 (as read by the client)
        texname = data[ptr:ptr + 32].split(b"\x00")[0].decode("latin1")
        ptr += 32
        texnames.append(texname)
    return {
        "name": name, "meshes": num_meshs,
        "min": (minx, miny, minz), "max": (maxx, maxy, maxz),
        "size": (maxx - minx, maxy - miny, maxz - minz),
        "consumed": ptr, "total": len(data), "texnames": texnames,
    }


def main() -> None:
    print("=== Object7 model bounds (client units; 100 units = 1 tile) ===")
    bounds = {}
    for i in range(1, 42):
        p = OBJ_DIR / f"Object{i:02d}.bmd"
        if not p.exists():
            print(f"type {i - 1:2d}: MISSING {p.name}")
            continue
        try:
            b = bmd_aabb(p)
        except Exception as exc:  # noqa: BLE001
            print(f"type {i - 1:2d}: FAIL {exc}")
            continue
        bounds[i - 1] = b
        ok = "ok" if b["consumed"] == b["total"] else f"consumed {b['consumed']}/{b['total']}"
        sx, sy, sz = b["size"]
        print(f"type {i - 1:2d}: size=({sx:7.1f},{sy:7.1f},{sz:7.1f}) "
              f"min=({b['min'][0]:7.1f},{b['min'][1]:7.1f},{b['min'][2]:7.1f}) "
              f"meshes={b['meshes']} tex={b['texnames'][:2]} [{ok}]")

    print("\n=== EncTerrain7.obj placements per type ===")
    dec = muatt.map_decrypt((WORLD / "EncTerrain7.obj").read_bytes())
    (count,) = struct.unpack_from("<h", dec, 2)
    off = 4
    per_type: dict[int, list] = {}
    for _ in range(count):
        (typ,) = struct.unpack_from("<h", dec, off)
        x, y, z = struct.unpack_from("<fff", dec, off + 2)
        ax, ay, az = struct.unpack_from("<fff", dec, off + 14)
        (scale,) = struct.unpack_from("<f", dec, off + 26)
        off += 30
        per_type.setdefault(typ, []).append((x, y, z, ax, ay, az, scale))
    for typ in sorted(per_type):
        items = per_type[typ]
        xs = [it[0] for it in items]
        ys = [it[1] for it in items]
        zs = [it[2] for it in items]
        ays = sorted({round(it[4], 1) for it in items})
        scales = sorted({round(it[6], 2) for it in items})
        print(f"type {typ:3d} n={len(items):4d} "
              f"x[{min(xs):6.0f}..{max(xs):6.0f}] y[{min(ys):6.0f}..{max(ys):6.0f}] "
              f"z[{min(zs):6.0f}..{max(zs):6.0f}] yaw{ays[:6]} scale{scales[:4]}")


if __name__ == "__main__":
    main()
