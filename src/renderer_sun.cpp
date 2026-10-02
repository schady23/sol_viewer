#include "renderer.hpp"
#include "sun_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
void Renderer::prepareSun(const sun::Clock& clock) {
    sunPhase_=float(clock.phase());
    if(sunProgram_) return;
    sunProgram_=program(sun::vertex,sun::fragment);
    gl_.GenVertexArrays(1,&sunVao_);
    std::vector<unsigned char> noise(64*64*64*4); std::uint32_t seed=0x650eabd1u;
    for(auto& v:noise) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; v=static_cast<unsigned char>(seed>>24); }
    gl_.ActiveTexture(GL_TEXTURE0); gl_.GenTextures(1,&sunNoise_); gl_.BindTexture(GL_TEXTURE_3D,sunNoise_);
    gl_.TexImage3D(GL_TEXTURE_3D,0,GL_RGBA8,64,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,noise.data());
    gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    for(auto axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}) gl_.TexParameteri(GL_TEXTURE_3D,axis,GL_REPEAT);
    check(); std::cout<<"Sun: analytic photosphere / granulation, active regions, corona and looping plasma ejections\n";
}
void Renderer::clearSun() {
    gl_.DeleteProgram(sunProgram_); gl_.DeleteVertexArrays(1,&sunVao_); gl_.DeleteTextures(1,&sunNoise_);
    sunProgram_=sunVao_=sunNoise_=0;
}
void Renderer::drawSun(const Camera& camera,Vec3 centre,float radius) {
    if(!sunProgram_) return;
    gl_.UseProgram(sunProgram_); gl_.BindVertexArray(sunVao_);
    auto vec=[&](const char* n,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(sunProgram_,n),v.x,v.y,v.z); };
    auto scalar=[&](const char* n,float v) { gl_.Uniform1f(gl_.GetUniformLocation(sunProgram_,n),v); };
    vec("eye",camera.position()); vec("centre",centre); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward());
    scalar("radius",radius); scalar("lens",camera.lens); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight); scalar("clipNear",camera.clipNear); scalar("phase",sunPhase_);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindTexture(GL_TEXTURE_3D,sunNoise_); gl_.Uniform1i(gl_.GetUniformLocation(sunProgram_,"noiseVolume"),0);
    gl_.Enable(GL_DEPTH_TEST); gl_.Disable(GL_CULL_FACE); gl_.Enable(GL_BLEND); gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    gl_.DrawArrays(GL_TRIANGLES,0,3);
    gl_.Disable(GL_BLEND); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::sunHud(const sun::Clock& clock,float mx,float my) {
    ui_.clear();
    constexpr Vec3 ink{.028f,.032f,.031f},paper{.92f,.94f,.88f},muted{.66f,.70f,.65f},accent{.84f,.91f,.30f};
    auto button=[&](Rect r,const std::string& label) {
        rect(r.x,r.y,r.w,r.h,ink,.9f); if(r.contains(mx,my)) border(r,paper,.8f); buttonLabel(r.x+8,r.y+8,label,paper);
    };
    button(layout_.sunPause(),clock.paused?"SPACE / PLAY ACTIVITY":"SPACE / PAUSE ACTIVITY");
    button(layout_.sunSpeed(),"S / ACTIVITY "+std::to_string(clock.speed)+"X");
    button(layout_.sunReset(),"R / RESTART ACTIVITY");
    text(layout_.width()-218,218,"ILLUSTRATIVE / 90 SECOND LOOP",muted);
    button(layout_.sunClose(),"1 / PHOTOSPHERE CLOSEUP");
    button(layout_.sunCrown(),"2 / CORONA AND EJECTIONS");
    button(layout_.sunScale(),"3 / FULL SUN VIEW");
    auto panel=layout_.sunScalePanel(); rect(panel.x,panel.y,panel.w,panel.h,ink,.88f);
    text(panel.x+8,panel.y+10,"THE SCALE OF OUR STAR",accent);
    text(panel.x+8,panel.y+30,"DIAMETER / 1,391,400 KM",paper);
    text(panel.x+8,panel.y+46,"ABOUT 109 EARTH DIAMETERS",paper);
    text(panel.x+8,panel.y+62,"DYNAMIC ACTIVE REGIONS",muted);
    text(panel.x+8,panel.y+78,"GRANULATION / SOLAR PLASMA",muted);
    flushUi();
}
}
