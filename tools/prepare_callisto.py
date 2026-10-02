#!/usr/bin/env python3
"""Prepare the pinned USGS Callisto monochrome mosaic with an artistic display tint."""
import argparse
import hashlib
import pathlib
import shutil
import subprocess

SOURCE_SHA256 = 'e1f0bd2e0e05de605d067d6b5f5ededddaf31ca6c064562a1ca770f15a7dbaa3'
WIDTH, HEIGHT = 4096, 2048


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit('Output exists; choose a new filename')
    if hashlib.sha256(args.source.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise SystemExit('Unexpected USGS Callisto source SHA256')
    magick = shutil.which('magick')
    if not magick:
        raise SystemExit('ImageMagick is required for offline TIFF decoding')
    source = str(args.source.resolve())
    size = subprocess.check_output([magick, 'identify', '-format', '%wx%h', source], text=True)
    if size != '15138x7569':
        raise SystemExit('Unexpected source dimensions')
    gray = subprocess.check_output([
        magick, source, '-alpha', 'off', '-filter', 'Lanczos',
        '-resize', f'{WIDTH}x{HEIGHT}!', '-depth', '8', 'gray:-'
    ])
    if len(gray) != WIDTH * HEIGHT:
        raise SystemExit('Unexpected decoded payload length')
    # GeoTIFF centre longitude is 180 E: raster runs 0..360 E, north to south.
    # Shift half a row to the atlas convention, -180..180 E. Do not mirror.
    gray = b''.join(gray[y*WIDTH+WIDTH//2:(y+1)*WIDTH] +
                    gray[y*WIDTH:y*WIDTH+WIDTH//2] for y in range(HEIGHT))
    # A single luminance-based stone/ice ramp; this does not encode composition.
    # Zero is the TIFF's no-data value: show it as flat charcoal, never invented detail.
    palette = [bytes((35, 36, 35))] + [
        bytes(round(offset + gain * (v / 255) ** .72)
              for offset, gain in ((14, 224), (13, 214), (12, 198)))
        for v in range(1, 256)
    ]
    payload = b''.join(palette[v] for v in gray)
    checksum = 14695981039346656037
    for byte in payload:
        checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
    with args.output.open('xb') as output:
        output.write(f'CALLISTO_RGB 1 {WIDTH} {HEIGHT} {checksum:016x}\n'.encode())
        output.write(payload)
    print(f'{args.output}: {len(payload)} bytes, FNV1a {checksum:016x}')


if __name__ == '__main__':
    main()
