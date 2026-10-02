#!/usr/bin/env python3
"""Convert the checked NOAA ETOPO2022 nine-arc-minute OPeNDAP subset; stdlib only.
See docs/EARTH.md for exact requests and metadata. No image interpretation is used.
"""
import argparse, array, hashlib, json, math, pathlib, re, struct, sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('directory',type=pathlib.Path)
p.add_argument('output',type=pathlib.Path)
a=p.parse_args()
if a.output.exists(): raise SystemExit('Output exists; choose a new filename')
metadata=pathlib.Path('docs/earth/etopo2022.das').read_text()
for required in ['Float32 _FillValue -99999.0', 'String units "meters"', 'String positive "up"',
                 'String vert_crs_name "EGM2008"', 'Int32 node_offset 1']:
    if required not in metadata: raise SystemExit('Unexpected NOAA metadata: '+required)
expected=json.loads(pathlib.Path('docs/earth/etopo2022-subset-sha256.json').read_text())
rows=[]; sources={}
for band in range(10):
    path=a.directory/f'etopo2022-band{band:02d}.dods'
    raw=path.read_bytes(); sources[path.name]=hashlib.sha256(raw).hexdigest()
    if sources[path.name]!=expected[path.name]: raise SystemExit('NOAA subset hash mismatch: '+str(path))
    header,body=raw.split(b'\nData:\n',1)
    if not re.search(rb'Float32 z\[lat = 120\]\[lon = 2400\]',header):
        raise SystemExit('Unexpected DDS dimensions/type in '+str(path))
    offset=0
    def values(count,code):
        global offset
        if len(body)<offset+8 or struct.unpack_from('>II',body,offset)!=(count,count):
            raise SystemExit('Missing/invalid XDR array counts in '+str(path))
        offset+=8; out=array.array(code)
        size=count*out.itemsize
        if len(body)<offset+size: raise SystemExit('Truncated NOAA response: '+str(path))
        out.frombytes(body[offset:offset+size]); offset+=size
        if sys.byteorder=='little': out.byteswap()
        return out
    heights=values(120*2400,'f'); lat=values(120,'d'); lon=values(2400,'d')
    if offset!=len(body): raise SystemExit('Unexpected trailing NOAA data')
    for x,v in enumerate(lon):
        if abs(v-(-179.925+x*.15))>1e-7: raise SystemExit('Longitude convention changed')
    for y,v in enumerate(lat):
        if abs(v-(-89.925+(band*120+y)*.15))>1e-7: raise SystemExit('Latitude convention changed')
        row=heights[y*2400:(y+1)*2400]
        if any(not math.isfinite(h) or h==-99999 or h<-12000 or h>9000 for h in row):
            raise SystemExit('Missing or out-of-range source elevations; refusing invented fill')
        rows.append(array.array('h',(round(h) for h in row)))
# NetCDF coordinates ascend south -> north. Runtime rows run north -> south.
payload=bytearray()
for row in reversed(rows):
    if sys.byteorder!='little': row.byteswap()
    payload.extend(row.tobytes())
h=14695981039346656037
for b in payload: h=((h^b)*1099511628211)&((1<<64)-1)
with a.output.open('xb') as f:
    f.write(f'EARTH_DEM 1 2400 1200 {h:016x}\n'.encode()); f.write(payload)
print(f'{a.output}: {len(payload)} bytes; FNV1a {h:016x}; signed metres / EGM2008')
print(json.dumps(sources,indent=2))
