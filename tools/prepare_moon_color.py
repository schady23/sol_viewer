#!/usr/bin/env python3
"""Offline conversion only. Needs ImageMagick; the app has no image-library dependency."""
import argparse, hashlib, pathlib, subprocess, tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
expected = '4af8b0cd4d50c30851359d98e7e72040240dd8d03256b58b345b5b76e9edb4ef'
if hashlib.sha256(args.source.read_bytes()).hexdigest() != expected:
    raise SystemExit('Source differs from the pinned NASA SVS 2019 LROC 8K TIFF; refusing conversion')
if args.output.exists():
    raise SystemExit('Output exists; choose a new output filename')
with tempfile.TemporaryDirectory(prefix='moon-') as directory:
    raw = pathlib.Path(directory) / 'cloud.rgb'
    subprocess.run(['magick', str(args.source), '-colorspace', 'sRGB', '-depth', '8', 'rgb:' + str(raw)], check=True)
    pixels = raw.read_bytes()
if len(pixels) != 8192 * 4096 * 3:
    raise SystemExit('Unexpected converted size')
checksum = 14695981039346656037
for byte in pixels:
    checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
with args.output.open('xb') as output:
    output.write(f'MOON_RGB 1 8192 4096 {checksum:016x}\n'.encode('ascii'))
    output.write(pixels)
print(f'{args.output}: {len(pixels)} RGB bytes; FNV1a {checksum:016x}')
