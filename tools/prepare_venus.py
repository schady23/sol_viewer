#!/usr/bin/env python3
"""Prepare the pinned USGS Magellan v02 DEM. Requires NumPy and Pillow offline only."""
import argparse, hashlib, pathlib
import numpy as np
from PIL import Image
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('source',type=pathlib.Path); p.add_argument('output',type=pathlib.Path)
a=p.parse_args()
if a.output.exists(): raise SystemExit('Output exists; choose a new filename')
raw=a.source.read_bytes()
expected='92dd2c8850b128f9dcceb400f1367e3df96229f5309840fee4275b74f192ea11'
if hashlib.sha256(raw).hexdigest()!=expected: raise SystemExit('Unexpected source SHA256')
with Image.open(a.source) as im:
    t=im.tag_v2
    if im.size!=(8192,4096) or t[259]!=1 or t[258]!=(16,) or t[278]!=1:
        raise SystemExit('Unexpected TIFF layout')
    if t[33922]!=(0.,0.,0.,-19009777.2544,9504888.6272,0.) or t[33550]!=(4641.0589,4641.0589,0.):
        raise SystemExit('Unexpected georeferencing')
    # Decode uncompressed strips directly: Pillow cannot load this TIFF's planar tag.
    rows=[np.frombuffer(raw,dtype='<i2',count=8192,offset=o) for o in t[273]]
    source=np.stack(rows)
if source.shape!=(4096,8192): raise SystemExit('Unexpected source rows')
valid=source!=-32768
if np.any((source[valid]<-2951)|(source[valid]>11687)): raise SystemExit('Unexpected elevation range')
# Average 2x2 source cells, retaining missing coverage if ANY contributor is missing.
blocks=source.astype(np.int32).reshape(2048,2,4096,2)
heights=np.rint(blocks.mean(axis=(1,3))).astype('<i2')
heights[~valid.reshape(2048,2,4096,2).all(axis=(1,3))]=-32768
payload=heights.tobytes(); checksum=14695981039346656037
for b in payload: checksum=((checksum^b)*1099511628211)&((1<<64)-1)
with a.output.open('xb') as f:
    f.write(f'VENUS_DEM 1 4096 2048 {checksum:016x}\n'.encode()); f.write(payload)
print(f'{a.output}: {len(payload)} bytes, missing cells {(heights==-32768).sum()}, FNV1a {checksum:016x}')
