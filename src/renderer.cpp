#include "renderer.hpp"
#include "font.hpp"
#include "preview.hpp"
#include "environment_shaders.hpp"
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace mars {
namespace {
constexpr Vec3 ink{0.028f,0.032f,0.031f};
constexpr Vec3 line{0.40f,0.43f,0.40f},muted{0.66f,0.70f,0.65f};
constexpr Vec3 paper{0.92f,0.94f,0.88f},accent{0.84f,0.91f,0.30f};
const char* terrainVertex=R"GLSL(#version 330 core
layout(location=0) in vec3 direction;
layout(location=1) in float meters;
uniform vec3 outward, rightward, upward, eye, backward;
uniform vec3 projectionParameters;
uniform float exaggeration, aspect, baseRadius, displayRadius;
uniform vec3 bodyCentre;
noperspective out vec3 normalPosition;
out float elevation;
out vec3 surfaceDirection;
void main() {
    vec3 localWorld=direction*(1.0+meters*exaggeration/baseRadius);
    vec3 world=localWorld*displayRadius+bodyCentre;
    // Local, affine coordinates keep facet derivatives stable at very close zoom.
    normalPosition=localWorld-outward;
    elevation=meters;
    surfaceDirection=direction;
    vec3 relative=world-eye;
    float z=-dot(relative,backward);
    float near=projectionParameters.x, far=projectionParameters.y, tanHalfFov=projectionParameters.z;
    gl_Position=vec4(dot(relative,rightward)/(tanHalfFov*aspect),
                     dot(relative,upward)/tanHalfFov,
                     (far+near)/(far-near)*z-2.0*far*near/(far-near),z);
})GLSL";
const std::string terrainFragment=std::string("#version 330 core\n")+sphericalNoise+R"GLSL(
noperspective in vec3 normalPosition;
in float elevation;
in vec3 surfaceDirection;
uniform vec3 lightDirection, outward;
uniform float ice, mercuryStyle, heightColors, scanPhase, scanStrength, moonImage;
uniform sampler2D moonMap;
uniform sampler2D moonSlopeMap;
uniform float moonSlopeReady, exaggeration;
out vec4 color;
void main() {
    vec3 n=normalize(cross(dFdx(normalPosition),dFdy(normalPosition)));
    if(dot(n,normalPosition+outward)<0.0) n=-n;
    if(mercuryStyle>0.5) {
        // Cylindrical cells become subpixel slivers at the poles. Keep geometry
        // measured, but soften their unstable facet shading toward radial light.
        vec3 p=normalize(surfaceDirection);
        float polar=smoothstep(0.9659258,0.9986295,abs(p.y)); // 75 to 87 degrees
        vec3 stable=p;
        if(mercuryStyle>1.5 && moonSlopeReady>0.5) {
            // A measured tangent gradient avoids differentiating subpixel polar
            // slivers. It changes lighting only; geometry and probes stay exact.
            vec2 uv=p.xz/(2.0*0.2588190451)+0.5;
            uv=clamp(uv,vec2(0.5/512.0),vec2(1.0-0.5/512.0));
            uv.y=uv.y*0.5+(p.y<0.0?0.5:0.0);
            vec3 gradient=texture(moonSlopeMap,uv).rgb;
            gradient-=p*dot(p,gradient);
            stable=normalize(p-gradient*exaggeration/(1.0+elevation*exaggeration/1737400.0));
        }
        n=normalize(mix(n,stable,polar*(mercuryStyle>1.5?1.0:0.985)));
    }
    float diffuse=max(0.0,dot(n,lightDirection));
    // Soften only the band boundaries: rounding noise must not flip a face's shade.
    float band=diffuse*4.0;
    float light=0.48+0.13*(floor(band)+smoothstep(0.4,0.6,fract(band)));
    vec3 c;
    if(elevation< -5000.0) c=vec3(0.30,0.22,0.24);
    else if(elevation< -3000.0) c=vec3(0.43,0.27,0.26);
    else if(elevation< -1000.0) c=vec3(0.57,0.33,0.27);
    else if(elevation< 1500.0) c=vec3(0.70,0.41,0.29);
    else if(elevation< 4500.0) c=vec3(0.80,0.50,0.32);
    else if(elevation< 9000.0) c=vec3(0.87,0.61,0.39);
    else if(elevation< 15000.0) c=vec3(0.93,0.73,0.49);
    else c=vec3(0.97,0.86,0.67);
    if(mercuryStyle>0.5) {
        // Height tints are cartographic, never claimed to be an albedo photograph.
        float t=clamp((elevation+5382.0)/9879.0,0.0,1.0);
        float banded=floor(t*7.0)/7.0;
        c=mix(vec3(0.40,0.38,0.35),vec3(0.73,0.70,0.62),banded);
        if(heightColors>0.5) {
            if(elevation< -3000.0) c=vec3(0.21,0.31,0.38);
            else if(elevation< -1500.0) c=vec3(0.31,0.43,0.46);
            else if(elevation< 0.0) c=vec3(0.49,0.54,0.44);
            else if(elevation< 1500.0) c=vec3(0.70,0.62,0.40);
            else if(elevation< 3000.0) c=vec3(0.81,0.54,0.35);
            else c=vec3(0.92,0.77,0.56);
        }
    }
    if(mercuryStyle>1.5) {
        vec3 p=normalize(surfaceDirection);
        float u=atan(-p.z,p.x)/6.28318530718+0.5;
        float v=0.5-asin(clamp(p.y,-1.0,1.0))/3.14159265359;
        // SVS is centred on zero longitude (unlike the DEM). It is appearance,
        // never elevation. Modest palette quantization keeps the atlas style.
        vec3 imageColor=texture(moonMap,vec2(u,v)).rgb;
        float value=dot(imageColor,vec3(0.299,0.587,0.114));
        c=moonImage>0.5?mix(vec3(value),imageColor,0.18):vec3(0.61);
        c=floor(c*32.0+0.5)/32.0;
        if(heightColors>0.5) {
            float t=clamp((elevation+9125.5)/19898.5,0.0,1.0);
            t=floor(t*9.0)/9.0;
            c=t<0.5?mix(vec3(0.20,0.29,0.36),vec3(0.57,0.62,0.55),t*2.0)
                   :mix(vec3(0.57,0.62,0.55),vec3(0.91,0.82,0.62),(t-0.5)*2.0);
        }
    }
    if(ice>0.5) {
        vec3 p=normalize(surfaceDirection);
        // Approximate, irregular residual caps. No invented extra surface height.
        float edge=(noise3(p*45.0)-0.5)*0.004+(noise3(p*115.0)-0.5)*0.001;
        float cap=smoothstep((p.y>0.0?0.987:0.994)+edge,(p.y>0.0?0.989:0.996)+edge,abs(p.y));
        c=mix(c,vec3(0.88,0.89,0.81),cap);
        // Diffuse ice softens the very thin polar facets without altering relief.
        float iceLight=0.82+0.14*max(0.0,dot(p,lightDirection));
        light=mix(light,mix(iceLight,light,0.2),cap);
    }
    vec3 shaded=c*light;
    if(scanStrength>0.0) {
        vec3 p=normalize(surfaceDirection);
        float altitude=elevation/1000.0;
        float contour=1.0-smoothstep(0.0,max(fwidth(altitude)*1.4,0.015),abs(fract(altitude+0.5)-0.5));
        float sweep=1.0-2.0*scanPhase;
        float front=exp(-pow((p.y-sweep)/0.025,2.0));
        float tail=exp(-max(p.y-sweep,0.0)*12.0)*step(sweep,p.y);
        float longitude=atan(-p.z,p.x)/6.2831853;
        float latitude=asin(clamp(p.y,-1.0,1.0))/3.1415927;
        vec2 grid=vec2(longitude*24.0,latitude*18.0);
        vec2 lines=1.0-smoothstep(vec2(0.0),max(fwidth(grid)*1.3,vec2(0.015)),abs(fract(grid+0.5)-0.5));
        vec3 instrument=shaded*0.38+vec3(0.44,0.64,0.48)*(contour*0.5+max(lines.x,lines.y)*0.16);
        instrument+=vec3(0.77,0.94,0.35)*(front*0.8+tail*0.12);
        shaded=mix(shaded,instrument,scanStrength);
    }
    color=vec4(shaded,1.0);
})GLSL";
const char* uiVertex=R"GLSL(#version 330 core
layout(location=0) in vec2 position;
layout(location=1) in vec4 tint;
uniform vec3 canvas;
out vec4 c;
void main() { c=tint; gl_Position=vec4(2.0*round(position.x*canvas.z)/canvas.x-1.0,1.0-2.0*round(position.y*canvas.z)/canvas.y,0,1); }
)GLSL";
const char* uiFragment=R"GLSL(#version 330 core
in vec4 c; out vec4 color;
void main() { color=c; }
)GLSL";
}
GLuint Renderer::program(const char* vertex,const char* fragment) {
    GLuint shaders[2]={0,0},p=0;
    try {
        for(int i=0;i<2;++i) {
            shaders[i]=gl_.CreateShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
            const char* source=i==0?vertex:fragment;
            gl_.ShaderSource(shaders[i],1,&source,nullptr); gl_.CompileShader(shaders[i]);
            GLint ok=0; gl_.GetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);
            if(!ok) { char log[4096]{}; gl_.GetShaderInfoLog(shaders[i],sizeof(log),nullptr,log); throw std::runtime_error(log); }
        }
        p=gl_.CreateProgram();
        for(auto s:shaders) gl_.AttachShader(p,s);
        gl_.LinkProgram(p);
        GLint ok=0; gl_.GetProgramiv(p,GL_LINK_STATUS,&ok);
        if(!ok) { char log[4096]{}; gl_.GetProgramInfoLog(p,sizeof(log),nullptr,log); throw std::runtime_error(log); }
        for(auto s:shaders) gl_.DeleteShader(s);
        return p;
    } catch(...) { for(auto s:shaders) if(s) gl_.DeleteShader(s); if(p) gl_.DeleteProgram(p); throw; }
}
Renderer::Renderer() {
    terrainProgram_=program(terrainVertex,terrainFragment.c_str()); uiProgram_=program(uiVertex,uiFragment);
    weatherProgram_=program(weatherVertex,(std::string("#version 330 core\n")+sphericalNoise+weatherFragment).c_str());
    initPlaceholders();
    gl_.GenVertexArrays(1,&uiVao_); gl_.BindVertexArray(uiVao_);
    gl_.GenBuffers(1,&uiVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(UiVertex),nullptr);
    gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(UiVertex),reinterpret_cast<void*>(sizeof(float)*2));
    gl_.GenFramebuffers(1,&fbo_); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
    gl_.GenTextures(1,&color_); gl_.BindTexture(GL_TEXTURE_2D,color_);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    gl_.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color_,0);
    gl_.GenTextures(1,&depth_); gl_.BindTexture(GL_TEXTURE_2D,depth_);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    gl_.FramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth_,0);
    gl_.Disable(GL_DITHER);
    resize(800,500);
    initPreviews(); initMoonColor();
    check();
}
void Renderer::uploadTerrain(const Mesh& mesh,int slot) {
    clearTerrain(slot);
    auto& gpu=terrain_.at(slot);
    gpu.batches=mesh.batches; gpu.chordFactor=mesh.chordFactor;
    gpu.drawCounts.reserve(gpu.batches.size()); gpu.drawOffsets.reserve(gpu.batches.size());
    gl_.GenVertexArrays(1,&gpu.vao); gl_.BindVertexArray(gpu.vao);
    gl_.GenBuffers(1,&gpu.vbo); gl_.BindBuffer(GL_ARRAY_BUFFER,gpu.vbo);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.vertices.size()*sizeof(TerrainVertex)),mesh.vertices.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),nullptr);
    gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),reinterpret_cast<void*>(sizeof(Vec3)));
    gl_.GenBuffers(1,&gpu.indices); gl_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,gpu.indices);
    gl_.BufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(mesh.indices.size()*sizeof(std::uint32_t)),mesh.indices.data(),GL_STATIC_DRAW);
    gpu.indexCount=GLsizei(mesh.indices.size());
    check();
}
void Renderer::clearTerrain(int slot) {
    auto& gpu=terrain_.at(slot);
    gl_.DeleteBuffers(1,&gpu.indices); gl_.DeleteBuffers(1,&gpu.vbo); gl_.DeleteVertexArrays(1,&gpu.vao);
    gpu.indices=gpu.vbo=gpu.vao=0; gpu.indexCount=gpu.drawnTriangles=0; gpu.batches.clear();
}
void Renderer::initPreviews() {
    const char* executable=SDL_GetBasePath();
    std::filesystem::path base=executable?executable:".";
    for(int i=0;i<3;++i) {
        int slot=i==2?5:i+2;
        const char* name=i==0?"mars.preview":i==1?"mercury.preview":"moon.preview";
        for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"}) {
            auto path=root/name;
            if(!std::filesystem::exists(path)) continue;
            try { auto mesh=solar::loadPreview(path,i==2?solar::PreviewBody::Moon:i==1?solar::PreviewBody::Mercury:solar::PreviewBody::Mars); uploadTerrain(mesh,slot); }
            catch(const std::exception& e) { clearTerrain(slot); SDL_Log("Surface preview unavailable: %s",e.what()); }
            break;
        }
    }
}
bool Renderer::previewAvailable(solar::BodyId body) const {
    return (body==solar::BodyId::Mars&&triangles(2)>0)||(body==solar::BodyId::Mercury&&triangles(3)>0)||(body==solar::BodyId::Moon&&triangles(5)>0);
}
void Renderer::resize(int w,int h) {
    if(w<=0||h<=0 || (w==layout_.pixelWidth && h==layout_.pixelHeight)) return;
    layout_.resize(w,h,layout_.requestedScale);
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
    gl_.BindTexture(GL_TEXTURE_2D,color_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    gl_.BindTexture(GL_TEXTURE_2D,depth_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,w,h,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
    if(gl_.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Offscreen framebuffer incomplete");
    check();
}
Renderer::~Renderer() {
    for(auto& surface:satellites_) clearSatellite(surface);
    gl_.DeleteProgram(satelliteProgram_);
    gl_.DeleteProgram(spaceProgram_); gl_.DeleteVertexArrays(1,&spaceVao_);
    clearSun(); clearIo(); clearCallisto(); clearVenus(); clearEarth(); clearJupiter(); clearSaturn(); gl_.DeleteTextures(1,&moonSource_); gl_.DeleteTextures(1,&moonSlopes_);
    for(auto& b:belts_) { gl_.DeleteBuffers(1,&b.vbo); gl_.DeleteVertexArrays(1,&b.vao); }
    gl_.DeleteProgram(beltProgram_);
    gl_.DeleteBuffers(1,&bodyVbo_); gl_.DeleteVertexArrays(1,&bodyVao_); gl_.DeleteProgram(bodyProgram_);
    gl_.DeleteTextures(1,&depth_); gl_.DeleteTextures(1,&color_); gl_.DeleteFramebuffers(1,&fbo_);
    for(int slot=0;slot<6;++slot) clearTerrain(slot);
    gl_.DeleteBuffers(1,&uiVbo_); gl_.DeleteVertexArrays(1,&uiVao_);
    gl_.DeleteProgram(uiProgram_); gl_.DeleteProgram(terrainProgram_); gl_.DeleteProgram(weatherProgram_);
}
void Renderer::terrain(const Camera& c,float exaggeration,const Environment& e,int slot,bool heightColors) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
    gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    spaceBackground(c);
    drawTerrain(c,exaggeration,e,slot,heightColors);
}
void Renderer::drawTerrain(const Camera& c,float exaggeration,const Environment& e,int slot,bool heightColors,Vec3 centre,float displayRadius) {
    auto& gpu=terrain_.at(slot);
    bool lunar=slot==4||slot==5,mercury=slot==1||slot==3;
    float radius=lunar?moon::radiusMeters:mercury?mercury::radiusMeters:radiusMeters;
    float minimum=lunar?moon::minimumMeters:mercury?mercury::minimumMeters:-8177;
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE);
    gl_.UseProgram(terrainProgram_);
    auto vec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(terrainProgram_,name),v.x,v.y,v.z); };
    auto scalar=[&](const char* name,float v) { gl_.Uniform1f(gl_.GetUniformLocation(terrainProgram_,name),v); };
    vec("outward",c.outward()); vec("rightward",c.right()); vec("upward",c.up());
    vec("eye",c.position()); vec("backward",c.backward());
    vec("projectionParameters",{c.clipNear,Camera::farPlane,c.lens});
    vec("lightDirection",normalized(c.outward()*0.85f+c.up()*0.65f-c.right()*0.55f));
    scalar("exaggeration",exaggeration); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    scalar("ice",!lunar&&!mercury&&e.showIce()?1.0f:0.0f);
    scalar("baseRadius",radius); scalar("mercuryStyle",lunar?2:mercury?1:0); scalar("heightColors",heightColors?1:0);
    scalar("displayRadius",displayRadius); vec("bodyCentre",centre);
    scalar("scanPhase",scanPhase_); scalar("scanStrength",scanStrength_);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindTexture(GL_TEXTURE_2D,moonSource_);
    gl_.Uniform1i(gl_.GetUniformLocation(terrainProgram_,"moonMap"),0);
    scalar("moonImage",moonSource_?1:0);
    gl_.ActiveTexture(GL_TEXTURE1); gl_.BindTexture(GL_TEXTURE_2D,moonSlopes_);
    gl_.Uniform1i(gl_.GetUniformLocation(terrainProgram_,"moonSlopeMap"),1);
    scalar("moonSlopeReady",slot==4&&moonSlopes_?1:0);
    gl_.ActiveTexture(GL_TEXTURE0);
    gl_.BindVertexArray(gpu.vao);
    if(slot==2||slot==3||slot==5) {
        gl_.DrawElements(GL_TRIANGLES,gpu.indexCount,GL_UNSIGNED_INT,nullptr);
        gpu.drawnTriangles=gpu.indexCount/3;
        gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST); return;
    }
    gpu.drawCounts.clear(); gpu.drawOffsets.clear(); gpu.drawnTriangles=0;
    for(const auto& b:gpu.batches) {
        if(culling_&&!batchVisible(b,c,exaggeration,float(layout_.pixelWidth)/layout_.pixelHeight,gpu.chordFactor,radius,minimum)) continue;
        gpu.drawCounts.push_back(GLsizei(b.count));
        gpu.drawOffsets.push_back(reinterpret_cast<const void*>(std::uintptr_t(b.first)*sizeof(std::uint32_t)));
        gpu.drawnTriangles+=int(b.count/3);
    }
    if(!gpu.drawCounts.empty()) gl_.MultiDrawElements(GL_TRIANGLES,gpu.drawCounts.data(),GL_UNSIGNED_INT,gpu.drawOffsets.data(),GLsizei(gpu.drawCounts.size()));
    gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::weather(const Camera& c,float exaggeration,const Environment& e) {
    if(!e.showClouds()&&!e.showDust()) return;
    gl_.UseProgram(weatherProgram_); gl_.BindVertexArray(uiVao_);
    auto vec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(weatherProgram_,name),v.x,v.y,v.z); };
    auto scalar=[&](const char* name,float v) { gl_.Uniform1f(gl_.GetUniformLocation(weatherProgram_,name),v); };
    vec("outward",c.outward()); vec("rightward",c.right()); vec("upward",c.up());
    vec("eye",c.position()); vec("backward",c.backward());
    vec("projectionParameters",{c.clipNear,Camera::farPlane,c.lens});
    vec("canvas",{float(layout_.pixelWidth),float(layout_.pixelHeight),0});
    vec("lightDirection",normalized(c.outward()*0.85f+c.up()*0.65f-c.right()*0.55f));
    vec("stormCentre",e.stormCentre()); scalar("stormRadius",e.stormRadius());
    scalar("strength",e.stormStrength());
    scalar("surfaceMinimum",1.0f-8177.0f*exaggeration/radiusMeters);
    scalar("phase",float(std::fmod(e.time()*0.002,2*pi)));
    gl_.Enable(GL_DEPTH_TEST); gl_.DepthMask(GL_FALSE);
    gl_.Enable(GL_BLEND); gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    // Two analytic shells, inner dust first. Display clearance uses the source's
    // maximum height, follows relief exaggeration, and never changes picking depth.
    for(int kind=1;kind>=0;--kind) {
        if(kind==1?!e.showDust():!e.showClouds()) continue;
        scalar("kind",float(kind));
        scalar("shellRadius",1.0f+21171.0f*exaggeration/radiusMeters+(kind?0.004f:0.010f));
        gl_.DrawArrays(GL_TRIANGLES,0,3);
    }
    gl_.DepthMask(GL_TRUE); gl_.Disable(GL_BLEND); gl_.Disable(GL_DEPTH_TEST);
}
std::optional<Location> Renderer::pick(float x,float y,const Camera& c) {
    int width=layout_.pixelWidth,height=layout_.pixelHeight;
    if(x<0||y<0||x*layout_.scale>=width||y*layout_.scale>=height) return std::nullopt;
    int px=int(x*layout_.scale),py=int(y*layout_.scale); float depth=1;
    gl_.BindFramebuffer(GL_READ_FRAMEBUFFER,fbo_);
    gl_.ReadPixels(px,height-1-py,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);
    if(depth>=1) return std::nullopt;
    float nx=2.0f*(px+0.5f)/width-1.0f;
    float ny=1.0f-2.0f*(py+0.5f)/height;
    return location(c.unproject(nx,ny,depth,float(width)/height));
}
void Renderer::rect(float x,float y,float w,float h,Vec3 c,float alpha) {
    ui_.insert(ui_.end(),{{x,y,c,alpha},{x+w,y,c,alpha},{x,y+h,c,alpha},{x+w,y,c,alpha},{x+w,y+h,c,alpha},{x,y+h,c,alpha}});
}
void Renderer::text(int x,int y,const std::string& s,Vec3 c,float scale) {
    float cursor=float(x);
    for(char ch:s) {
        auto g=glyph(ch);
        for(int row=0;row<7;++row) for(int col=0;col<5;++col)
            if(g[row]&(1<<(4-col))) rect(cursor+col*scale,float(y)+row*scale,float(scale),float(scale),c);
        cursor+=6*scale;
    }
}
void Renderer::buttonLabel(int x,int y,const std::string& label,Vec3 color,bool enabled) {
    auto end=label.find(" / ");
    std::string key=end==std::string::npos?"":label.substr(0,end);
    bool shortcut=(key.size()==1&&((key[0]>='A'&&key[0]<='Z')||(key[0]>='1'&&key[0]<='4')))||
                  key=="ENTER"||key=="SPACE"||key=="HOME"||key=="F11"||key=="ALT+LEFT";
    std::size_t action=shortcut?end+3:0;
    // Destination rows also advertise their 1–4 shortcuts, without mistaking
    // numbered section headings (such as "02 / ENVIRONMENT") for keys.
    if(!shortcut) {
        end=label.find("  ");
        key=end==std::string::npos?"":label.substr(0,end);
        shortcut=(key.size()==1&&key[0]>='1'&&key[0]<='4')||
                 (key.size()==2&&key[0]=='0'&&key[1]>='1'&&key[1]<='4');
        if(shortcut) action=end+2;
    }
    if(!enabled||!shortcut) { text(x,y,label,color); return; }
    constexpr Vec3 cyan{.25f,.88f,1.f},ink{.025f,.06f,.075f};
    Rect cap{x-3,y-3,int(key.size())*6+5,13};
    rect(float(cap.x),float(cap.y),float(cap.w),float(cap.h),cyan);
    border(cap,ink,.85f);
    text(x,y,key,ink);
    // Reuse the existing prefix width so longer action labels still fit.
    text(x+int(action)*6,y,label.substr(action),color);
}
void Renderer::border(Rect r,Vec3 c,float alpha) {
    rect(float(r.x),float(r.y),float(r.w),1,c,alpha); rect(float(r.x),float(r.y+r.h-1),float(r.w),1,c,alpha);
    rect(float(r.x),float(r.y),1,float(r.h),c,alpha); rect(float(r.x+r.w-1),float(r.y),1,float(r.h),c,alpha);
}
void Renderer::crosshair(int x,int y,Vec3 c) {
    rect(float(x-6),float(y),4,1,c); rect(float(x+3),float(y),4,1,c);
    rect(float(x),float(y-6),1,4,c); rect(float(x),float(y+3),1,4,c);
}
void Renderer::hud(const Camera& c,const HeightField& field,const Probe& p,float exaggeration,
                   int selected,float mx,float my,float fps,bool fullscreen,const Environment& e,
                   const SatelliteOrbit& orbit,const NumberEdit& edit) {
    ui_.clear();
    int width=layout_.pixelWidth,height=layout_.pixelHeight;
    float scale=layout_.scale;
    int uw=layout_.width(),uh=layout_.height();
    auto panel=[&](Rect r) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,0.78f);
        rect(float(r.x),float(r.y),float(r.w),1,line,0.7f);
        rect(float(r.x),float(r.y+r.h-1),float(r.w),1,line,0.35f);
    };
    auto button=[&](Rect b,const std::string& label,bool active=false) {
        bool hover=b.contains(mx,my);
        rect(float(b.x),float(b.y),float(b.w),float(b.h),active?accent:ink,active?0.94f:hover?0.95f:0.70f);
        if(hover) border(b,paper,0.7f);
        buttonLabel(b.x+9,b.y+(b.h-7)/2,label,active?ink:paper);
    };
    auto toggle=[&](Rect b,const std::string& label,bool active) {
        button(b,label);
        rect(float(b.x),float(b.y+6),2,float(b.h-12),active?accent:line);
    };
    panel(layout_.title());
    rect(20,20,3,58,accent);
    text(32,28,"MARS",paper,3);
    // Breadcrumbs are drawn by the shared navigation overlay.
    char buf[120];
    if(orbit.enabled) {
        panel(layout_.orbitPanel());
        const char* labels[]={"PERIAPSIS / KM","APOAPSIS / KM","INCLINATION / DEG"};
        auto el=orbit.elements(); double values[]={el.periapsisKm,el.apoapsisKm,el.inclinationDeg};
        for(int i=0;i<3;++i) {
            auto b=layout_.orbitValue(i); text(b.x,b.y-12,labels[i],muted);
            std::snprintf(buf,sizeof(buf),"%.1f",values[i]);
            button(b,edit.field==i?edit.value+" <":buf,edit.field==i);
            button(layout_.orbitMinus(i),"-"); button(layout_.orbitPlus(i),"+");
        }
        button(layout_.orbitPause(),orbit.paused?"SPACE / PLAY":"SPACE / PAUSE",orbit.paused);
        std::snprintf(buf,sizeof(buf),"S / TIME %.0fX",orbit.timeScale);
        button(layout_.orbitSpeed(),buf);
        button(layout_.orbitView(),orbit.limbView?"V / VIEW: HORIZON":"V / VIEW: STRAIGHT DOWN");
        text(28,331,edit.error.empty()?"ALT ABOVE 3396 KM SPHERE":edit.error,edit.error.empty()?muted:accent);
        text(28,345,edit.field>=0?"ENTER / APPLY   ESC / CANCEL":"CLICK VALUE / TYPE TO EDIT",muted);
    } else {
        panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / LOCATIONS",muted);
        for(int i=0;i<4;++i) {
            Rect b=layout_.landmarkButton(i);
            button(b,"0"+std::to_string(i+1)+"  "+landmarks[i].name,i==selected);
        }
    }
    button(layout_.orbitButton(),orbit.enabled?"O / EXIT SATELLITE VIEW":"O / SATELLITE ORBIT",orbit.enabled);
    button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN");
    std::snprintf(buf,sizeof(buf),orbit.enabled?"RELIEF 1X FIXED":"E / RELIEF %.0fX",double(exaggeration));
    button(layout_.reliefButton(),buf); button(layout_.homeButton(),"HOME / RESET");
    auto sc=layout_.scalePanel(); panel(sc);
    bool fitted=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>0.001f;
    std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fitted?" FIT":"");
    text(sc.x+9,sc.y+11,buf,muted);
    button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
    button(layout_.environmentButton(),layout_.environmentOpen?"02 / ENVIRONMENT     -":"02 / ENVIRONMENT     +",layout_.environmentOpen);
    if(layout_.environmentOpen) {
        panel(layout_.environmentPanel());
        text(uw-211,154,"ILLUSTRATIVE / NOT OBSERVED",muted);
        toggle(layout_.layerButton(0),e.ice?"[+] POLAR ICE":"[ ] POLAR ICE",e.showIce());
        toggle(layout_.layerButton(1),e.clouds?"[+] THIN CLOUDS":"[ ] THIN CLOUDS",e.showClouds());
        toggle(layout_.layerButton(2),e.dust?"[+] DUST STORMS":"[ ] DUST STORMS",e.dust&&!e.topography);
        toggle(layout_.layerButton(3),"T / CLEAN TOPOGRAPHY",e.topography);
        button(layout_.pauseButton(),e.paused?"P / RESUME":"P / PAUSE",e.paused);
        button(layout_.stormButton(),"D / PREVIEW");
        text(uw-211,309,e.paused?"CLOCK PAUSED":e.stormActive()?"DUST / REGIONAL EVENT":"DUST / QUIET",muted);
        for(int i=0;i<8;++i) rect(float(uw-71+i*5),310,3,5,accent,e.showDust()&&i<e.stormStrength()*8?1.0f:0.18f);
    } else {
        rect(float(uw-226),148,206,24,ink,0.70f);
        text(uw-217,157,e.topography?"CLEAN TOPOGRAPHY":e.paused?"ILLUSTRATIVE / PAUSED":"ILLUSTRATIVE ICE + WEATHER",muted);
    }

    auto r=layout_.readout(); panel(r);
    text(r.x+10,r.y+9,p.underPointer?"03 / SURFACE PROBE":p.subsatellite?"03 / SUBSATELLITE":"03 / VIEW CENTRE",accent);
    std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(p.location.lat),p.location.lat>=0?'N':'S',p.location.lon);
    text(r.x+10,r.y+26,buf,paper,2);
    rect(float(r.x+10),float(r.y+48),302,1,line,0.35f);
    std::snprintf(buf,sizeof(buf),"%+.0f M",double(p.meters)); text(r.x+10,r.y+57,buf,paper,2);
    text(r.x+136,r.y+56,"ABOVE AREOID",muted);
    text(r.x+136,r.y+69,"MOLA / INTERPOLATED",muted);

    auto legend=layout_.legend(); panel(legend);
    if(orbit.enabled) {
        auto state=orbit.state();
        text(legend.x+10,legend.y+9,"04 / ORBIT TELEMETRY",accent);
        std::snprintf(buf,sizeof(buf),"ALT   %7.1f KM",state.altitudeKm); text(legend.x+10,legend.y+25,buf,paper);
        std::snprintf(buf,sizeof(buf),"SPEED %7.3f KM/S",state.speedKmS); text(legend.x+10,legend.y+39,buf,paper);
        std::snprintf(buf,sizeof(buf),"PERIOD %6.1f MIN",orbit.periodSeconds()/60); text(legend.x+10,legend.y+53,buf,muted);
        std::snprintf(buf,sizeof(buf),"FOV %.0f DEG / TWO-BODY MODEL",orbit.fovDegrees); text(legend.x+10,legend.y+69,buf,muted);
    } else {
        text(legend.x+10,legend.y+9,"04 / ELEVATION",accent);
        const Vec3 colors[]={{.30f,.22f,.24f},{.43f,.27f,.26f},{.57f,.33f,.27f},{.70f,.41f,.29f},
                             {.80f,.50f,.32f},{.87f,.61f,.39f},{.93f,.73f,.49f},{.97f,.86f,.67f}};
        for(int i=0;i<8;++i) rect(float(legend.x+10+23*i),float(legend.y+27),22,6,colors[i]);
        text(legend.x+10,legend.y+40,"-8.2 KM",muted); text(legend.x+146,legend.y+40,"+21.2 KM",muted);
        text(legend.x+10,legend.y+57,e.topography||(!e.ice&&!e.clouds&&!e.dust)?"BASE HEIGHT COLOURS":"BASE HEIGHT + VISUAL LAYERS",muted);
        text(legend.x+10,legend.y+70,c.distance<1.10f?"REGIONAL DATA / ZOOM LIMIT":"16 PPD / REGIONAL DATA",muted);
    }
    rect(20,float(uh-21),float(uw-40),21,ink,0.78f);
    rect(20,float(uh-21),float(uw-40),1,line,0.4f);
    text(20,uh-13,orbit.enabled?"DRAG / LOOK   WHEEL / FOV   SPACE / PAUSE   O / EXIT ORBIT":"DRAG / ORBIT   WHEEL / ZOOM   TAB / HIDE UI   [ ] / UI SCALE",muted);
    if(fps>0) std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps));
    else std::snprintf(buf,sizeof(buf),"-- FPS");
    text(uw-74,uh-13,buf,muted);

    if(p.underPointer) crosshair(int(mx),int(my),paper);
    else crosshair(uw/2,uh/2,muted);
    // Landmark markers are labels, not fictitious terrain. Hide those occluded by terrain/HUD.
    for(int i=0;i<4;++i) {
        auto l=landmarks[i];
        Vec3 pos=direction(l.lat,l.lon)*(1+field.sample(l.lat,l.lon)*exaggeration/radiusMeters);
        Vec3 relative=pos-c.position();
        float z=-dot(relative,c.backward());
        if(z<=Camera::nearPlane || dot(pos,c.position()-pos)<=0) continue;
        float nx=dot(relative,c.right())/(z*c.lens*(float(width)/height));
        float ny=dot(relative,c.up())/(z*c.lens);
        int px=int((nx+1)*0.5f*width),py=int((1-ny)*0.5f*height);
        int x=px/scale,y=py/scale;
        if(x<8||y<8||x>=uw-14||y>=uh-8||layout_.overHud(float(x),float(y))) continue;
        float depth=1; gl_.ReadPixels(px,height-1-py,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);
        float visibleZ=2*Camera::nearPlane*Camera::farPlane/(Camera::farPlane+Camera::nearPlane-(2*depth-1)*(Camera::farPlane-Camera::nearPlane));
        if(depth<1 && z>visibleZ+0.008f) continue;
        border({x-3,y-3,7,7},i==selected?paper:accent);
        text(x+7,y-3,std::to_string(i+1),accent);
    }
    flushUi();
}
void Renderer::flushUi() {
    int width=layout_.pixelWidth,height=layout_.pixelHeight; float scale=layout_.scale;
    gl_.Viewport(0,0,width,height); gl_.UseProgram(uiProgram_); gl_.BindVertexArray(uiVao_);
    gl_.Uniform3f(gl_.GetUniformLocation(uiProgram_,"canvas"),float(width),float(height),float(scale));
    gl_.BindBuffer(GL_ARRAY_BUFFER,uiVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(ui_.size()*sizeof(UiVertex)),ui_.data(),GL_STREAM_DRAW);
    gl_.Enable(GL_BLEND); gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    gl_.DrawArrays(GL_TRIANGLES,0,GLsizei(ui_.size()));
    gl_.Disable(GL_BLEND);
}
void Renderer::present() {
    int w=layout_.pixelWidth,h=layout_.pixelHeight;
    gl_.BindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
    gl_.BindFramebuffer(GL_READ_FRAMEBUFFER,fbo_);
    gl_.BlitFramebuffer(0,0,w,h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);
}
void Renderer::screenshot(const std::string& path) {
    int width=layout_.pixelWidth,height=layout_.pixelHeight;
    gl_.BindFramebuffer(GL_READ_FRAMEBUFFER,fbo_); gl_.PixelStorei(GL_PACK_ALIGNMENT,1);
    std::vector<unsigned char> pixels(width*height*3);
    gl_.ReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream f(path,std::ios::binary); f<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y) f.write(reinterpret_cast<const char*>(pixels.data()+y*width*3),width*3);
    if(!f) throw std::runtime_error("Cannot save screenshot: "+path);
}
std::string Renderer::device() const { return reinterpret_cast<const char*>(gl_.GetString(GL_RENDERER)); }
void Renderer::check() const {
    auto e=gl_.GetError();
    if(e==GL_OUT_OF_MEMORY) throw std::runtime_error("GPU allocation failed; try --detail 8 or --detail 4");
    if(e!=GL_NO_ERROR) throw std::runtime_error("OpenGL error: "+std::to_string(e));
}
} // namespace mars
