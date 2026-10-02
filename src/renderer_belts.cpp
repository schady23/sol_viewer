#include "renderer.hpp"
#include "belts.hpp"
namespace mars {
namespace {
struct BeltVertex { Vec3 point,tint; };
const char* vertex=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 tint;
uniform vec3 eye,rightward,upward,backward,origin;
uniform float scale,aspect,lens,clipNear,pixelSize;
out vec3 color;
void main() {
    vec3 rel=position*scale-origin-eye; float z=-dot(rel,backward);
    float n=clipNear,f=20.0;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,(f+n)/(f-n)*z-2.0*f*n/(f-n),z);
    gl_PointSize=pixelSize; color=tint;
})GLSL";
const char* fragment=R"GLSL(#version 330 core
in vec3 color; uniform int points; uniform float opacity;
out vec4 outputColor;
void main() {
    float alpha=opacity;
    if(points==1) { float r=length(gl_PointCoord*2.0-1.0); if(r>1.0) discard; alpha*=1.0-smoothstep(.2,1.0,r); }
    outputColor=vec4(color,alpha);
})GLSL";
}
void Renderer::prepareBelt(int slot) {
    auto& gpu=belts_.at(slot); if(gpu.vao) return;
    if(!beltProgram_) beltProgram_=program(vertex,fragment);
    auto id=slot==0?solar::BodyId::AsteroidBelt:solar::BodyId::KuiperBelt;
    std::vector<BeltVertex> vertices;
    for(auto sample:solar::belts::distribution(id)) {
        auto tint=slot==0?Vec3{.60f,.52f,.40f}:sample.population==3?Vec3{.36f,.49f,.64f}:sample.population==2?Vec3{.59f,.61f,.76f}:Vec3{.49f,.67f,.73f};
        vertices.push_back({sample.point,tint*sample.brightness});
    }
    gpu.points=GLsizei(vertices.size());
    // Light radial guides are boundaries/reference distances, not solid matter.
    auto circle=[&](double au,Vec3 tint) {
        for(int j=0;j<512;++j) for(int k:{j,j+1}) vertices.push_back({direction(0,k*360./512)*float(au),tint});
    };
    if(slot==0) { circle(2.1,{.29f,.28f,.22f}); circle(3.3,{.29f,.28f,.22f}); circle(1.52371034,{.22f,.17f,.13f}); circle(5.202887,{.20f,.19f,.16f}); }
    else { circle(30.06992276,{.19f,.27f,.37f}); circle(50,{.22f,.30f,.32f}); }
    for(auto child:solar::children(id)) {
        const auto& d=solar::belts::dwarf(child);
        for(int j=0;j<512;++j) for(int k:{j,j+1}) vertices.push_back({solar::belts::orbit(d.a,d.e,d.i,d.node,d.peri,k*360./512,1),solar::body(child).color*.32f});
    }
    gpu.lines=GLsizei(vertices.size())-gpu.points;
    gpu.ringFirst=GLsizei(vertices.size());
    if(slot==1) {
        for(int j=0;j<1024;++j) {
            auto p=direction(0,j*360./1024),q=direction(0,(j+1)*360./1024);
            float inner=2252.f/1161,outer=2322.f/1161;
            for(auto v:{p*inner,p*outer,q*inner,p*outer,q*outer,q*inner}) vertices.push_back({v,{.55f,.57f,.55f}});
        }
    }
    gpu.ringCount=GLsizei(vertices.size())-gpu.ringFirst;
    gl_.GenVertexArrays(1,&gpu.vao); gl_.BindVertexArray(gpu.vao);
    gl_.GenBuffers(1,&gpu.vbo); gl_.BindBuffer(GL_ARRAY_BUFFER,gpu.vbo);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(vertices.size()*sizeof(BeltVertex)),vertices.data(),GL_STATIC_DRAW);
    for(int i=0;i<2;++i) { gl_.EnableVertexAttribArray(i); gl_.VertexAttribPointer(i,3,GL_FLOAT,GL_FALSE,sizeof(BeltVertex),reinterpret_cast<void*>(i*sizeof(Vec3))); }
}
void Renderer::beltCamera(const Camera& camera,Vec3 origin,float scale) {
    gl_.UseProgram(beltProgram_);
    auto vec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(beltProgram_,name),v.x,v.y,v.z); };
    auto scalar=[&](const char* name,float v) { gl_.Uniform1f(gl_.GetUniformLocation(beltProgram_,name),v); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("origin",origin);
    scalar("scale",scale); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear);
    scalar("pixelSize",std::max(1.4f,1.5f*layout_.scale));
    gl_.Enable(GL_DEPTH_TEST); gl_.Disable(GL_CULL_FACE); gl_.Enable(GL_BLEND); gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); gl_.DepthMask(GL_FALSE);
}
void Renderer::drawBelts(const solar::SceneFrame& scene) {
    if(!solar::belts::region(scene.belt)) return;
    for(int slot=0;slot<2;++slot) {
        auto id=slot==0?solar::BodyId::AsteroidBelt:solar::BodyId::KuiperBelt;
        if(scene.belt!=id) continue;
        prepareBelt(slot); const auto& gpu=belts_[slot];
        beltCamera(scene.camera,scene.renderOrigin,float(solar::belts::units(id)));
        gl_.BindVertexArray(gpu.vao); gl_.Enable(GL_PROGRAM_POINT_SIZE);
        gl_.Uniform1i(gl_.GetUniformLocation(beltProgram_,"points"),1);
        gl_.Uniform1f(gl_.GetUniformLocation(beltProgram_,"opacity"),.72f);
        gl_.DrawArrays(GL_POINTS,0,gpu.points); gl_.Disable(GL_PROGRAM_POINT_SIZE);
        {
            gl_.Uniform1i(gl_.GetUniformLocation(beltProgram_,"points"),0);
            gl_.Uniform1f(gl_.GetUniformLocation(beltProgram_,"opacity"),.55f);
            gl_.DrawArrays(GL_LINES,gpu.points,gpu.lines);
        }
        gl_.DepthMask(GL_TRUE); gl_.Disable(GL_BLEND); gl_.Disable(GL_DEPTH_TEST);
    }
}
void Renderer::drawHaumeaRing(const Camera& camera,Vec3 centre,float radius) {
    prepareBelt(1); const auto& gpu=belts_[1];
    beltCamera(camera,centre*-1.f,radius); gl_.BindVertexArray(gpu.vao);
    gl_.Uniform1i(gl_.GetUniformLocation(beltProgram_,"points"),0);
    gl_.Uniform1f(gl_.GetUniformLocation(beltProgram_,"opacity"),.5f);
    gl_.DrawArrays(GL_TRIANGLES,gpu.ringFirst,gpu.ringCount);
    gl_.DepthMask(GL_TRUE); gl_.Disable(GL_BLEND); gl_.Disable(GL_DEPTH_TEST);
}
}
