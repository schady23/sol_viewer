#!/usr/bin/env python3
"""Offline preparation of pinned NASA GLBs and USGS moon mosaics (ImageMagick)."""
import argparse
import hashlib
import json
import math
import pathlib
import struct
import subprocess
import tempfile

SOURCES = {
    'europa': 'a323f0c9ccb47d5af9902ea8297fe81f9a9708795645b80801f103c3f7c9a624',
    'ganymede': 'c2c8d9506b8cf8f7a0a90d823d9052e91c8d9885cf7267fdce8de8216f4df888',
    'phobos': '757712101e4fbcf9527942ee664a3bc6dfe3ab460466b760f4bf51ee85dbd8d3',
    'deimos': '5d5da763970acffe88aac26d3c3bd0fa66c5bef251d1bb4bfe2ae5fd723507e4',
}


def checksum(payload):
    value = 14695981039346656037
    for byte in payload:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value


def model(raw, clean_poles=False):
    magic, version, size = struct.unpack_from('<III', raw)
    assert magic == 0x46546c67 and version == 2 and size == len(raw)
    length, kind = struct.unpack_from('<II', raw, 12)
    assert kind == 0x4e4f534a
    doc = json.loads(raw[20:20+length])
    offset = 20 + length
    length, kind = struct.unpack_from('<II', raw, offset)
    assert kind == 0x004e4942
    binary = raw[offset+8:offset+8+length]
    assert doc['nodes'] == [{'mesh': 0, 'name': doc['nodes'][0]['name']}]

    def view(index):
        v = doc['bufferViews'][index]
        return binary[v.get('byteOffset', 0):v.get('byteOffset', 0)+v['byteLength']]

    def accessor(index, fmt):
        a = doc['accessors'][index]
        assert 'sparse' not in a and a.get('byteOffset', 0) == 0
        return list(struct.iter_unpack(fmt, view(a['bufferView'])))

    primitive, = doc['meshes'][0]['primitives']
    positions = accessor(primitive['attributes']['POSITION'], '<fff')
    uv = accessor(primitive['attributes']['TEXCOORD_0'], '<ff')
    indices = [i[0] for i in accessor(primitive['indices'], '<H')]
    material = doc['materials'][primitive['material']]
    texture = material['pbrMetallicRoughness']['baseColorTexture']['index']
    image_index = doc['textures'][texture]['source']
    image = view(doc['images'][image_index]['bufferView'])
    with tempfile.TemporaryDirectory() as tmp:
        path = pathlib.Path(tmp) / 'texture.png'
        path.write_bytes(image)
        w, h = map(int, subprocess.check_output(['magick', 'identify', '-format', '%w %h', str(path)]).split())
        rgb = subprocess.check_output(['magick', str(path), '-alpha', 'off', '-depth', '8', 'rgb:-'])
    radius = max(math.sqrt(sum(x*x for x in p)) for p in positions)
    payload = bytearray()

    def triangle(points, texcoords, level):
        if level:
            mid = lambda a, b: tuple((x+y)/2 for x, y in zip(a, b))
            a, b, c = points
            ta, tb, tc = texcoords
            ab, bc, ca = mid(a,b), mid(b,c), mid(c,a)
            tab, tbc, tca = mid(ta,tb), mid(tb,tc), mid(tc,ta)
            for p, t in [((a,ab,ca),(ta,tab,tca)), ((ab,b,bc),(tab,tb,tbc)),
                         ((ca,bc,c),(tca,tbc,tc)), ((ab,bc,ca),(tab,tbc,tca))]:
                triangle(p, t, level-1)
            return
        if clean_poles:
            # Some VTAD spheres include almost coincident pole vertices. Remove
            # only zero-area slivers after float32 normalization; otherwise they
            # acquire reversed winding through rounding in the native loader.
            a, b, c = [struct.unpack('<fff',struct.pack('<fff',*(x/radius for x in p))) for p in points]
            ab, ac = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
            cross = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
            orientation = sum(cross[i]*a[i] for i in range(3))
            if abs(orientation) < 1e-10:
                return
            if orientation < 0:
                raise ValueError('Unexpected reversed NASA source triangle')
        # Preserve authored UV islands and geometry; splitting is planar, never
        # radial normalization or invented terrain. glTF UV origin is top left.
        u, v = (sum(t[i] for t in texcoords)/3 for i in range(2))
        x, y = min(w-1, max(0, int(u*w))), min(h-1, max(0, int(v*h)))
        color = bytes(round(round(c/255*31)/31*255) for c in rgb[(y*w+x)*3:(y*w+x)*3+3]) + b'\xff'
        for p in points:
            payload.extend(struct.pack('<fff', *(x/radius for x in p)) + color)

    for i in range(0, len(indices), 3):
        ids = indices[i:i+3]
        triangle([positions[j] for j in ids], [uv[j] for j in ids], 2)
    return payload


def mosaic(body, source):
    expected = '19631x9816' if body == 'europa' else '16539x8270'
    size = subprocess.check_output(['magick', 'identify', '-format', '%wx%h', str(source)], text=True)
    if size != expected:
        raise SystemExit('Unexpected source dimensions')
    gray = subprocess.check_output(['magick', str(source), '-alpha', 'off', '-filter', 'Lanczos',
                                    '-resize', '4096x2048!', '-depth', '8', 'gray:-'])
    if len(gray) != 4096*2048:
        raise SystemExit('Unexpected decoded raster length')
    # Both GeoTIFFs have centre longitude 180 E, positive projected X to the
    # right: left edge 0 E, right edge 360 E. Shift, never mirror the raster.
    gray = b''.join(gray[y*4096+2048:(y+1)*4096] + gray[y*4096:y*4096+2048] for y in range(2048))
    ramp = ((27,228),(24,219),(18,195)) if body == 'europa' else ((16,222),(15,212),(13,194))
    palette = [bytes((28,29,28))] + [bytes(round(offset+gain*(v/255)**.85) for offset,gain in ramp) for v in range(1,256)]
    return b''.join(palette[v] for v in gray)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('body', choices=SOURCES)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    raw = args.source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCES[args.body]:
        raise SystemExit('Unexpected source SHA256')
    shape = args.body in ('phobos', 'deimos')
    payload = model(raw) if shape else mosaic(args.body, args.source.resolve())
    header = f'{args.body.upper()}_MESH 1 {len(payload)//16}' if shape else f'{args.body.upper()}_RGB 1 4096 2048'
    with args.output.open('xb') as out:
        out.write(f'{header} {checksum(payload):016x}\n'.encode())
        out.write(payload)
    print(args.output, len(payload), 'bytes')


if __name__ == '__main__':
    main()
