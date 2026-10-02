#pragma once

namespace mars {
// Sample the unit sphere in 3D: no longitude UV seam or polar texture singularity.
inline constexpr const char* sphericalNoise=R"GLSL(
float hash3(vec3 p) {
    p=fract(p*0.1031);
    p+=dot(p,p.yzx+33.33);
    return fract((p.x+p.y)*p.z);
}
float noise3(vec3 p) {
    vec3 i=floor(p),f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),
                   mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),
                   mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1,1,1)),f.x),f.y),f.z);
}
)GLSL";
inline constexpr const char* weatherVertex=R"GLSL(#version 330 core
void main() {
    vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);
    gl_Position=vec4(p*2.0-1.0,0,1);
}
)GLSL";
inline constexpr const char* weatherFragment=R"GLSL(
uniform vec3 eye,backward,rightward,upward,projectionParameters,canvas;
uniform vec3 stormCentre,lightDirection;
uniform float shellRadius,surfaceMinimum,phase,kind,strength,stormRadius;
out vec4 color;
void main() {
    vec2 uv=gl_FragCoord.xy/canvas.xy*2.0-1.0;
    vec3 ray=normalize(rightward*(uv.x*canvas.x/canvas.y*projectionParameters.z)
                       +upward*(uv.y*projectionParameters.z)-backward);
    float b=dot(eye,ray),d=b*b-dot(eye,eye)+shellRadius*shellRadius;
    if(d<=0.0) discard;
    float t=-b-sqrt(d);
    if(t<=0.0) discard;
    vec3 world=eye+ray*t,n=normalize(world);
    float z=dot(eye-world,backward),near=projectionParameters.x,far=projectionParameters.y;
    gl_FragDepth=far/(far-near)-far*near/((far-near)*z);
    float edge=smoothstep(0.04,0.32,dot(n,-ray))*smoothstep(0.004,0.03,t);
    // Exaggerated relief requires raised shells. Fade within the minimum terrain
    // silhouette so that those shells never look like a detached atmospheric halo.
    float impact=sqrt(max(0.0,dot(eye,eye)-b*b));
    edge*=1.0-smoothstep(surfaceMinimum*0.94,surfaceMinimum*0.995,impact);
    float density; vec3 tint;
    if(kind<0.5) {
        // Sparse, stretched ice wisps; slowly advected and evolving.
        vec3 q=vec3(cos(phase)*n.x-sin(phase)*n.z,n.y,sin(phase)*n.x+cos(phase)*n.z);
        float broad=noise3(q*vec3(12,34,12)+vec3(3.2,7.1,1.7));
        float fine=noise3(q*vec3(38,110,38)+vec3(sin(phase*3.0),0,cos(phase*3.0)));
        float belt=0.5+0.5*(1.0-smoothstep(0.22,0.8,abs(n.y)));
        density=smoothstep(0.61,0.85,broad*0.72+fine*0.28)*belt*0.19;
        tint=vec3(0.88,0.89,0.86);
    } else {
        // A bounded regional patch: chord distance avoids acos precision at its centre.
        float r=length(n-stormCentre)/stormRadius;
        if(r>1.65) discard;
        float broad=noise3(n*24.0+vec3(sin(phase)*2.0,5,cos(phase)*2.0));
        float fine=noise3(n*83.0+vec3(1,sin(phase)*3.0,cos(phase)*3.0));
        density=(1.0-smoothstep(0.25,1.4,r+(broad-0.5)*0.6))
                *(0.60+0.40*fine)*strength*0.63;
        tint=vec3(0.80,0.57,0.32);
    }
    float alpha=density*edge;
    if(alpha<0.003) discard;
    color=vec4(tint*(0.65+0.35*max(0.0,dot(n,lightDirection))),alpha);
}
)GLSL";
} // namespace mars
