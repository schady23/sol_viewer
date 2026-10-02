#include "renderer.hpp"
#include "saturn_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
void Renderer::initSaturn(bool lowDetail) {
    const char* executable=SDL_GetBasePath(); std::filesystem::path base=executable?executable:".";
    std::filesystem::path path;
    for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"})
        if(std::filesystem::exists(root/"saturn-vtad.rgb")) { path=root/"saturn-vtad.rgb"; break; }
    if(path.empty()) throw std::runtime_error("Missing assets/saturn-vtad.rgb; see docs/SATURN.md");
    auto source=saturn::Image::load(path);
    std::array<float,1024> winds{};
    for(int y=0;y<1024;++y) winds[y]=float(saturn::jetSpeed(90-(y+.5)*180/1024));
    auto rings=saturn::RingImage::load(path.parent_path()/"saturn-rings.rgba");
    std::string cloudFragment=std::string(saturn::globeFragment)+saturn::ringFunctions;
    std::string ringFragment=std::string(saturn::ringFragment)+saturn::ringFunctions;
    saturnProgram_=program(saturn::globeVertex,cloudFragment.c_str());
    saturnRingProgram_=program(saturn::ringVertex,ringFragment.c_str());
    gl_.ActiveTexture(GL_TEXTURE0); gl_.GenTextures(1,&saturnRingMap_); gl_.BindTexture(GL_TEXTURE_2D,saturnRingMap_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,4096,1,0,GL_RGBA,GL_UNSIGNED_BYTE,rings.rgba.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    auto ringMesh=saturn::makeRings();
    gl_.GenVertexArrays(1,&saturnRingVao_); gl_.BindVertexArray(saturnRingVao_);
    gl_.GenBuffers(1,&saturnRingVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,saturnRingVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(ringMesh.size()*sizeof(Vec3)),ringMesh.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3),nullptr);
    saturnRingCount_=GLsizei(ringMesh.size());
    gl_.ActiveTexture(GL_TEXTURE0); gl_.GenTextures(1,&saturnSource_); gl_.BindTexture(GL_TEXTURE_2D,saturnSource_);
    gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGB8,source.width,source.height,0,GL_RGB,GL_UNSIGNED_BYTE,source.rgb.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl_.GenerateMipmap(GL_TEXTURE_2D);
    gl_.GenTextures(1,&saturnWind_); gl_.BindTexture(GL_TEXTURE_2D,saturnWind_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_R32F,1,1024,0,GL_RED,GL_FLOAT,winds.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    // Reusable periodic noise volume. Hardware trilinear filtering makes the
    // multi-scale filament shader substantially cheaper than repeated hash noise.
    std::vector<unsigned char> noise(64*64*64*4); std::uint32_t seed=0x91e10da5u;
    for(auto& value:noise) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; value=static_cast<unsigned char>(seed>>24); }
    gl_.GenTextures(1,&saturnNoise_); gl_.BindTexture(GL_TEXTURE_3D,saturnNoise_);
    gl_.TexImage3D(GL_TEXTURE_3D,0,GL_RGBA8,64,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,noise.data());
    gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    for(auto axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}) gl_.TexParameteri(GL_TEXTURE_3D,axis,GL_REPEAT);
    saturnSize_=lowDetail?256:768;
    auto mesh=makeSampledGlobe(saturnSize_,[](double,double) { return 0.f; });
    gl_.GenVertexArrays(1,&saturnVao_); gl_.BindVertexArray(saturnVao_);
    gl_.GenBuffers(1,&saturnVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,saturnVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.vertices.size()*sizeof(TerrainVertex)),mesh.vertices.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),nullptr);
    gl_.GenBuffers(1,&saturnIbo_); gl_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,saturnIbo_);
    gl_.BufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(mesh.indices.size()*sizeof(std::uint32_t)),mesh.indices.data(),GL_STATIC_DRAW);
    saturnIndexCount_=GLsizei(mesh.indices.size()); check();
    std::cout<<"Saturn: VTAD reference + modeled cloud detail / "<<source.width<<" x "<<source.height<<" visualization texture / "<<saturnIndexCount_/3<<" indexed triangles / seamless loop\n";
}
void Renderer::clearSaturn() {
    gl_.DeleteTextures(1,&saturnRingMap_); gl_.DeleteBuffers(1,&saturnRingVbo_); gl_.DeleteVertexArrays(1,&saturnRingVao_); gl_.DeleteProgram(saturnRingProgram_);
    saturnRingMap_=saturnRingVbo_=saturnRingVao_=saturnRingProgram_=0; saturnRingCount_=0;
    gl_.DeleteTextures(1,&saturnSource_); gl_.DeleteTextures(1,&saturnWind_); gl_.DeleteTextures(1,&saturnNoise_);
    gl_.DeleteBuffers(1,&saturnVbo_); gl_.DeleteBuffers(1,&saturnIbo_); gl_.DeleteVertexArrays(1,&saturnVao_); gl_.DeleteProgram(saturnProgram_);
    saturnSource_=saturnWind_=saturnNoise_=saturnVbo_=saturnIbo_=saturnVao_=saturnProgram_=0;
    saturnSize_=0; saturnIndexCount_=0;
}

void Renderer::prepareSaturn(const saturn::Clock& clock,bool lowDetail) {
    if(!saturnError_.empty()&&clock.revision!=saturnRevision_) saturnAttempted_=false;
    if(!saturnAttempted_) {
        saturnAttempted_=true;
        try { initSaturn(lowDetail); saturnError_.clear(); }
        catch(const std::exception& e) {
            clearSaturn(); saturnError_=e.what();
            SDL_Log("Saturn unavailable: %s",e.what());
            while(gl_.GetError()!=GL_NO_ERROR) {}
        }
    }
    saturnRevision_=clock.revision;
    saturnRotation_=float(clock.rotation()*radians);
    saturnPhase_=float(clock.loopFraction()*2*pi);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
}
void Renderer::drawSaturn(const Camera& camera,Vec3 centre,float radius,float spin) {
    if(!saturnAvailable()) return;
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(saturnProgram_); gl_.BindVertexArray(saturnVao_);
    auto vec=[&](const char* name,Vec3 value) { gl_.Uniform3f(gl_.GetUniformLocation(saturnProgram_,name),value.x,value.y,value.z); };
    auto scalar=[&](const char* name,float value) { gl_.Uniform1f(gl_.GetUniformLocation(saturnProgram_,name),value); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    vec("eyeLocal",(camera.position()-centre)*(1/radius));
    vec("lightDirection",normalized(camera.backward()*.65f+camera.up()*.55f-camera.right()*.55f));
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    scalar("spin",spin); scalar("phase",saturnPhase_);
    for(int i=0;i<2;++i) {
        gl_.ActiveTexture(GL_TEXTURE0+i); gl_.BindTexture(GL_TEXTURE_2D,i==0?saturnSource_:saturnWind_);
        gl_.Uniform1i(gl_.GetUniformLocation(saturnProgram_,i==0?"cloudMap":"windProfile"),i);
    }
    gl_.ActiveTexture(GL_TEXTURE2); gl_.BindTexture(GL_TEXTURE_3D,saturnNoise_);
    gl_.Uniform1i(gl_.GetUniformLocation(saturnProgram_,"noiseVolume"),2);
    gl_.ActiveTexture(GL_TEXTURE3); gl_.BindTexture(GL_TEXTURE_2D,saturnRingMap_);
    gl_.Uniform1i(gl_.GetUniformLocation(saturnProgram_,"ringMap"),3);
    gl_.DrawElements(GL_TRIANGLES,saturnIndexCount_,GL_UNSIGNED_INT,nullptr);
    // The opaque ellipsoid writes depth first. Composite the two-sided ring
    // annulus against it with depth tests but no transparent depth writes.
    gl_.Disable(GL_CULL_FACE); gl_.Enable(GL_BLEND); gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); gl_.DepthMask(GL_FALSE);
    gl_.UseProgram(saturnRingProgram_); gl_.BindVertexArray(saturnRingVao_);
    auto ringVec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(saturnRingProgram_,name),v.x,v.y,v.z); };
    auto ringScalar=[&](const char* name,float v) { gl_.Uniform1f(gl_.GetUniformLocation(saturnRingProgram_,name),v); };
    ringVec("eye",camera.position()); ringVec("rightward",camera.right()); ringVec("upward",camera.up()); ringVec("backward",camera.backward()); ringVec("centre",centre);
    ringVec("eyeLocal",(camera.position()-centre)*(1/radius));
    ringVec("lightDirection",normalized(camera.backward()*.65f+camera.up()*.55f-camera.right()*.55f));
    ringScalar("radius",radius); ringScalar("lens",camera.lens); ringScalar("clipNear",camera.clipNear); ringScalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindTexture(GL_TEXTURE_2D,saturnRingMap_); gl_.Uniform1i(gl_.GetUniformLocation(saturnRingProgram_,"ringMap"),0);
    gl_.DrawArrays(GL_TRIANGLES,0,saturnRingCount_);
    gl_.DepthMask(GL_TRUE); gl_.Disable(GL_BLEND);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::saturnGlobe(const saturn::View& view) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    auto camera=view.pose(); camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    spaceBackground(camera);
    if(!saturnAvailable()) { solar::SceneFrame scene; scene.camera=camera; scene.bodies.push_back({solar::BodyId::Saturn,{},1}); placeholders(scene); }
    else drawSaturn(camera,{},1,0);
}
void Renderer::saturnHud(const saturn::View& view,float mx,float my,float fps,bool fullscreen,bool globe) {
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f); border(r,line,.45f); };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:.82f);
        if(r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    char buf[160];
    panel(layout_.saturnPanel());
    button(layout_.saturnPause(),view.clock.paused?"SPACE / PLAY":"SPACE / PAUSE",view.clock.paused);
    button(layout_.saturnSlower(),"-"); button(layout_.saturnFaster(),"+");
    std::snprintf(buf,sizeof(buf),"S / TIME %.0fX",saturn::rates[view.clock.speed]); text(layout_.width()-187,159,buf,accent);
    const std::array<const char*,saturn::rates.size()> scales{{"1 SEC = 1 MODEL SECOND","1 SEC = 1 MODEL MINUTE","1 SEC = 10 MODEL MINUTES","1 SEC = 1 MODEL HOUR"}};
    text(layout_.width()-216,184,scales[view.clock.speed],muted);
    if(globe) {
        button(layout_.saturnFrame(),view.frame==saturn::Frame::Rotation?"V / WATCH ROTATION":"V / FOLLOW ROTATION");
    }
    button(layout_.saturnReset(),saturnAvailable()?"R / RESET ATMOSPHERE":"R / RETRY REFERENCE");
    int loopY=layout_.saturnReset().y+36;
    std::snprintf(buf,sizeof(buf),"LOOP %05.1f%% / 10 ROTATIONS",view.clock.loopFraction()*100); text(layout_.width()-216,loopY,buf,accent);
    text(layout_.width()-216,loopY+14,"STYLIZED / SEAMLESS LOOP",muted);
    if(globe) {
        panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"SATURN",paper,3);
        panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / ATMOSPHERIC REGIONS",muted);
        for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+saturn::features[i].name,view.selected==i);
        button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN"); button(layout_.homeButton(),"HOME / RESET");
        auto sc=layout_.scalePanel(); panel(sc);
        bool fit=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
        std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fit?" FIT":"");
        text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
        auto pose=view.pose(); pose.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
        auto hit=layout_.overHud(mx,my)?std::nullopt:saturn::pickCloud(mx,my,pose,layout_);
        auto where=hit.value_or(Location{pose.lat,pose.lon}); double latitude=saturn::graphicLatitude(where.lat);
        auto r=layout_.readout(); panel(r); text(r.x+10,r.y+9,hit?"02 / CLOUD COORDINATE":"02 / VIEW CENTRE",accent);
        std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(latitude),latitude>=0?'N':'S',where.lon); text(r.x+10,r.y+26,buf,paper,2);
        text(r.x+10,r.y+52,"MODEL / PLANETOGRAPHIC / EAST",muted);
        if(!saturnAvailable()) text(r.x+10,r.y+67,"REFERENCE UNAVAILABLE / SEE TERMINAL",accent);
        else text(r.x+10,r.y+67,std::abs(latitude)>79.9?"MODELED POLAR HEXAGON":"VISUALIZATION / NO ELEVATION",muted);
        rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.85f);
        text(20,layout_.height()-13,"DRAG / ORBIT   WHEEL / ZOOM   1-4 / REGION   SPACE / PAUSE   S / TIME",muted);
        std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    }
    flushUi();
}
}
