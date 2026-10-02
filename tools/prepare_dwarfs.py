#!/usr/bin/env python3
"""Prepare pinned NASA Ceres/Pluto meshes or untextured dwarf reference shapes."""
import argparse, hashlib, math, pathlib, struct
from prepare_satellites import model, checksum
SOURCES = {'ceres': 'c43ec43aa8fb90aa8cf30660c2f2853cfac408d38149d1dc470490f4f4aa9899', 'pluto': '9915e6874ff7e7cd64faf8ef016ea16de6c8058417c42909efa099a88f2dd25b'}

def reference(body):
    # Haumea: illustrative triaxial approximation to Ortiz et al. 2017;
    # semiaxes 1161, 852, 513 km. Unknown terrain is intentionally absent.
    axes=(1.,513/1161,852/1161) if body=='haumea' else (1.,1.,1.)
    tint=(201,203,201) if body=='haumea' else (180,126,90) if body=='makemake' else (210,212,210)
    payload=bytearray()
    def point(lat,lon):
        return (math.cos(lat)*math.cos(lon)*axes[0],math.sin(lat)*axes[1],-math.cos(lat)*math.sin(lon)*axes[2])
    for j in range(96):
        for i in range(192):
            a,b=point(-math.pi/2+j*math.pi/96,i*math.tau/192),point(-math.pi/2+j*math.pi/96,(i+1)*math.tau/192)
            c,d=point(-math.pi/2+(j+1)*math.pi/96,i*math.tau/192),point(-math.pi/2+(j+1)*math.pi/96,(i+1)*math.tau/192)
            for tri in ([a,b,c] if j>0 else [],[b,d,c] if j<95 else []):
                for p in tri: payload.extend(struct.pack('<fffBBBB',*p,*tint,255))
    return payload

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('body',choices=['ceres','pluto','haumea','makemake','eris']); p.add_argument('output',type=pathlib.Path); p.add_argument('--source',type=pathlib.Path)
    a=p.parse_args()
    if a.body in SOURCES:
        raw=a.source.read_bytes()
        if hashlib.sha256(raw).hexdigest()!=SOURCES[a.body]: raise SystemExit('Unexpected NASA GLB SHA256')
        payload=model(raw,clean_poles=True)
    else: payload=reference(a.body)
    with a.output.open('xb') as f:
        f.write(f'{a.body.upper()}_MESH 1 {len(payload)//16} {checksum(payload):016x}\n'.encode()); f.write(payload)
    print(a.output,len(payload)//48,'triangles')
if __name__=='__main__': main()
