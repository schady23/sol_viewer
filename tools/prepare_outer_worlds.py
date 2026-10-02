#!/usr/bin/env python3
"""Offline preparation of pinned NASA VTAD outer planet and moon visualization meshes."""
import argparse
import hashlib
import pathlib
import json
import struct
from prepare_satellites import model, checksum

SOURCES = {'uranus': '9c56f96bbfa82d923479c3dd83a00c35238a36addcfbcc61916f1377fe586bd5', 'neptune': '2fc3af08d56ed429b9cc56b95d151102aa5807ea958e7cb8806a42e12a699ba8', 'miranda': '5a4d24c5fb237ae6d21d1a58857efb32fd0e53162754f17e4156618a541c4c57', 'ariel': 'b7a5cd8ad3d48687cc3162861971a76a19fe9fad4726563e24b6cc917e9bd858', 'umbriel': 'b523de7d1406289d540369c635d0b13576460c1fa85a885352afc71c3f118d71', 'titania': '08556622082b283f2f52c9214636c22acf27300889374dd37197f11a4448324c', 'oberon': '7b42c417e523db2a22c744cd471587c26f07581564dae7c3b0c20986eea93add', 'triton': 'a25df7ee7fe8679e3da8cca7f1a09ec7241ad5e0c7ad97a39f057b8c1a0043ce'}


def cloud_model(raw, body):
    # VTAD planets have a Z polar axis and a +90 degree X node rotation.
    # Preserve authored UVs, recenter millimetric exporter offsets, and calibrate
    # flattening to NASA's 1-bar equatorial/polar radii before the common bake.
    length = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20+length])
    node, = doc['nodes']
    assert node['mesh'] == 0 and len(node['rotation']) == 4
    assert all(abs(a-b)<1e-6 for a,b in zip(node['rotation'],(.70710678,0,0,.70710678)))
    del node['rotation']
    binary = bytearray(raw[28+length:])
    primitive, = doc['meshes'][0]['primitives']
    a = doc['accessors'][primitive['attributes']['POSITION']]
    view = doc['bufferViews'][a['bufferView']]
    start = view.get('byteOffset',0)+a.get('byteOffset',0)
    points = [struct.unpack_from('<fff',binary,start+12*i) for i in range(a['count'])]
    lo = [min(p[k] for p in points) for k in range(3)]
    hi = [max(p[k] for p in points) for k in range(3)]
    centre = [(a+b)/2 for a,b in zip(lo,hi)]
    half = [(b-a)/2 for a,b in zip(lo,hi)]
    q = 24973/25559 if body=='uranus' else 24341/24764
    for i,p in enumerate(points):
        x,y,z = [(p[k]-centre[k])/half[k] for k in range(3)]
        struct.pack_into('<fff',binary,start+12*i,x,-z*q,y)
    encoded = json.dumps(doc,separators=(',',':')).encode()
    encoded += b' ' * (-len(encoded)%4)
    binary += b'\0' * (-len(binary)%4)
    patched = struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))
    patched += struct.pack('<II',len(encoded),0x4e4f534a)+encoded
    patched += struct.pack('<II',len(binary),0x004e4942)+binary
    return model(patched,clean_poles=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('body', choices=SOURCES)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit('Output exists; choose a new filename')
    raw = args.source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCES[args.body]:
        raise SystemExit('Unexpected NASA moon GLB SHA256')
    payload = cloud_model(raw,args.body) if args.body in ('uranus','neptune') else model(raw, clean_poles=True)
    with args.output.open('xb') as out:
        out.write(f'{args.body.upper()}_MESH 1 {len(payload)//16} {checksum(payload):016x}\n'.encode())
        out.write(payload)
    print(args.output, len(payload)//48, 'triangles')


if __name__ == '__main__':
    main()
