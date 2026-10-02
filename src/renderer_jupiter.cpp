#include "renderer.hpp"
#include "jupiter_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
void Renderer::initJupiter(bool lowDetail) {
    const char* executable=SDL_GetBasePath(); std::filesystem::path base=executable?executable:".";
    std::filesystem::path path;
    for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"})
        if(std::filesystem::exists(root/"jupiter2015.rgb")) { path=root/"jupiter2015.rgb"; break; }
    if(path.empty()) throw std::runtime_error("Missing assets/jupiter2015.rgb; see docs/JUPITER.md");
    auto source=jupiter::Image::load(path);
    // The native archival map supplies the large cloud structures. Only the
    // unsupported caps are filled; fine motion/relief are illustrative shaders.
    int cap=int(source.height/12),edge=int(source.height*.06);
    for(int pole=0;pole<2;++pole) {
        double mean[3]{}; int row=pole?source.height-1-cap:cap;
        for(int x=0;x<source.width;++x) for(int c=0;c<3;++c) mean[c]+=source.rgb[(row*source.width+x)*3+c]/double(source.width);
        for(int j=0;j<cap;++j) {
            int y=pole?source.height-1-j:j,from=std::clamp(y,edge,source.height-1-edge);
            double t=double(cap-j)/cap; t=t*t*(3-2*t);
            for(int x=0;x<source.width;++x) for(int c=0;c<3;++c)
                source.rgb[(y*source.width+x)*3+c]=static_cast<unsigned char>(source.rgb[(from*source.width+x)*3+c]*(1-t)+mean[c]*t);
        }
    }
    std::array<float,1024> winds{};
    for(int y=0;y<1024;++y) winds[y]=float(jupiter::jetSpeed(90-(y+.5)*180/1024));
    jupiterProgram_=program(jupiter::globeVertex,jupiter::globeFragment);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.GenTextures(1,&jupiterSource_); gl_.BindTexture(GL_TEXTURE_2D,jupiterSource_);
    gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGB8,source.width,source.height,0,GL_RGB,GL_UNSIGNED_BYTE,source.rgb.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl_.GenerateMipmap(GL_TEXTURE_2D);
    gl_.GenTextures(1,&jupiterWind_); gl_.BindTexture(GL_TEXTURE_2D,jupiterWind_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_R32F,1,1024,0,GL_RED,GL_FLOAT,winds.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    // Reusable periodic noise volume. Hardware trilinear filtering makes the
    // multi-scale filament shader substantially cheaper than repeated hash noise.
    std::vector<unsigned char> noise(64*64*64*4); std::uint32_t seed=0x91e10da5u;
    for(auto& value:noise) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; value=static_cast<unsigned char>(seed>>24); }
    gl_.GenTextures(1,&jupiterNoise_); gl_.BindTexture(GL_TEXTURE_3D,jupiterNoise_);
    gl_.TexImage3D(GL_TEXTURE_3D,0,GL_RGBA8,64,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,noise.data());
    gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    for(auto axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}) gl_.TexParameteri(GL_TEXTURE_3D,axis,GL_REPEAT);
    jupiterSize_=lowDetail?256:768;
    auto mesh=makeSampledGlobe(jupiterSize_,[](double,double) { return 0.f; });
    gl_.GenVertexArrays(1,&jupiterVao_); gl_.BindVertexArray(jupiterVao_);
    gl_.GenBuffers(1,&jupiterVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,jupiterVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.vertices.size()*sizeof(TerrainVertex)),mesh.vertices.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),nullptr);
    gl_.GenBuffers(1,&jupiterIbo_); gl_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,jupiterIbo_);
    gl_.BufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(mesh.indices.size()*sizeof(std::uint32_t)),mesh.indices.data(),GL_STATIC_DRAW);
    jupiterIndexCount_=GLsizei(mesh.indices.size()); check();
    std::cout<<"Jupiter: Juno-inspired cloud detail / "<<source.width<<" x "<<source.height<<" archival structure / "<<jupiterIndexCount_/3<<" indexed triangles / seamless loop\n";
}
void Renderer::clearJupiter() {
    gl_.DeleteTextures(1,&jupiterSource_); gl_.DeleteTextures(1,&jupiterWind_); gl_.DeleteTextures(1,&jupiterNoise_);
    gl_.DeleteBuffers(1,&jupiterVbo_); gl_.DeleteBuffers(1,&jupiterIbo_); gl_.DeleteVertexArrays(1,&jupiterVao_); gl_.DeleteProgram(jupiterProgram_);
    jupiterSource_=jupiterWind_=jupiterNoise_=jupiterVbo_=jupiterIbo_=jupiterVao_=jupiterProgram_=0;
    jupiterSize_=0; jupiterIndexCount_=0;
}

void Renderer::prepareJupiter(const jupiter::Clock& clock,bool lowDetail) {
    if(!jupiterError_.empty()&&clock.revision!=jupiterRevision_) jupiterAttempted_=false;
    if(!jupiterAttempted_) {
        jupiterAttempted_=true;
        try { initJupiter(lowDetail); jupiterError_.clear(); }
        catch(const std::exception& e) {
            clearJupiter(); jupiterError_=e.what();
            SDL_Log("Jupiter unavailable: %s",e.what());
            while(gl_.GetError()!=GL_NO_ERROR) {}
        }
    }
    jupiterRevision_=clock.revision;
    jupiterRotation_=float(clock.rotation()*radians);
    jupiterPhase_=float(clock.loopFraction()*2*pi);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
}
void Renderer::drawJupiter(const Camera& camera,Vec3 centre,float radius,float spin) {
    if(!jupiterAvailable()) return;
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(jupiterProgram_); gl_.BindVertexArray(jupiterVao_);
    auto vec=[&](const char* name,Vec3 value) { gl_.Uniform3f(gl_.GetUniformLocation(jupiterProgram_,name),value.x,value.y,value.z); };
    auto scalar=[&](const char* name,float value) { gl_.Uniform1f(gl_.GetUniformLocation(jupiterProgram_,name),value); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    vec("eyeLocal",(camera.position()-centre)*(1/radius));
    vec("lightDirection",normalized(camera.backward()*.65f+camera.up()*.55f-camera.right()*.55f));
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    scalar("spin",spin); scalar("phase",jupiterPhase_);
    for(int i=0;i<2;++i) {
        gl_.ActiveTexture(GL_TEXTURE0+i); gl_.BindTexture(GL_TEXTURE_2D,i==0?jupiterSource_:jupiterWind_);
        gl_.Uniform1i(gl_.GetUniformLocation(jupiterProgram_,i==0?"cloudMap":"windProfile"),i);
    }
    gl_.ActiveTexture(GL_TEXTURE2); gl_.BindTexture(GL_TEXTURE_3D,jupiterNoise_);
    gl_.Uniform1i(gl_.GetUniformLocation(jupiterProgram_,"noiseVolume"),2);
    gl_.DrawElements(GL_TRIANGLES,jupiterIndexCount_,GL_UNSIGNED_INT,nullptr);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::jupiterGlobe(const jupiter::View& view) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    spaceBackground(view.pose());
    drawJupiter(view.pose(),{},1,0);
}
void Renderer::jupiterHud(const jupiter::View& view,float mx,float my,float fps,bool fullscreen,bool globe) {
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f); border(r,line,.45f); };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:.82f);
        if(r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    char buf[160];
    panel(layout_.jupiterPanel());
    button(layout_.jupiterPause(),view.clock.paused?"SPACE / PLAY":"SPACE / PAUSE",view.clock.paused);
    button(layout_.jupiterSlower(),"-"); button(layout_.jupiterFaster(),"+");
    std::snprintf(buf,sizeof(buf),"S / TIME %.0fX",jupiter::rates[view.clock.speed]); text(layout_.width()-187,159,buf,accent);
    const std::array<const char*,jupiter::rates.size()> scales{{"1 SEC = 1 MODEL SECOND","1 SEC = 1 MODEL MINUTE","1 SEC = 10 MODEL MINUTES","1 SEC = 1 MODEL HOUR"}};
    text(layout_.width()-216,184,scales[view.clock.speed],muted);
    if(globe) {
        button(layout_.jupiterFrame(),view.frame==jupiter::Frame::Rotation?"V / WATCH ROTATION":view.frame==jupiter::Frame::Spot?"V / TRACKING RED SPOT":"V / FOLLOW ROTATION");
    } else {
        button(layout_.jupiterFrame(),"F / FRAME WHOLE SYSTEM");
        button(layout_.jupiterApproach(),"A / APPROACH SELECTED");
    }
    button(layout_.jupiterReset(),jupiterAvailable()?"R / RESET ATMOSPHERE":"R / RETRY REFERENCE");
    int loopY=layout_.jupiterReset().y+36;
    std::snprintf(buf,sizeof(buf),"LOOP %05.1f%% / 10 ROTATIONS",view.clock.loopFraction()*100); text(layout_.width()-216,loopY,buf,accent);
    text(layout_.width()-216,loopY+14,"STYLIZED / SEAMLESS LOOP",muted);
    if(globe) {
        panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"JUPITER",paper,3);
        panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / ATMOSPHERIC REGIONS",muted);
        for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+jupiter::features[i].name,view.selected==i);
        button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN"); button(layout_.homeButton(),"HOME / RESET");
        auto sc=layout_.scalePanel(); panel(sc);
        bool fit=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
        std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fit?" FIT":"");
        text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
        auto pose=view.pose(); auto hit=layout_.overHud(mx,my)?std::nullopt:pick(mx,my,pose);
        auto where=hit.value_or(Location{pose.lat,pose.lon}); double latitude=jupiter::graphicLatitude(where.lat);
        auto r=layout_.readout(); panel(r); text(r.x+10,r.y+9,hit?"02 / CLOUD COORDINATE":"02 / VIEW CENTRE",accent);
        std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(latitude),latitude>=0?'N':'S',where.lon); text(r.x+10,r.y+26,buf,paper,2);
        text(r.x+10,r.y+52,"PLANETOGRAPHIC / SYSTEM III",muted);
        if(!jupiterAvailable()) text(r.x+10,r.y+67,"REFERENCE UNAVAILABLE / SEE TERMINAL",accent);
        else text(r.x+10,r.y+67,std::abs(latitude)>79.9?"ILLUSTRATIVE POLAR CLOUDS":"ARCHIVE + MODEL / NO ELEVATION",muted);
        rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.85f);
        text(20,layout_.height()-13,"DRAG / ORBIT   WHEEL / ZOOM   1-4 / REGION   SPACE / PAUSE   S / TIME",muted);
        std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    }
    flushUi();
}
}
