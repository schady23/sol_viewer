#pragma once
namespace satellites {
inline constexpr const char* outerVertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 direction;
layout(location=1) in vec4 faceColor;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens,spin,tilt,axisRatio;
uniform int icePlanet,ringMode;
out vec3 point,modelPoint;
flat out vec3 surfaceColor;
void main() {
    surfaceColor=faceColor.rgb;
    float c=cos(spin),s=sin(spin);
    vec3 p=direction;
    // Dense cloud vertices follow an analytic 1-bar ellipsoid;
    // source appearance is sampled separately in the atmosphere shader.
    if(icePlanet>0&&ringMode==0) p=normalize(p/vec3(1.,axisRatio,1.))*vec3(1.,axisRatio,1.);
    modelPoint=vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);
    c=cos(tilt); s=sin(tilt);
    point=vec3(c*modelPoint.x-s*modelPoint.y,s*modelPoint.x+c*modelPoint.y,modelPoint.z);
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
})GLSL";
}
