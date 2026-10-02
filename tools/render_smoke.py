#!/usr/bin/env python3
"""Exercise real shaders, assets and launch options from a clean working directory.

Python standard library only. Use --video-driver offscreen for Mesa/SDL Linux CI,
or omit it for a native-window run. This does not replace interactive QA.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time


def ppm(path):
    with path.open('rb') as stream:
        if stream.readline().strip() != b'P6':
            raise ValueError('Not a P6 screenshot')
        width, height = map(int, stream.readline().split())
        if stream.readline().strip() != b'255':
            raise ValueError('Unexpected pixel depth')
        pixels = stream.read()
    if width < 320 or height < 240 or len(pixels) != width * height * 3:
        raise ValueError('Invalid screenshot dimensions or truncated pixels')
    if len(set(pixels)) < 32:
        raise ValueError('Screenshot appears blank')
    return width, height, hashlib.sha256(pixels).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--suite', choices=['quick', 'full'], default='quick')
    parser.add_argument('--video-driver', default='')
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    if args.video_driver:
        env['SDL_VIDEODRIVER'] = args.video_driver
    cases = []
    for phase in (0, 60, 15):
        cases.append((f'earth-clouds-{phase}', ['--start', 'earth', '--hide-ui', '--earth-paused', '--earth-phase', str(phase)]))
    for phase in (0, 15):
        cases.append((f'earth-topography-{phase}', ['--start', 'earth', '--hide-ui', '--earth-topography', '--earth-paused', '--earth-phase', str(phase)]))
    cases += [
        ('earth-everest', ['--start', 'earth', '--landmark', '1', '--location-info', '--earth-paused']),
        ('earth-trench', ['--start', 'earth', '--landmark', '2', '--location-info', '--earth-paused', '--ui-scale', '2']),
        ('earth-guide', ['--start', 'earth', '--info', '--earth-paused']),
        ('earth-system', ['--start', 'system', '--body', 'earth', '--earth-paused']),
        ('solar', ['--start', 'solar', '--earth-paused', '--sun-paused', '--jupiter-paused', '--saturn-paused', '--ice-paused']),
    ]
    if args.suite == 'full':
        bodies = ['sun', 'mars', 'mercury', 'moon', 'venus', 'jupiter', 'saturn',
                  'phobos', 'deimos', 'io', 'europa', 'ganymede', 'callisto',
                  'titan', 'mimas', 'enceladus', 'tethys', 'dione', 'rhea', 'iapetus',
                  'uranus', 'neptune', 'miranda', 'ariel', 'umbriel', 'titania', 'oberon',
                  'triton', 'proteus', 'ceres', 'pluto', 'haumea', 'makemake', 'eris',
                  'asteroid-belt', 'kuiper-belt']
        cases += [(body, ['--start', body, '--coarse', '--moon-detail', '4', '--mercury-detail', '4']) for body in bodies]
        cases += [('venus-surface', ['--start', 'venus', '--venus-topography']),
                  ('moon-guide', ['--start', 'moon', '--moon-detail', '4', '--landmark', '16', '--location-info'])]
    results = []
    hashes = {}
    failure = None
    try:
        with tempfile.TemporaryDirectory(prefix='solar-smoke-cwd-') as cwd:
            # These must fail before SDL initialization; catches parser regressions.
            for option, value in [('--frames', '2junk'), ('--frames', '2.5'),
                                  ('--landmark', '-2147483648'), ('--landmark', '1junk'),
                                  ('--ui-scale', '1.5junk'), ('--ui-scale', 'nan'),
                                  ('--distance', '1e300'), ('--relief', '10junk')]:
                invalid = subprocess.run([str(executable), option, value], cwd=cwd, env=env,
                                         capture_output=True, text=True, timeout=10)
                if invalid.returncode != 1 or 'OpenGL:' in invalid.stdout:
                    raise RuntimeError(f'Invalid launch accepted: {option} {value}')
            for name, options in cases:
                image = output / f'{name}.ppm'
                image.unlink(missing_ok=True)
                start = time.monotonic()
                run = subprocess.run([str(executable), *options, '--frames', '3', '--screenshot', str(image)],
                                     cwd=cwd, env=env, capture_output=True, text=True, timeout=120)
                log = run.stdout + run.stderr
                (output / f'{name}.log').write_text(log)
                if run.returncode or re.search(r'unavailable|shader.*failed|opengl error|data.*failed', log, re.I):
                    raise RuntimeError(f'{name}: launch/asset/shader failure; see {name}.log')
                if 'Final view:' not in log:
                    raise RuntimeError(f'{name}: no completed render')
                width, height, digest = ppm(image)
                hashes[name] = (width, height, digest)
                if name == 'earth-trench' and not re.search(r'Earth:.*topography 1', log):
                    raise RuntimeError('Trench selection did not reveal seafloor')
                if name in ('earth-everest', 'earth-trench') and 'location open 1' not in log:
                    raise RuntimeError('Landmark notes did not open')
                results.append(dict(name=name, seconds=round(time.monotonic()-start, 3),
                                    width=width, height=height, pixels_sha256=digest))
                print(f'PASS {name}', flush=True)
        if hashes['earth-clouds-0'] != hashes['earth-clouds-60']:
            raise RuntimeError('Cloud loop endpoints differ (or window sizes changed)')
        if hashes['earth-clouds-0'] == hashes['earth-clouds-15']:
            raise RuntimeError('Clouds do not move')
        if hashes['earth-topography-0'] != hashes['earth-topography-15']:
            raise RuntimeError('Hidden clouds affect topography')
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        failure = str(error)
    report = dict(executable=str(executable), executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                  suite=args.suite, video_driver=args.video_driver or 'native', passed=failure is None,
                  completed=results, failure=failure)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    if failure:
        raise SystemExit(f'FAIL: {failure}')
    print(f'PASS: {len(results)} render cases, 8 invalid launches and cloud-loop comparisons')


if __name__ == '__main__':
    main()
