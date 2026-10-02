#!/usr/bin/env python3
"""Prepare Earth color or cloud reference. Offline only: Python + ImageMagick."""
import argparse, hashlib, pathlib, subprocess, tempfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source',type=pathlib.Path)
p.add_argument('output',type=pathlib.Path)
p.add_argument('--clouds',action='store_true',help='extract NASA GEOS-5 frame alpha')
a=p.parse_args()
expected='b254a27e90bdea3901de3d8b69d74f6a0e9d1128acc9c9450ad48ced7eb4e764' if a.clouds else '4f4240673a3a1b173d61b92ca4b07bac5fd17059ea5f725ba6da5a9c5386b7ba'
if hashlib.sha256(a.source.read_bytes()).hexdigest()!=expected:
    raise SystemExit('Unexpected NASA reference; see docs/EARTH.md')
if a.output.exists(): raise SystemExit('Output exists; choose a new output filename')
with tempfile.TemporaryDirectory(prefix='earth-') as d:
    raw=pathlib.Path(d)/'pixels.raw'
    if a.clouds:
        command=['magick',str(a.source),'-alpha','extract','-depth','8','gray:'+str(raw)]
        width,height,channels,magic,version=2048,1024,1,'EARTH_CLOUDS',1
    else:
        command=['magick',str(a.source),'-colorspace','sRGB','-resize','4096x2048!',
                 '-depth','8','rgb:'+str(raw)]
        width,height,channels,magic,version=4096,2048,3,'EARTH_RGB',2
    subprocess.run(command,check=True); pixels=raw.read_bytes()
if len(pixels)!=width*height*channels: raise SystemExit('Unexpected converted size')
h=14695981039346656037
for b in pixels: h=((h^b)*1099511628211)&((1<<64)-1)
with a.output.open('xb') as f:
    f.write(f'{magic} {version} {width} {height} {h:016x}\n'.encode()); f.write(pixels)
print(f'{a.output}: {len(pixels)} bytes; FNV1a {h:016x}')
