#!/usr/bin/env python3
"""Prepare JPL's inferred-color Venus radar composite; needs ImageMagick offline."""
import argparse
import hashlib
import pathlib
import shutil
import subprocess

SOURCE_SHA256 = "59a749343de9cc5121a6d470b44b6e59dec15edaf9232ce5ec46427bdcea7bc0"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit("Output exists; choose a new filename")
    if hashlib.sha256(args.source.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise SystemExit("Unexpected JPL ven0ajj2.tif source SHA256")
    magick = shutil.which("magick")
    if not magick:
        raise SystemExit("ImageMagick is required for offline TIFF decoding")
    # Preserve the source's north-to-south rows and -180..180 east longitude.
    payload = subprocess.run(
        [magick, str(args.source.resolve()), "-alpha", "off", "-colorspace", "sRGB",
         "-depth", "8", "rgb:-"], check=True, stdout=subprocess.PIPE
    ).stdout
    if len(payload) != 1440 * 720 * 3:
        raise SystemExit("Unexpected decoded source dimensions")
    checksum = 14695981039346656037
    for byte in payload:
        checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
    with args.output.open("xb") as output:
        output.write(f"VENUS_RGB 1 1440 720 {checksum:016x}\n".encode())
        output.write(payload)
    print(f"{args.output}: {len(payload)} bytes, FNV1a {checksum:016x}")


if __name__ == "__main__":
    main()
