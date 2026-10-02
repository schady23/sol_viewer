#pragma once
namespace satellites {
inline constexpr const char* fragment=R"GLSL(#version 330 core
uniform vec3 lightDirection,eyeLocal;
uniform int lighting,titanHaze,icePlanet,ringMode;
uniform float tilt,axisRatio,phase;
uniform float ringOpacity,spin;
uniform sampler2D cloudMap;
uniform sampler3D cloudNoise;
in vec3 point,modelPoint;
flat in vec3 surfaceColor;
out vec4 color;
const float PI=3.14159265359;
vec3 sphere(float lon,float lat) { return vec3(cos(lat)*cos(lon),sin(lat),-cos(lat)*sin(lon)); }
vec3 advect(vec3 p,float angle) {
    float c=cos(angle),s=sin(angle); return vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);
}
float cloudFbm(vec3 p) {
    return textureLod(cloudNoise,p/64.,0.).r*.57
         + textureLod(cloudNoise,(p*2.03+19.)/64.,0.).g*.28
         + textureLod(cloudNoise,(p*4.11+37.)/64.,0.).b*.15;
}
vec3 archive(float lon,float lat) {
    vec2 uv=vec2(lon/(2.*PI),.5-lat/PI);
    vec2 dx=dFdx(uv),dy=dFdy(uv); dx.x-=round(dx.x); dy.x-=round(dy.x);
    return textureGrad(cloudMap,uv,dx,dy).rgb;
}
void main() {
    vec3 facet=normalize(cross(dFdx(point),dFdy(point)));
    if(dot(facet,point)<0.) facet=-facet;
    float band=max(0.,dot(facet,lightDirection))*4.;
    float shade=lighting==1?.48+.13*(floor(band)+smoothstep(.4,.6,fract(band))):1.;
    vec3 c=surfaceColor*shade;
    if(icePlanet>0&&ringMode==0) {
        // Smooth oblate normal + a faint facet response, as on Jupiter.
        vec3 normal=normalize(modelPoint/vec3(1.,axisRatio*axisRatio,1.));
        float ct=cos(tilt),st=sin(tilt);
        normal=vec3(ct*normal.x-st*normal.y,st*normal.x+ct*normal.y,normal.z);
        float diffuse=lighting==1?.30+.70*max(0.,dot(normal,lightDirection)):1.;
        vec3 n=normalize(modelPoint/vec3(1.,axisRatio,1.));
        float lat=asin(clamp(n.y,-1.,1.));
        float lon=atan(-n.z,n.x)-spin;
        vec3 p=sphere(lon,lat);
        float latitude=abs(lat);
        float mid=smoothstep(.16,.55,latitude)*(1.-smoothstep(.85,1.25,latitude));
        float polar=smoothstep(.90,1.40,latitude);
        // Blend latitude-dependent flows, each with integer loop harmonics.
        // Unlike a noninteger phase shift, both value and velocity close at 2PI.
        vec3 equatorial=advect(p,phase);
        vec3 temperate=advect(p,-2.*phase);
        vec3 cap=advect(p,3.*phase);
        float broad=mix(cloudFbm(equatorial*10.),cloudFbm(temperate*15.),mid);
        broad=mix(broad,cloudFbm(cap*12.),polar);
        float fine=mix(cloudFbm(equatorial*45.+broad*3.),cloudFbm(temperate*65.+broad*4.),mid);
        float threads=cloudFbm(advect(p,-3.*phase)*130.+vec3(broad*5.,fine*5.,broad*3.));
        float envelope=pow(max(0.,cos(lat)),2.);
        float fold=(broad-.5)*.045*envelope;
        float cloudLat=clamp(lat+fold,-PI*.5,PI*.5);
        // Reprojected source imagery now participates in the moving bands.
        // All offsets make whole revolutions over the ten-rotation loop.
        vec3 source=mix(archive(lon-phase,cloudLat),archive(lon+2.*phase,cloudLat),mid*.65);
        vec3 base=icePlanet==1?mix(source,vec3(.49,.73,.72),.22):mix(source,vec3(.33,.58,.70),.30);
        float amplitude=icePlanet==1?.065:.13;
        float ribbon=sin(lat*85.+(broad-.5)*9.+(fine-.5)*3.);
        float filament=smoothstep(.54,.73,fine*.66+threads*.34);
        base+=envelope*amplitude*((broad-.5)*.7+(fine-.5)*.35+ribbon*.075);
        // Wispy methane cloud streaks, restrained on Uranus; all authored.
        base=mix(base,icePlanet==1?vec3(.70,.82,.79):vec3(.72,.82,.90),filament*envelope*(icePlanet==1?.055:.16));
        if(icePlanet==1) base+=vec3(.04,.055,.045)*smoothstep(.45,.90,abs(n.y));
        float limb=pow(1.-max(0.,dot(normal,normalize(eyeLocal-point))),4.);
        c=mix(base,vec3(.60,.78,.80),limb*.12)*diffuse;
        c*=mix(1.,shade,.025);
    }
    if(ringMode==1) {
        // Physical annulus widths; visibility/brightness is illustrative.
        color=vec4(vec3(.49,.53,.51),ringOpacity); return;
    }
    if(titanHaze==1) {
        // An opaque visual haze hides the archival surface map. Smooth radial
        // lighting keeps it atmospheric; it is not measured weather or a DEM.
        vec3 n=normalize(point);
        float light=lighting==1?.35+.65*max(0.,dot(n,lightDirection)):1.;
        float bands=.015*sin(n.y*24.)+.007*sin(n.y*67.);
        vec3 haze=mix(vec3(.66,.38,.13),vec3(.83,.59,.25),smoothstep(-.6,.8,n.y));
        float limb=pow(1.-max(0.,dot(n,normalize(eyeLocal-point))),3.);
        haze=mix(haze,vec3(.96,.73,.39),limb*.25);
        c=(haze+bands)*light;
    }
    color=vec4(c,1.);
})GLSL";
}
