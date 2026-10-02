#!/usr/bin/env python3
"""Verify an installed app tree against this checkout and write its SHA-256 manifest."""
import argparse
import hashlib
from pathlib import Path


def digest(path):
    checksum = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            checksum.update(block)
    return checksum.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    parser.add_argument('--terrain-dir', type=Path, help='Same prepared data directory as SOLAR_TERRAIN_DIR')
    args = parser.parse_args()
    package = args.package.resolve(strict=True)
    root = Path(__file__).resolve().parents[1]
    terrain = args.terrain_dir.resolve(strict=True) if args.terrain_dir else root/'data'
    binary = package / ('solar_system_sim.exe' if (package/'solar_system_sim.exe').exists() else 'solar_system_sim')
    required = [binary, package/'THIRD_PARTY_NOTICES.md', package/'licenses/SDL3.txt']
    required += [package/'data'/name for name in ('mars16.mola', 'mercury64.dem', 'moon64.dem')]
    required += [package/'assets'/p.relative_to(root/'assets') for p in (root/'assets').rglob('*') if p.is_file()]
    for path in required:
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f'Missing/empty release file: {path}')
    lines = []
    total = 0
    for path in sorted(package.rglob('*')):
        if not path.is_file() or path.name == 'MANIFEST.sha256':
            continue
        if not path.resolve().is_relative_to(package):
            raise SystemExit(f'External package symlink: {path}')
        relative = path.relative_to(package)
        checksum = digest(path)
        if relative.parts[0] in ('assets', 'data'):
            original = terrain/Path(*relative.parts[1:]) if relative.parts[0] == 'data' else root/relative
            if not original.is_file() or digest(original) != checksum:
                raise SystemExit(f'Packaged resource differs from source: {relative}')
        lines.append(f'{checksum}  {relative.as_posix()}')
        total += path.stat().st_size
    (package/'MANIFEST.sha256').write_text('\n'.join(lines)+'\n')
    print(f'PASS: {len(lines)} packaged files, {total/1024**3:.2f} GiB; resource hashes match source')


if __name__ == '__main__':
    main()
