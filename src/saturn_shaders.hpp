#pragma once
namespace saturn {
inline constexpr const char* globeVertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 direction;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens;
out vec3 point;
void main() {
    point=direction*vec3(1,54364./60268.,1);
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
})GLSL";
// Shared radial source mapping and authored opacity intervals for both shadows
// and ring rendering. Radii are the NASA ring fact sheet's representative edges.
inline constexpr const char* ringFunctions=R"GLSL(
const float Q=54364./60268.;
vec4 ringSample(float r) {
    float km=r*60268.;
    if(km<74658.||km>139876.) return vec4(0);
    float u=(r-1.2323)/(2.3309-1.2323);
    vec4 c=texture(ringMap,vec2(u,.5));
    // The strip is a visualization texture, not a measured optical-depth map.
    // Keep the major division and narrow gap at their physical radii.
    float opacity=km<91975.?.30:km<117507.?.96:km<122340.?.055:km<136780.?.76:0.;
    if(abs(km-133410.)<160.) opacity=.01;
    if(abs(km-139826.)<50.) { opacity=.30; c.rgb=vec3(.78,.72,.61); c.a=1.; }
    c.a*=opacity;
    return c;
}
)GLSL";
inline constexpr const char* globeFragment=R"GLSL(#version 330 core
uniform sampler2D cloudMap,windProfile,ringMap;
uniform sampler3D noiseVolume;
uniform float spin,phase;
uniform vec3 lightDirection,eyeLocal;
in vec3 point;
out vec4 color;
const float PI=3.14159265359;
vec4 ringSample(float r);
vec4 noise4(vec3 p) { return textureLod(noiseVolume,p/64.,0.); }
float fbm(vec3 p) { return noise4(p).r*.53+noise4(p*2.03+19.).g*.27+noise4(p*4.11+37.).b*.13+noise4(p*8.17+7.).a*.07; }
void main() {
    const float Q=54364./60268.;
    vec3 normal=normalize(point/vec3(1,Q*Q,1));
    float lat=asin(clamp(normal.y,-1.,1.));
    float lon=atan(-point.z,point.x)-spin;
    vec2 uv=vec2(lon/(2.*PI),.5-lat/PI);
    vec2 dx=dFdx(uv),dy=dFdy(uv); dx.x-=round(dx.x); dy.x-=round(dy.x);
    float jet=texture(windProfile,vec2(.5,uv.y)).r;
    float polar=1.-smoothstep(1.2,1.5,abs(lat));
    float signJet=mix(-1.,1.,smoothstep(-10.,20.,jet));
    // Integer harmonics and spherical fields close the 10-rotation loop.
    vec3 s=vec3(cos(lon)*cos(lat),sin(lat),sin(lon)*cos(lat));
    vec3 p=s*vec3(42.,125.,42.);
    vec3 flow=noise4(vec3(cos(lon*6.-phase*signJet)*5.,lat*35.,sin(lon*6.-phase*signJet)*5.)).rgb-.5;
    vec2 offset=vec2(.0035*sin(phase)*jet/330.+flow.x*.003,flow.y*.0013)*polar;
    vec3 c=textureGrad(cloudMap,uv+offset,dx,dy).rgb;
    vec3 warp=noise4(p*.25+11.).rgb-.5;
    float broad=fbm(p+warp*4.+flow*3.);
    float fine=fbm(p*4.+vec3(4.*sin(phase),0,4.*cos(phase)));
    float ribbonPhase=lat*520.+broad*16.+fine*3.;
    float aa=clamp(fwidth(ribbonPhase),0.,1.);
    float ribbons=smoothstep(.74-aa*.2,.98+aa*.2,.5+.5*sin(ribbonPhase));
    // Saturn's hazier, softer gold palette differs from Jupiter's rust belts.
    c=clamp((c-.10)*1.08,0.,1.);
    c+=vec3((fine-.5)*.035+(broad-.5)*.035);
    c=mix(c,vec3(.94,.87,.69),ribbons*.045*polar);
    if(lat>1.20) {
        float angle=atan(normal.z,normal.x)-spin;
        float rho=length(normal.xz);
        float hexRadius=rho*cos(mod(angle+PI/6.,PI/3.)-PI/6.)+(broad-.5)*.007+(fine-.5)*.003;
        float interior=1.-smoothstep(.165,.195,hexRadius);
        vec3 cap=mix(c,vec3(.50,.54,.43),interior*.22);
        cap+=(fine-.5)*.035;
        float boundary=1.-smoothstep(.001,.014,abs(hexRadius-.18));
        cap=mix(cap,vec3(.37,.43,.35),boundary*.16);
        float eye=1.-smoothstep(.008,.052,rho);
        cap=mix(cap,vec3(.21,.28,.27),eye*.46);
        c=mix(c,cap,smoothstep(1.20,1.37,lat)*.78);
    }
    vec3 face=normalize(cross(dFdx(point),dFdy(point))); if(dot(face,point)<0.) face=-face;
    vec3 n=normalize(mix(normal,face,.65));
    float light=.30+.70*pow(max(0.,dot(n,lightDirection)),.8);
    float shadow=0.;
    if(abs(lightDirection.y)>.0001) {
        float t=-point.y/lightDirection.y;
        if(t>0.) shadow=ringSample(length((point+t*lightDirection).xz)).a;
    }
    light*=1.-shadow*.72;
    float limb=pow(1.-max(0.,dot(normal,normalize(eyeLocal-point))),4.);
    c=mix(c*light,vec3(.74,.66,.48)*light,limb*.17);
    color=vec4(clamp(c,0.,1.),1);
}
)GLSL";
inline constexpr const char* ringVertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 position;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens;
out vec3 point;
void main() {
    point=position;
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
})GLSL";
inline constexpr const char* ringFragment=R"GLSL(#version 330 core
uniform sampler2D ringMap;
uniform vec3 lightDirection,eyeLocal;
in vec3 point;
out vec4 color;
vec4 ringSample(float r);
void main() {
    const float Q=54364./60268.;
    vec4 source=ringSample(length(point.xz));
    if(source.a<.003) discard;
    vec3 p=point/vec3(1,Q,1),l=lightDirection/vec3(1,Q,1);
    float a=dot(l,l),b=dot(p,l),c=dot(p,p)-1.;
    float disc=b*b-a*c;
    float shadow=0.;
    if(disc>0.&&(-b-sqrt(disc))/a>0.) shadow=smoothstep(0.,.015,disc);
    float light=.40+.60*sqrt(abs(lightDirection.y));
    if(eyeLocal.y*lightDirection.y<0.) light*=.66;
    // Source gaps, texture alpha, two-sided lighting and analytic oblate-planet
    // shadow. Depth writes are disabled so the transparent annulus is composited
    // against the opaque planet and other bodies without an invisible occluder.
    color=vec4(source.rgb*light*(1.-shadow*.86),source.a);
}
)GLSL";
}
