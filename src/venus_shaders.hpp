#pragma once
namespace venus {
inline constexpr const char* vertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 direction;
layout(location=1) in float meters;
layout(location=2) in float faceMeters;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens,shell,exaggeration;
uniform int cloudPass;
out vec3 point,unitDirection;
flat out float surfaceHeight;
void main() {
    unitDirection=direction; surfaceHeight=faceMeters;
    point=direction*(cloudPass==1?shell:1.+meters*exaggeration/6051000.);
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
})GLSL";
inline constexpr const char* fragment=R"GLSL(#version 330 core
uniform sampler3D noiseVolume;
uniform sampler2D surfaceMap;
uniform vec3 lightDirection,eyeLocal,loopPhase;
uniform int cloudPass;
in vec3 point,unitDirection;
flat in float surfaceHeight;
out vec4 color;
float noise(vec3 p) { return texture(noiseVolume,p/16.+vec3(.5/128.)).r; }
// Only broad scales: visible-light Venus is a low-contrast, nearly featureless
// cloud deck. The sharply folded UV/IR structures do not belong in this view.
float softNoise(vec3 p) { return .8*noise(p)+.2*noise(p*1.93+19.); }
mat2 rotate(float a) { float c=cos(a),s=sin(a); return mat2(c,-s,s,c); }
vec3 bend(vec3 p) {
    return vec3(noise(p+3.),noise(p+19.),noise(p+37.))-.5;
}
vec3 inferredSurface(vec3 n) {
    // JPL's Magellan/Venera/Pioneer radar composite, with simulated hues.
    // Color depicts radar structure, not elevation or measured optical albedo.
    vec2 uv=vec2(atan(-n.z,n.x)/6.28318530718+.5,.5-asin(clamp(n.y,-1.,1.))/3.14159265359);
    vec2 dx=dFdx(uv),dy=dFdy(uv);
    dx.x-=round(dx.x); dy.x-=round(dy.x);
    vec3 c=textureGrad(surfaceMap,uv,dx,dy).rgb;
    // Slightly restrain the presentation composite's orange saturation.
    float value=dot(c,vec3(.299,.587,.114));
    return mix(vec3(value)*vec3(1.1,.97,.80),c,.72);
}
void main() {
    vec3 n=normalize(unitDirection);
    if(cloudPass==1) {
        // Sample on the sphere, never on a longitude grid: both poles and the
        // meridian stay continuous. One retrograde turn closes in 60 seconds.
        vec3 p=vec3(loopPhase.x*n.x+loopPhase.y*n.z,n.y,-loopPhase.y*n.x+loopPhase.x*n.z);
        vec2 twice=vec2(loopPhase.x*loopPhase.x-loopPhase.y*loopPhase.y,2.*loopPhase.x*loopPhase.y);
        // Broad, gently changing gradients retain motion beneath the haze.
        // Integer harmonics join both position and velocity at the loop wrap.
        float shear=.6*p.y+.12*loopPhase.y*(1.-p.y*p.y);
        p.xz=rotate(shear)*p.xz;
        vec3 drift=vec3(loopPhase.y,.35*twice.y,loopPhase.x);
        vec3 warp=bend(p*1.8+drift*.3);
        float broad=softNoise(p*vec3(1.7,3.2,1.7)+warp*.55+drift*.4);
        float veil=softNoise(p*vec3(3.2,5.,3.2)+warp*.7-drift*.25);
        // Warm ivory with gently emphasized broad gradients for readability.
        // No fine tracers, bump normals, specular term, or bright rim highlight.
        float tone=clamp(.5+(broad-.5)*1.35+(veil-.5)*.45,0.,1.);
        vec3 base=mix(vec3(.82,.75,.625),vec3(1.,.96,.85),tone);
        float viewCos=max(0.,dot(n,normalize(eyeLocal-point)));
        // More haze along grazing paths gently washes out the remaining cloud
        // contrast. Apply illumination afterward so haze cannot glow in shadow.
        float haze=1.-exp(-.28/max(viewCos,.08));
        base=mix(base,vec3(.95,.903,.793),haze*.55);
        // A broad diffuse response approximates a scattering cloud layer. This
        // remains atlas lighting, not calibrated atmospheric radiative transfer.
        float sun=dot(n,lightDirection);
        float diffuse=smoothstep(-.2,1.,sun);
        float light=.58+.42*diffuse;
        color=vec4(base*light,1.);
    } else {
        vec3 face=normalize(cross(dFdx(point),dFdy(point))); if(dot(face,n)<0.) face=-face;
        vec3 base=inferredSurface(n);
        if(surfaceHeight< -30000.) base=vec3(.31,.32,.34);
        color=vec4(base*(.35+.65*max(0.,dot(face,lightDirection))),1.);
    }
})GLSL";
}
