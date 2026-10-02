#!/usr/bin/env python3
"""Create an explicitly untextured mean-radius sphere, not a Proteus shape model."""
import argparse
import math
import struct
from prepare_satellites import checksum
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('output')
a=parser.parse_args()
payload=bytearray()
def point(j,i):
    lat=math.radians(-90+j*180/128)
    lon=math.radians(i*360/256)
    return struct.unpack('<fff',struct.pack('<fff',math.cos(lat)*math.cos(lon),math.sin(lat),-math.cos(lat)*math.sin(lon)))
volume=0
for j in range(128):
    for i in range(256):
        p,q,r,s=point(j,i),point(j,i+1),point(j+1,i),point(j+1,i+1)
        for tri in ([(p,q,r)] if j else [])+([(q,s,r)] if j<127 else []):
            x,y,z=tri
            cross=(y[1]*z[2]-y[2]*z[1],y[2]*z[0]-y[0]*z[2],y[0]*z[1]-y[1]*z[0])
            volume+=sum(x[k]*cross[k] for k in range(3))/6
            for p in tri: payload+=struct.pack('<fff4B',*p,74,74,74,255)
with open(a.output,'xb') as out:
    out.write(f'PROTEUS_MESH 1 {len(payload)//16} {checksum(payload):016x}\n'.encode())
    out.write(payload)
print('Bounding / equivalent radius ratio:',1/(volume*3/(4*math.pi))**(1/3))
