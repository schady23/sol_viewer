#!/usr/bin/env python3
"""Reproject the pinned NASA VTAD Saturn visualization texture; offline only."""
import argparse
import hashlib
import json
import math
import pathlib
import struct
import subprocess
import tempfile

SOURCE_SHA256 = '8dbfde533b13e938408d8aded1d1ce48a084806233f67775371d83013dba7d38'
WIDTH, HEIGHT = 2048, 1024


def write_asset(path, header, payload):
    checksum = 14695981039346656037
    for b in payload:
        checksum = ((checksum ^ b) * 1099511628211) & ((1 << 64)-1)
    with path.open('xb') as out:
        out.write(f'{header} {checksum:016x}\n'.encode())
        out.write(payload)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('cloud_output', type=pathlib.Path)
    parser.add_argument('ring_output', type=pathlib.Path)
    args = parser.parse_args()
    if args.cloud_output.exists() or args.ring_output.exists():
        raise SystemExit('Output exists; choose new filenames')
    raw = args.source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCE_SHA256:
        raise SystemExit('Unexpected NASA Saturn GLB SHA256')
    length, kind = struct.unpack_from('<II', raw, 12)
    assert kind == 0x4e4f534a
    doc = json.loads(raw[20:20+length])
    binary = raw[28+length:]

    def buffer(index):
        v = doc['bufferViews'][index]
        return binary[v.get('byteOffset', 0):v.get('byteOffset', 0)+v['byteLength']]

    def accessor(index, fmt):
        return list(struct.iter_unpack(fmt, buffer(doc['accessors'][index]['bufferView'])))

    with tempfile.TemporaryDirectory(prefix='saturn-') as temp:
        decoded = []
        for i, image in enumerate(doc['images']):
            p = pathlib.Path(temp)/f'{i}.png'
            p.write_bytes(buffer(image['bufferView']))
            size = subprocess.check_output(['magick','identify','-format','%wx%h',str(p)],text=True)
            assert size == ('4096x3072' if i == 0 else '4096x16')
            decoded.append(subprocess.check_output(['magick',str(p),'-depth','8','rgba:-']))
    atlas, ring = decoded
    primitive = doc['meshes'][0]['primitives'][0]
    p = accessor(primitive['attributes']['POSITION'], '<fff')
    uv = accessor(primitive['attributes']['TEXCOORD_0'], '<ff')
    ids = [i[0] for i in accessor(primitive['indices'], '<H')]
    # The source is a spherified-cube UV atlas, NOT an equirectangular map.
    # Ray-project the authored mesh and its UVs onto a new spherical grid.
    q = 451.0190124511719/500
    p = [(x/500,y/(500*q),z/500) for x,y,z in p]
    payload = bytearray(WIDTH*HEIGHT*3)
    covered = bytearray(WIDTH*HEIGHT)
    longitude = [(x+.5)*2*math.pi/WIDTH for x in range(WIDTH)]
    latitude = [math.pi/2-(y+.5)*math.pi/HEIGHT for y in range(HEIGHT)]
    cos_lon, sin_lon = [math.cos(x) for x in longitude], [math.sin(x) for x in longitude]
    # Destination rows are planetographic latitude; intersect source geometry
    # using the equivalent parametric direction on the scaled ellipsoid.
    parametric = [math.atan(q*math.tan(x)) for x in latitude]
    cos_lat, sin_lat = [math.cos(x) for x in parametric], [math.sin(x) for x in parametric]
    dot = lambda a,b: sum(x*y for x,y in zip(a,b))
    sub = lambda a,b: tuple(x-y for x,y in zip(a,b))
    cross = lambda a,b: (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    for offset in range(0,len(ids),3):
        ia,ib,ic = ids[offset:offset+3]
        a,b,c = p[ia],p[ib],p[ic]
        e,f = sub(b,a),sub(c,a)
        n = cross(e,f)
        plane = dot(n,a)
        ee,ef,ff = dot(e,e),dot(e,f),dot(f,f)
        det = ee*ff-ef*ef
        if det <= 0:
            continue
        lons = [math.atan2(-v[2],v[0]) for v in (a,b,c)]
        for j in range(1,3):
            while lons[j]-lons[0]>math.pi: lons[j]-=2*math.pi
            while lons[j]-lons[0]<-math.pi: lons[j]+=2*math.pi
        # A pole vertex has no longitude; place it midway between its neighbours.
        polar = [math.hypot(v[0],v[2])<1e-5 for v in (a,b,c)]
        for j in range(3):
            if polar[j]: lons[j]=(lons[(j+1)%3]+lons[(j+2)%3])/2
        lats = [math.atan2(v[1]/q,math.hypot(v[0],v[2])) for v in (a,b,c)]
        # Great-circle edges can bulge beyond their vertex latitudes. Pad the
        # modest source facets; ray barycentrics make final coverage exact.
        y0=max(0,int((math.pi/2-max(lats))*HEIGHT/math.pi)-12)
        y1=min(HEIGHT-1,int((math.pi/2-min(lats))*HEIGHT/math.pi)+12)
        x0=int(min(lons)*WIDTH/(2*math.pi))-2
        x1=int(max(lons)*WIDTH/(2*math.pi))+2
        for y in range(y0,y1+1):
            for column in range(x0,x1+1):
                x=column%WIDTH
                ray=(cos_lat[y]*cos_lon[x],sin_lat[y],-cos_lat[y]*sin_lon[x])
                denom=dot(n,ray)
                if denom<=0: continue
                distance=plane/denom
                relative=tuple(distance*v-z for v,z in zip(ray,a))
                re,rf=dot(relative,e),dot(relative,f)
                wb,wc=(re*ff-rf*ef)/det,(rf*ee-re*ef)/det
                wa=1-wb-wc
                if min(wa,wb,wc)<-1e-6: continue
                u=wa*uv[ia][0]+wb*uv[ib][0]+wc*uv[ic][0]
                v=wa*uv[ia][1]+wb*uv[ib][1]+wc*uv[ic][1]
                sx=min(4095,max(0,int(u*4096)))
                sy=min(3071,max(0,int(v*3072)))
                source=(sy*4096+sx)*4; target=(y*WIDTH+x)*3
                payload[target:target+3]=atlas[source:source+3]
                covered[y*WIDTH+x]=1
    if not all(covered):
        raise SystemExit(f'Projection left {covered.count(0)} unmapped pixels')
    # Average the radial strip in premultiplied-alpha space to preserve gaps.
    radial = bytearray()
    for x in range(4096):
        pixels=[ring[(y*4096+x)*4:(y*4096+x)*4+4] for y in range(16)]
        alpha=sum(p[3] for p in pixels)
        radial.extend(round(sum(p[c]*p[3] for p in pixels)/alpha) if alpha else 0 for c in range(3))
        radial.append(round(alpha/16))
    write_asset(args.cloud_output,'SATURN_RGB 1 2048 1024',payload)
    write_asset(args.ring_output,'SATURN_RINGS 1 4096 1',radial)
    print('Prepared north-to-south model cloud map and RGBA radial ring strip')


if __name__ == '__main__':
    main()
