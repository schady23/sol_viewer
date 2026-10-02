#pragma once
namespace sun {
inline constexpr const char* vertex=R"GLSL(#version 330 core
out vec2 uv;
void main() {
    vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);
    uv=p*2.-1.; gl_Position=vec4(uv,0.,1.);
})GLSL";
inline constexpr const char* fragment=R"GLSL(#version 330 core
in vec2 uv;
out vec4 color;
uniform vec3 eye,centre,rightward,upward,backward;
uniform float radius,lens,aspect,clipNear,phase;
uniform sampler3D noiseVolume;
const float PI=3.14159265359;
float noise(vec3 p) { return textureLod(noiseVolume,p/64.,0.).r; }
vec3 turn(vec3 p,float a) { float c=cos(a),s=sin(a); return vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z); }
vec3 region(int i) {
    float lat=.38*sin(float(i)*2.7+.2)+.025*sin(phase*3.+float(i)*1.7);
    float lon=float(i)*2.39996+.05*sin(phase*2.+float(i));
    return vec3(cos(lat)*cos(lon),sin(lat),-cos(lat)*sin(lon));
}
void main() {
    vec3 origin=(eye-centre)/radius;
    vec3 ray=normalize(rightward*(uv.x*lens*aspect)+upward*(uv.y*lens)-backward);
    float along=-dot(origin,ray);
    if(along<=0.) discard;
    vec3 nearest=origin+ray*along;
    float impact=length(nearest),disc=1.-impact*impact;
    vec3 rgb; float alpha,t;
    if(disc>=0.) {
        t=along-sqrt(disc);
        if(t<=0.) discard;
        vec3 n=normalize(origin+ray*t);
        // Integer rotations preserve the seamless authored activity cycle.
        vec3 p=turn(n,-phase);
        vec3 flow=vec3(sin(phase),cos(phase),sin(2.*phase));
        float broad=noise(p*18.+flow*1.3);
        float cells=noise(p*240.+flow*2.+broad*4.);
        float grains=noise(p*630.+flow*3.);
        float filament=noise(p*75.+vec3(broad*6.,cells*4.,grains*2.));
        float emission=.55+.40*cells+.17*grains+.15*filament;
        rgb=mix(vec3(.95,.20,.018),vec3(1.,.69,.18),smoothstep(.22,.78,cells));
        rgb*=emission;
        for(int i=0;i<10;++i) {
            float d=1.-dot(p,region(i));
            float energized=exp(-d*85.);
            float flare=pow(max(0.,sin(phase*3.+float(i)*1.9)),12.);
            if(d<.045) {
                vec3 axis=region(i);
                vec3 east=normalize(cross(vec3(0.,1.,0.),axis));
                vec3 north=cross(axis,east);
                float life=smoothstep(.08,.90,.5+.5*sin(phase*2.+float(i)*.97));
                float size=.024+.037*life;
                vec2 q=vec2(dot(p,east),dot(p,north))/size;
                float a=.5*sin(phase+float(i)*2.1);
                q=mat2(cos(a),-sin(a),sin(a),cos(a))*q;
                // Coherent warp gives evolving ragged outlines instead of
                // circular Gaussian dots. Nearby pores form a bipolar group.
                vec3 warp=textureLod(noiseVolume,(p*115.+flow*3.+float(i)*7.)/64.,0.).rgb;
                q+=(warp.xy-.5)*.48;
                float primary=length(q*vec2(.82,1.25));
                float secondary=length((q-vec2(1.05+.18*sin(phase*3.+float(i)),.20))*vec2(1.10,1.40));
                float pore=length((q+vec2(.90,.62+.16*cos(phase*2.)))*vec2(2.1,2.8));
                float shape=min(primary,min(secondary,pore));
                float penumbra=1.-smoothstep(.55,1.35,shape);
                float umbra=1.-smoothstep(.23,.59,shape+(warp.z-.5)*.22);
                float fibers=.5+.5*sin(atan(q.y,q.x)*31.+warp.z*8.);
                rgb*=1.-life*(penumbra*(.22+.10*fibers)+umbra*.57);
            }
            rgb+=energized*(.13+.8*flare)*vec3(1.,.57,.18);
            rgb+=exp(-d*320.)*flare*vec3(1.,.95,.7);
        }
        float mu=max(0.,dot(n,-ray));
        rgb*=.40+.60*pow(mu,.45); // Photospheric limb darkening, not a day/night terminator.
        rgb=vec3(1.)-exp(-rgb*1.7);
        for(int i=0;i<10;++i) {
            float flare=pow(max(0.,sin(phase*3.+float(i)*1.9)),12.);
            float core=exp(-(1.-dot(p,region(i)))*260.);
            rgb=mix(rgb,vec3(1.,.96,.78),core*flare*.85);
        }
        alpha=1.;
    } else {
        vec3 n=normalize(nearest);
        float altitude=impact-1.;
        if(altitude>2.4) discard;
        vec3 p=turn(n,-phase);
        float angle=atan(dot(n,upward),dot(n,rightward));
        float streamer=pow(.5+.5*sin(angle*19.+noise(p*15.)*7.+sin(phase)*.5),4.);
        float haze=exp(-altitude*10.)*.75+exp(-altitude*3.5)*(.025+streamer*.13);
        rgb=vec3(1.,.31,.055)*haze;
        // Expanding hollow clouds detach from active regions; each fades to
        // zero with zero slope before its geometry resets at the loop boundary.
        for(int i=0;i<10;++i) {
            float age=fract(phase/(2.*PI)+float(i)*.137);
            float life=pow(sin(PI*age),2.);
            vec3 axis=turn(region(i),phase);
            vec3 cloudCentre=axis*(1.02+age*1.8);
            float cloudRadius=.10+age*.62;
            float cloudAlong=dot(cloudCentre-origin,ray);
            vec3 closest=origin+ray*cloudAlong-cloudCentre;
            float edge=length(closest)/cloudRadius;
            edge+=(noise(closest*35.+float(i)*13.)-.5)*.08;
            float front=dot(normalize(closest+vec3(.0001)),axis);
            float shell=exp(-pow((edge-.91)*18.,2.))*mix(.08,1.,smoothstep(-.4,.6,front));
            float striation=.45+.55*noise((origin+ray*cloudAlong)*95.+float(i)*11.);
            float visibility=smoothstep(-.15,.20,dot(axis,-ray));
            rgb+=vec3(1.,.38,.075)*shell*life*striation*visibility*.95;
            // Low arch of luminous plasma with two anchored footpoints.
            float cap=dot(n,axis);
            float arc=exp(-pow((cap-.965)*130.,2.))*exp(-pow((altitude-.08-.20*life)*16.,2.));
            rgb+=vec3(1.,.44,.09)*arc*life*.6;
        }
        alpha=clamp(max(rgb.r,max(rgb.g,rgb.b)),0.,.88);
        if(alpha<.002) discard;
        rgb/=max(alpha,.001);
        t=along;
    }
    float z=-dot(ray*t*radius,backward);
    float far=20.;
    gl_FragDepth=clamp(.5+.5*((far+clipNear)/(far-clipNear)-2.*far*clipNear/((far-clipNear)*z)),0.,1.);
    color=vec4(rgb,alpha);
})GLSL";
}
