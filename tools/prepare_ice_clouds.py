#!/usr/bin/env python3
"""Reproject the pinned NASA VTAD ice giant visualization textures; offline only."""
import argparse
import hashlib
import json
import math
import pathlib
import struct
import subprocess
import tempfile

from prepare_outer_worlds import SOURCES
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
    parser.add_argument('body', choices=('uranus','neptune'))
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('cloud_output', type=pathlib.Path)
    args = parser.parse_args()
    if args.cloud_output.exists():
        raise SystemExit('Output exists; choose new filenames')
    raw = args.source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != SOURCES[args.body]:
        raise SystemExit('Unexpected NASA ice giant GLB SHA256')
    length, kind = struct.unpack_from('<II', raw, 12)
    assert kind == 0x4e4f534a
    doc = json.loads(raw[20:20+length])
    binary = raw[28+length:]

    def buffer(index):
        v = doc['bufferViews'][index]
        return binary[v.get('byteOffset', 0):v.get('byteOffset', 0)+v['byteLength']]

    def accessor(index, fmt):
        return list(struct.iter_unpack(fmt, buffer(doc['accessors'][index]['bufferView'])))

    primitive = doc['meshes'][0]['primitives'][0]
    material=doc['materials'][primitive['material']]
    texture=material['pbrMetallicRoughness']['baseColorTexture']['index']
    image=doc['images'][doc['textures'][texture]['source']]
    with tempfile.TemporaryDirectory(prefix='ice-cloud-') as temp:
        p=pathlib.Path(temp)/'cloud.png'
        p.write_bytes(buffer(image['bufferView']))
        image_width,image_height=map(int,subprocess.check_output(['magick','identify','-format','%w %h',str(p)]).split())
        atlas=subprocess.check_output(['magick',str(p),'-alpha','off','-depth','8','rgb:-'])
    primitive = doc['meshes'][0]['primitives'][0]
    p = accessor(primitive['attributes']['POSITION'], '<fff')
    uv = accessor(primitive['attributes']['TEXCOORD_0'], '<ff')
    ids = [i[0] for i in accessor(primitive['indices'], '<H')]
    # The source is a spherified-cube UV atlas, NOT an equirectangular map.
    # Ray-project the authored mesh and its UVs onto a new spherical grid.
    q = 1.
    # Apply the pinned models' +90 degree X rotation and remove flattening:
    # the output atlas uses parametric latitude on a unit sphere.
    node,=doc['nodes']
    assert all(abs(a-b)<1e-6 for a,b in zip(node['rotation'],(.70710678,0,0,.70710678)))
    lo=[min(v[k] for v in p) for k in range(3)]
    hi=[max(v[k] for v in p) for k in range(3)]
    centre=[(a+b)/2 for a,b in zip(lo,hi)]
    half=[(b-a)/2 for a,b in zip(lo,hi)]
    p=[((x-centre[0])/half[0],-(z-centre[2])/half[2],(y-centre[1])/half[1]) for x,y,z in p]
    # Exporter pole residuals have arbitrary longitude. Snap only the true
    # pole vertices (the next latitude row is much farther than this tolerance).
    p=[(0.,1. if v[1]>0 else -1.,0.) if math.hypot(v[0],v[2])<1e-3 else v for v in p]
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
                sx=min(image_width-1,max(0,int(u*image_width)))
                sy=min(image_height-1,max(0,int(v*image_height)))
                source=(sy*image_width+sx)*3; target=(y*WIDTH+x)*3
                payload[target:target+3]=atlas[source:source+3]
                covered[y*WIDTH+x]=1
    # A few duplicated cube-edge vertices differ by exporter roundoff. Repair
    # only narrow pixel slivers from mapped neighbours within three pixels; reject any
    # real coverage gap rather than inventing unobserved cloud structures.
    missing=[i for i,v in enumerate(covered) if not v]
    if len(missing)>128:
        raise SystemExit(f'Projection left {len(missing)} unmapped pixels')
    for i in missing:
        y,x=divmod(i,WIDTH)
        neighbours=[yy*WIDTH+(xx%WIDTH) for yy in range(max(0,y-3),min(HEIGHT,y+4)) for xx in range(x-3,x+4) if covered[yy*WIDTH+(xx%WIDTH)]]
        if not neighbours: raise SystemExit('Non-isolated source mesh coverage gap')
        for c in range(3): payload[i*3+c]=round(sum(payload[j*3+c] for j in neighbours)/len(neighbours))
    print('Interpolated bounded polar seam pixels:',len(missing))
    write_asset(args.cloud_output,f'{args.body.upper()}_CLOUD_RGB 1 {WIDTH} {HEIGHT}',payload)
    print('Prepared checked cloud map:',args.cloud_output)



if __name__ == '__main__':
    main()
