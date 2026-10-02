#pragma once
namespace faceted {
inline constexpr const char* vertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 direction;
layout(location=1) in vec4 faceColor;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens,spin;
out vec3 point;
flat out vec3 surfaceColor;
void main() {
    surfaceColor=faceColor.rgb;
    float c=cos(spin),s=sin(spin);
    point=vec3(c*direction.x+s*direction.z,direction.y,-s*direction.x+c*direction.z);
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
})GLSL";
inline constexpr const char* fragment=R"GLSL(#version 330 core
uniform vec3 lightDirection;
uniform int lighting;
in vec3 point;
flat in vec3 surfaceColor;
out vec4 color;
void main() {
    vec3 facet=normalize(cross(dFdx(point),dFdy(point)));
    if(dot(facet,point)<0.) facet=-facet;
    // Match Mars/Mercury's stepped diffuse response, softening only the band
    // boundaries to avoid flicker as a face crosses a quantization threshold.
    float band=max(0.,dot(facet,lightDirection))*4.;
    float shade=lighting==1?.48+.13*(floor(band)+smoothstep(.4,.6,fract(band))):1.;
    color=vec4(surfaceColor*shade,1.);
})GLSL";
}
