#!/usr/bin/env python3
"""Offline preparation of pinned NASA VTAD Saturn moon visualization meshes."""
import argparse
import hashlib
import pathlib
from prepare_satellites import model, checksum

SOURCES = {'mimas': '66fc9c4b5f89313afa1a8214763525232608e37560044a614eb7cfaa876c47b1', 'rhea': '9cb351462fdc149303de79f9b8e33fb3deb9ce60102a190876a7ad280f307d57', 'titan': '086a5c1214c20f47dda068a49958a337b51e07b0477128d284408ac2f2676f4b', 'enceladus': '7548234bae15047d6c1af6bec4f348748b1acae7d4e7bfbe50e5bb3dda75506f', 'tethys': '5e195fa1faf454f7af6808d74a9416b0f97321536a0fcacc5687c0428a9d8659', 'dione': '9feee62be971d47ef55448e58304072e89d973c3a3454c812314e1f9dca27674', 'iapetus': '7a7212ac69e022cf37ae96fe4d34f04e9cfea00fd66a266aa3b1c970c475746d'}


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
    payload = model(raw, clean_poles=True)
    with args.output.open('xb') as out:
        out.write(f'{args.body.upper()}_MESH 1 {len(payload)//16} {checksum(payload):016x}\n'.encode())
        out.write(payload)
    print(args.output, len(payload)//48, 'triangles')


if __name__ == '__main__':
    main()
