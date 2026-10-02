#!/usr/bin/env python3
"""Prepare the pinned USGS Io color-merged mosaic. ImageMagick is offline-only."""
import argparse
import hashlib
import pathlib
import shutil
import subprocess

SOURCE_SHA256 = "135f88cb645bfdb4e045b380971ab2ba14e230590c96c9c86cfe7310fda881ac"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit("Output exists; choose a new filename")
    if hashlib.sha256(args.source.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise SystemExit("Unexpected USGS Io source SHA256")
    magick = shutil.which("magick")
    if not magick:
        raise SystemExit("ImageMagick is required for offline TIFF decoding")
    source = str(args.source.resolve())
    size = subprocess.check_output([magick, "identify", "-format", "%wx%h", source], text=True)
    if size != "11445x5723":
        raise SystemExit("Unexpected source dimensions")
    # The GeoTIFF runs north-to-south and -180 to +180 EAST left-to-right.
    # West-positive catalogue coordinates describe feature labels, not a reason
    # to flip this raster. Pele and Loki provide independent registration checks.
    payload = subprocess.check_output([
        magick, source, "-alpha", "off", "-colorspace", "sRGB",
        "-filter", "Lanczos", "-resize", "4096x2048!", "-depth", "8", "rgb:-"
    ])
    if len(payload) != 4096 * 2048 * 3:
        raise SystemExit("Unexpected decoded payload length")
    checksum = 14695981039346656037
    for byte in payload:
        checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
    with args.output.open("xb") as output:
        output.write(f"IO_RGB 1 4096 2048 {checksum:016x}\n".encode())
        output.write(payload)
    print(f"{args.output}: {len(payload)} bytes, FNV1a {checksum:016x}")


if __name__ == "__main__":
    main()
