#pragma once
namespace jupiter {
inline constexpr const char* globeVertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 direction;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens;
out vec3 point;
void main() {
    point=direction*vec3(1,66854./71492.,1);
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
}
)GLSL";
inline constexpr const char* globeFragment=R"GLSL(#version 330 core
uniform sampler2D cloudMap,windProfile;
uniform sampler3D noiseVolume;
uniform float spin,phase;
uniform vec3 lightDirection,eyeLocal;
in vec3 point;
out vec4 color;
const float PI=3.14159265359;
vec3 sphere(float lon,float lat) { return vec3(cos(lon)*cos(lat),sin(lat),sin(lon)*cos(lat)); }
mat2 rotate(float a) { float c=cos(a),s=sin(a); return mat2(c,s,-s,c); }
vec4 noise4(vec3 p) { return textureLod(noiseVolume,p/64.,0.); }
float fbm(vec3 p) {
    return noise4(p).r*.53+noise4(p*2.03+19.).g*.27+noise4(p*4.11+37.).b*.13+noise4(p*8.17+7.).a*.07;
}
// Integer harmonics make positions and velocities continuous at loop closure.
// Spherical coordinates keep these fields continuous across longitude/poles.
vec3 flow(float lon,float lat,float t) {
    vec3 p=vec3(cos(7.*lon-t)*4.,lat*30.,sin(7.*lon-t)*4.);
    vec3 a=noise4(p*.65+13.).rgb-.5;
    vec3 b=noise4(p+a*4.+vec3(0,2.*sin(t),0)).rgb-.5;
    return b;
}
vec3 archive(vec2 uv,vec2 dx,vec2 dy) { return textureGrad(cloudMap,uv,dx,dy).rgb; }
vec3 stormMap(vec2 q,float angle,vec2 dx,vec2 dy) {
    q=rotate(-angle)*q;
    vec2 uv=vec2(233.2/360.,.5+22.4/180.)+q*vec2(7.2/360.,-4.9/180.);
    return archive(uv,dx,dy);
}
void main() {
    const float Q=66854./71492.;
    vec3 smoothNormal=normalize(point/vec3(1,Q*Q,1));
    float lat=asin(clamp(smoothNormal.y,-1.,1.));
    float lon=atan(-point.z,point.x)-spin;
    vec2 uv=vec2(lon/(2.*PI),.5-lat/PI);
    vec2 dx=dFdx(uv),dy=dFdy(uv); dx.x-=round(dx.x); dy.x-=round(dy.x);
    float polar=1.-smoothstep(1.18,1.5,abs(lat));
    float jet=texture(windProfile,vec2(.5,uv.y)).r;
    float east=smoothstep(-18.,25.,jet);
    vec3 current=mix(flow(lon,lat,-phase),flow(lon,lat,phase),east)*polar;
    float spotX=mod(lon*180./PI-233.2+180.,360.)-180.;
    vec2 spot=vec2(spotX/7.2,(lat*180./PI+22.4)/4.9);
    float r=length(spot),protect=1.-smoothstep(1.,1.9,r);
    // Small, periodic offsets bend existing fronts instead of replacing their
    // real structure with random blotches. The source is sampled afresh each frame.
    vec2 offset=vec2(current.x*.010,current.y*.0035);
    offset.x+=.0045*sin(phase)*mix(-.45,1.,east)*polar;
    offset*=1.-protect;
    vec3 c=archive(uv+offset,dx,dy);
    float stormMask=1.-smoothstep(.86,1.18,r);
    if(r<1.2) {
        // Differential circulation within the observed oval: two complete-turn
        // phases blend radially, retaining exact loop closure without a pinwheel.
        vec3 slow=stormMap(spot,phase,dx,dy),fast=stormMap(spot,2.*phase,dx,dy);
        c=mix(c,mix(slow,fast,smoothstep(.25,1.,r)*.35),stormMask);
    }
    // Juno-inspired nested filaments: domain-warped fields stretched along jets,
    // evaluated independently of the mesh so close views do not become a mosaic.
    vec3 s=sphere(lon,lat),p=s*vec3(48.,95.,48.);
    vec3 warp=noise4(p*.22+11.).rgb-.5;
    p+=warp*5.+vec3(current.x*4.,current.y*2.,current.z*4.);
    float broad=fbm(p);
    float fine=fbm(p*3.7+vec3(9.*sin(phase),0,9.*cos(phase)));
    float ribbonPhase=lat*480.+broad*24.+fine*3.;
    float ribbon=.5+.5*sin(ribbonPhase);
    float aa=clamp(fwidth(ribbonPhase),0.,1.);
    float filaments=smoothstep(.72-aa*.25,.96+aa*.25,ribbon);
    float luminosity=dot(c,vec3(.299,.587,.114));
    float cool=smoothstep(.015,.10,c.b-c.r);
    // Ivory ammonia-like clouds, ochre/rust belts, and restrained blue-gray gaps.
    // These grades follow enhanced Juno images, not calibrated natural color.
    c=pow(clamp((c-.10)*1.23,0.,1.),vec3(.86));
    c=mix(c,vec3(.28,.39,.47),cool*.20);
    float micro=(fine-.5)*.075+(broad-.5)*.055;
    c+=micro*(1.-stormMask*.55);
    c=mix(c,vec3(.88,.89,.85),filaments*.10*polar*(1.-stormMask));
    // Fine folded tracers within the storm co-rotate with its archival cloud
    // structure. Cartesian sampling avoids an angular seam at the vortex centre.
    vec2 tracer=rotate(-phase+1.4*(1.-r*r))*spot;
    vec3 sp=vec3(tracer*18.,r*7.);
    vec3 sw=noise4(sp*.34+23.).rgb-.5;
    float stormFine=fbm(sp+sw*5.);
    float stormRibbon=r*105.+stormFine*23.;
    float stormAA=clamp(fwidth(stormRibbon),0.,1.);
    float streak=smoothstep(.75-stormAA*.2,.97+stormAA*.2,.5+.5*sin(stormRibbon));
    float inner=1.-smoothstep(.65,1.03,r);
    c+=(stormFine-.5)*.065*inner;
    c=mix(c,vec3(.78,.51,.31),streak*.13*inner);
    float high=smoothstep(1.27,1.49,abs(lat));
    if(high>0.) {
        // Hubble does not cover the poles. A clearly disclosed illustrative
        // cloud deck continues the Juno-inspired folded filaments there.
        vec3 pp=s*27.; pp.xz=rotate(phase)*pp.xz;
        vec3 pw=noise4(pp*.6).rgb-.5;
        float cloud=fbm(pp+pw*5.);
        float threads=.5+.5*sin(cloud*37.+fbm(pp*3.)*9.);
        vec3 cap=mix(vec3(.30,.36,.40),vec3(.71,.72,.65),smoothstep(.25,.8,cloud));
        cap=mix(cap,vec3(.78,.81,.76),smoothstep(.7,.98,threads)*.14);
        c=mix(c,cap,high*.8);
    }
    // Subtle cloud relief, independent of geographic picking. Flat geometric
    // normals retain the atlas's facets without flattening all color per face.
    vec3 face=normalize(cross(dFdx(point),dFdy(point))); if(dot(face,point)<0.) face=-face;
    vec3 n=normalize(mix(smoothNormal,face,.72));
    float cloudHeight=luminosity*.65+broad*.20+fine*.15;
    vec3 sx=dFdx(point),sy=dFdy(point),rx=cross(sy,n),ry=cross(n,sx);
    float det=dot(sx,rx);
    vec3 gradient=(dFdx(cloudHeight)*rx+dFdy(cloudHeight)*ry)/max(abs(det),1e-10);
    n=normalize(n-clamp(gradient,vec3(-1000),vec3(1000))*.00008);
    float light=.29+.71*pow(max(0.,dot(n,lightDirection)),.8);
    float limb=pow(1.-max(0.,dot(smoothNormal,normalize(eyeLocal-point))),4.);
    c=mix(c*light,vec3(.37,.46,.53)*(.6+.4*light),limb*.24);
    color=vec4(clamp(c,0.,1.),1);
}
)GLSL";
}
