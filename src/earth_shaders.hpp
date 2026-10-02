#pragma once
namespace earth {
inline constexpr const char* vertex=R"GLSL(#version 330 core
uniform float clipNear;
layout(location=0) in vec3 direction;
layout(location=1) in float meters;
uniform vec3 eye,rightward,upward,backward,centre;
uniform float radius,aspect,lens,shell,exaggeration;
uniform int cloudPass,topography;
out vec3 point,unitDirection;
void main() {
    unitDirection=direction;
    float height=topography==1?meters:max(0.,meters);
    point=direction*(cloudPass>0?shell:1.+height*exaggeration/6371000.);
    vec3 rel=point*radius+centre-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.*f*n/(f-n),z);
})GLSL";
inline constexpr const char* fragment=R"GLSL(#version 330 core
uniform sampler2D cloudMap,surfaceMap,elevationMap;
uniform sampler3D noiseVolume;
uniform vec3 lightDirection,eyeLocal,loopPhase;
uniform float exaggeration;
uniform int cloudPass,cloudsVisible,topography;
in vec3 point,unitDirection;
out vec4 color;
const float PI=3.14159265359;
vec2 uvOf(vec3 p) { return vec2(.5+atan(-p.z,p.x)/(2.*PI),.5-asin(clamp(p.y,-1.,1.))/PI); }
vec2 wrappedDerivative(vec2 d) { d.x-=round(d.x); return d; }
float noise(vec3 p) { return texture(noiseVolume,p/16.+vec3(.5/128.)).r; }
float fbm(vec3 p) { return .57*noise(p)+.28*noise(p*2.07+7.)+.15*noise(p*4.13+23.); }
vec3 drift(vec3 n,float layer) {
    // Closed paths and integer harmonics give matching positions AND velocities.
    vec3 a=vec3(noise(n*5.+11.),noise(n*5.+31.),noise(n*5.+53.))-.5;
    vec3 b=vec3(noise(n*8.+7.),noise(n*8.+23.),noise(n*8.+47.))-.5;
    vec3 east=cross(n,vec3(0,1,0));
    float shear=sin(n.y*12.+layer*2.);
    return normalize(n+.045*((loopPhase.x-1.)*a+loopPhase.y*b)
        +east*(.012*shear*loopPhase.y+layer*.009*loopPhase.x));
}
float cloudDensity(vec3 n,float layer) {
    vec3 p=drift(n,layer); vec2 uv=uvOf(p);
    float atlas=textureGrad(cloudMap,uv,wrappedDerivative(dFdx(uv)),wrappedDerivative(dFdy(uv))).r;
    vec3 motion=vec3(loopPhase.y,loopPhase.x,2.*loopPhase.x*loopPhase.y);
    float billows=fbm(p*(layer<.5?115.:180.)+motion*.7);
    float fine=noise(p*440.+motion*1.2);
    if(layer>.5) {
        // Translucent, independently advected high cloud, feathered by detail.
        return smoothstep(.10,.80,atlas)*smoothstep(.35,.74,billows)*.34;
    }
    float coverage=smoothstep(.025,.80,atlas);
    float erosion=(1.-coverage)*(.24*(billows-.5)+.075*(fine-.5));
    return clamp(coverage+erosion*smoothstep(.02,.18,atlas),0.,1.);
}
float elevation(vec2 uv) { return textureLod(elevationMap,uv,0.).r; }
vec3 reliefNormal(vec3 n,vec2 uv) {
    vec2 stepUV=1./vec2(textureSize(elevationMap,0));
    float west=elevation(uv-vec2(stepUV.x,0)),east=elevation(uv+vec2(stepUV.x,0));
    float north=elevation(uv-vec2(0,stepUV.y)),south=elevation(uv+vec2(0,stepUV.y));
    if(topography==0) { west=max(0.,west); east=max(0.,east); north=max(0.,north); south=max(0.,south); }
    vec3 e=normalize(vec3(n.z,0.,-n.x)+vec3(1e-8,0,0));
    vec3 t=normalize(cross(n,e));
    float latitudeScale=max(.025,length(n.xz));
    vec2 slope=vec2((east-west)/(2.*stepUV.x*2.*PI*6371000.*latitudeScale),
                    (north-south)/(2.*stepUV.y*PI*6371000.));
    // Source-derived slopes; no procedural mountains or bathymetry.
    float strength=topography==1?max(3.,exaggeration):max(2.,exaggeration*.65);
    return normalize(n-strength*(e*slope.x+t*slope.y));
}
vec3 topographicColor(float h) {
    if(h< -4500.) return mix(vec3(.075,.11,.22),vec3(.16,.34,.46),clamp((h+11000.)/6500.,0.,1.));
    if(h<0.) return mix(vec3(.16,.34,.46),vec3(.60,.79,.75),(h+4500.)/4500.);
    if(h<500.) return mix(vec3(.26,.46,.27),vec3(.51,.61,.33),h/500.);
    if(h<2000.) return mix(vec3(.51,.61,.33),vec3(.77,.67,.43),(h-500.)/1500.);
    if(h<4500.) return mix(vec3(.77,.67,.43),vec3(.57,.39,.29),(h-2000.)/2500.);
    return mix(vec3(.57,.39,.29),vec3(.97,.95,.88),clamp((h-4500.)/3500.,0.,1.));
}
void main() {
    vec3 n=normalize(unitDirection),view=normalize(eyeLocal-point);
    float daylight=max(0.,dot(n,lightDirection));
    if(cloudPass>0) {
        float layer=cloudPass==2?1.:0.;
        float cover=cloudDensity(n,layer);
        float sunward=cloudDensity(normalize(n+lightDirection*.005),layer);
        float shade=clamp((sunward-cover)*2.,-.22,.45);
        float silver=pow(max(0.,dot(view,-lightDirection)),5.)*(1.-cover);
        vec3 base=mix(vec3(.53,.64,.76),vec3(1.,.985,.95),sqrt(cover));
        color=vec4(base*(.40+.60*daylight)*(1.-shade)+silver*.15,
                   (1.-exp(-cover*(cloudPass==2?1.5:2.8))));
    } else {
        vec2 uv=uvOf(n),dx=wrappedDerivative(dFdx(uv)),dy=wrappedDerivative(dFdy(uv));
        float h=elevation(uv);
        vec3 face=reliefNormal(n,uv);
        vec3 base=topography==1?topographicColor(h):textureGrad(surfaceMap,uv,dx,dy).rgb;
        float light=.28+.72*max(0.,dot(face,lightDirection));
        float shadow=cloudsVisible==1?cloudDensity(normalize(n+lightDirection*.008),0.)*.27:0.;
        vec3 surface=base*light*(1.-shadow);
        if(topography==1) {
            // Anti-aliased 1-km isohypses fade when the pixel spans many contours.
            float width=max(fwidth(h),1.);
            float contour=1.-smoothstep(width*.65,width*1.65,abs(mod(h+500.,1000.)-500.));
            float visible=1.-smoothstep(150.,650.,width);
            surface*=1.-.19*contour*visible;
            float coast=1.-smoothstep(width,3.*width+8.,abs(h));
            surface=mix(surface,vec3(.91,.89,.69),coast*.45);
        } else {
            float ocean=1.-smoothstep(-35.,15.,h);
            vec3 halfway=normalize(lightDirection+view);
            float fresnel=.025+.30*pow(1.-max(0.,dot(n,view)),5.);
            surface+=vec3(.75,.85,.93)*pow(max(0.,dot(n,halfway)),160.)*ocean*(.2+fresnel);
            surface+=vec3(.045,.16,.32)*pow(1.-max(0.,dot(n,view)),3.)*.55;
        }
        color=vec4(surface,1.);
    }
})GLSL";
}
