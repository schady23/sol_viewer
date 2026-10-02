#!/usr/bin/env python3
"""Offline conversion only. Needs ImageMagick; the app has no image-library dependency."""
import argparse, hashlib, pathlib, subprocess, tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
expected = 'c3b915227ef88899a07a2b62584d9303d77d3491b9bb7de8824fb6df4a52f7b2'
if hashlib.sha256(args.source.read_bytes()).hexdigest() != expected:
    raise SystemExit('Source differs from the pinned NASA Hubble 2015a TIFF; refusing conversion')
if args.output.exists():
    raise SystemExit('Output exists; choose a new output filename')
with tempfile.TemporaryDirectory(prefix='jupiter-') as directory:
    raw = pathlib.Path(directory) / 'cloud.rgb'
    subprocess.run(['magick', str(args.source), '-colorspace', 'sRGB', '-depth', '8', 'rgb:' + str(raw)], check=True)
    pixels = raw.read_bytes()
if len(pixels) != 3600 * 1800 * 3:
    raise SystemExit('Unexpected converted size')
checksum = 14695981039346656037
for byte in pixels:
    checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
with args.output.open('xb') as output:
    output.write(f'JUPITER_RGB 2 3600 1800 {checksum:016x}\n'.encode('ascii'))
    output.write(pixels)
print(f'{args.output}: {len(pixels)} RGB bytes; FNV1a {checksum:016x}')
