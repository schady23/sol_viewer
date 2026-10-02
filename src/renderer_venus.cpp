#include "renderer.hpp"
#include "venus_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
void Renderer::initVenus() {
    venusProgram_=program(venus::vertex,venus::fragment);
    auto noise=earth::cloudNoise();
    gl_.ActiveTexture(GL_TEXTURE0); gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
    gl_.GenTextures(1,&venusNoise_); gl_.BindTexture(GL_TEXTURE_3D,venusNoise_);
    gl_.TexImage3D(GL_TEXTURE_3D,0,GL_R8,128,128,128,0,GL_RED,GL_UNSIGNED_BYTE,noise.data());
    gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    for(auto axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}) gl_.TexParameteri(GL_TEXTURE_3D,axis,GL_REPEAT);
    gl_.GenerateMipmap(GL_TEXTURE_3D);
    // Clouds are independent of the surface assets. Defer the 240 MiB terrain
    // upload until the surface is requested, and keep clouds usable on failure.
    auto shell=makeSampledGlobe(512,[](double,double) { return 0.f; });
    gl_.GenVertexArrays(1,&venusCloudVao_); gl_.BindVertexArray(venusCloudVao_);
    gl_.GenBuffers(1,&venusCloudVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,venusCloudVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(shell.vertices.size()*sizeof(TerrainVertex)),shell.vertices.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),nullptr);
    gl_.GenBuffers(1,&venusCloudIbo_); gl_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,venusCloudIbo_);
    gl_.BufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(shell.indices.size()*sizeof(std::uint32_t)),shell.indices.data(),GL_STATIC_DRAW);
    venusCloudIndexCount_=GLsizei(shell.indices.size());
    check();
    std::cout<<"Venus: "<<venusCloudIndexCount_/3<<" cloud triangles / 60 s cloud loop\n";
}
void Renderer::initVenusSurface() {
    const char* executable=SDL_GetBasePath(); std::filesystem::path base=executable?executable:".";
    std::filesystem::path path;
    for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"})
        if(std::filesystem::exists(root/"venus-magellan.dem")) { path=root/"venus-magellan.dem"; break; }
    if(path.empty()) throw std::runtime_error("Missing assets/venus-magellan.dem; see docs/VENUS.md");
    venusField_=venus::HeightField::load(path);
    auto surface=venus::SurfaceMap::load(path.parent_path()/"venus-surface.rgb");
    gl_.ActiveTexture(GL_TEXTURE0); gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
    gl_.GenTextures(1,&venusSurface_); gl_.BindTexture(GL_TEXTURE_2D,venusSurface_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGB8,surface.width,surface.height,0,GL_RGB,GL_UNSIGNED_BYTE,surface.rgb.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl_.GenerateMipmap(GL_TEXTURE_2D);
    auto mesh=venus::makeFacetedGlobe(venusField_);
    gl_.GenVertexArrays(1,&venusVao_); gl_.BindVertexArray(venusVao_);
    gl_.GenBuffers(1,&venusVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,venusVbo_);
    using Vertex=venus::FacetVertex;
    static_assert(sizeof(Vertex)==20,"Venus vertex packing changed");
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,meters)));
    gl_.EnableVertexAttribArray(2); gl_.VertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,faceMeters)));
    venusVertexCount_=GLsizei(mesh.size()); check();
    std::cout<<"Venus: Magellan topography / JPL inferred surface color / "<<venusVertexCount_/3<<" terrain triangles\n";
}
void Renderer::clearVenusSurface() {
    gl_.DeleteTextures(1,&venusSurface_);
    gl_.DeleteBuffers(1,&venusVbo_); gl_.DeleteVertexArrays(1,&venusVao_);
    venusSurface_=venusVbo_=venusVao_=0; venusVertexCount_=0; venusField_={};
}
void Renderer::clearVenus() {
    clearVenusSurface();
    gl_.DeleteTextures(1,&venusNoise_);
    gl_.DeleteBuffers(1,&venusCloudVbo_); gl_.DeleteBuffers(1,&venusCloudIbo_); gl_.DeleteVertexArrays(1,&venusCloudVao_);
    gl_.DeleteProgram(venusProgram_); venusNoise_=venusProgram_=0;
    venusCloudVbo_=venusCloudIbo_=venusCloudVao_=0; venusCloudIndexCount_=0;
}
void Renderer::prepareVenus(const venus::View& view) {
    const auto& clouds=view.clouds;
    if(clouds.revision!=venusRevision_) {
        if(!venusError_.empty()) venusAttempted_=false;
        if(!venusSurfaceError_.empty()) venusSurfaceAttempted_=false;
    }
    venusRevision_=clouds.revision;
    if(!venusAttempted_) {
        venusAttempted_=true;
        try { initVenus(); venusError_.clear(); }
        catch(const std::exception& e) { clearVenus(); venusError_=e.what(); SDL_Log("Venus unavailable: %s",e.what()); while(gl_.GetError()!=GL_NO_ERROR) {} }
    }
    if(view.topography&&venusError_.empty()&&!venusSurfaceAttempted_) {
        venusSurfaceAttempted_=true;
        try { initVenusSurface(); venusSurfaceError_.clear(); }
        catch(const std::exception& e) { clearVenusSurface(); venusSurfaceError_=e.what(); SDL_Log("Venus surface unavailable: %s",e.what()); while(gl_.GetError()!=GL_NO_ERROR) {} }
    }
    venusPhase_=clouds.phase();
    venusTopography_=view.topography; venusExaggeration_=view.exaggeration;
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
}
void Renderer::drawVenus(const Camera& camera,Vec3 centre,float radius,bool physicalScale) {
    if(!venusAvailable()) return;
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(venusProgram_); gl_.BindVertexArray(venusTopography_?venusVao_:venusCloudVao_);
    auto vec=[&](const char* name,Vec3 value) { gl_.Uniform3f(gl_.GetUniformLocation(venusProgram_,name),value.x,value.y,value.z); };
    auto scalar=[&](const char* name,float value) { gl_.Uniform1f(gl_.GetUniformLocation(venusProgram_,name),value); };
    auto integer=[&](const char* name,int value) { gl_.Uniform1i(gl_.GetUniformLocation(venusProgram_,name),value); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    vec("eyeLocal",(camera.position()-centre)*(1/radius)); vec("loopPhase",venusPhase_);
    vec("lightDirection",normalized(camera.backward()*.80f+camera.up()*.5f-camera.right()*.5f));
    float relief=physicalScale?1:venusExaggeration_;
    scalar("exaggeration",relief);
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    gl_.ActiveTexture(GL_TEXTURE1); gl_.BindTexture(GL_TEXTURE_3D,venusNoise_); integer("noiseVolume",1);
    gl_.ActiveTexture(GL_TEXTURE2); gl_.BindTexture(GL_TEXTURE_2D,venusSurface_); integer("surfaceMap",2);
    integer("cloudPass",venusTopography_?0:1);
    scalar("shell",1+70000/venus::radiusMeters);
    if(venusTopography_) gl_.DrawArrays(GL_TRIANGLES,0,venusVertexCount_);
    else gl_.DrawElements(GL_TRIANGLES,venusCloudIndexCount_,GL_UNSIGNED_INT,nullptr);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::venusGlobe(const venus::View& view) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    spaceBackground(camera);
    drawVenus(camera,{},1);
}
void Renderer::venusHud(const venus::View& view,float mx,float my,float fps,bool fullscreen,bool globe) {
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f); border(r,line,.45f); };
    auto button=[&](Rect r,const std::string& label,bool active=false,bool enabled=true) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:.82f);
        if(enabled&&r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,!enabled?muted:active?ink:paper,enabled);
    };
    char buf[160]; auto p=layout_.venusPanel(); panel(p);
    button(layout_.venusMap(),view.topography?"T / SURFACE":"T / HAZY CLOUDS",view.topography);
    button(layout_.venusRelief(),!globe?"RELIEF / 1X SYSTEM SCALE":!view.topography?"RELIEF / SURFACE MODE ONLY":view.exaggeration==10?"E / RELIEF 10X":"E / RELIEF 1X",false,globe&&view.topography);
    button(layout_.venusPause(),view.topography?"CLOUDS / HELD IN MAP VIEW":view.clouds.paused?"SPACE / RESUME CLOUDS":"SPACE / PAUSE CLOUDS",!view.topography&&view.clouds.paused,!view.topography);
    button(layout_.venusReset(),venusAvailable()?"R / RESTART LOOP":"R / RETRY MAP");
    if(!venusAvailable()) {
        text(p.x+10,p.y+126,view.topography?"SURFACE DATA UNAVAILABLE":"CLOUD RENDERER UNAVAILABLE",accent);
        text(p.x+10,p.y+142,view.topography?"R / RETRY   T / CLOUDS":"R / RETRY   SEE TERMINAL",muted);
    }
    text(p.x+10,p.y+160,"CLOUD MOTION / 60 SECOND LOOP",accent);
    rect(float(p.x+10),float(p.y+177),186,2,line,.5f);
    rect(float(p.x+10),float(p.y+177),float(186*view.clouds.fraction()),2,accent);
    text(p.x+10,p.y+190,"ILLUSTRATIVE / NOT LIVE WEATHER",muted);
    if(globe) {
        panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"VENUS",paper,3);
        panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / REGIONAL VIEWS",muted);
        for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+venus::regions[i].name,view.selected==i);
        button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN"); button(layout_.homeButton(),"HOME / RESET");
        auto sc=layout_.scalePanel(); panel(sc);
        bool fit=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
        std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fit?" FIT":"");
        text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
        auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
        auto hit=layout_.overHud(mx,my)?std::nullopt:pick(mx,my,camera);
        auto where=hit.value_or(Location{view.camera.lat,view.camera.lon}); double lon=std::remainder(where.lon,360.);
        auto r=layout_.readout(); panel(r); text(r.x+10,r.y+9,hit?"02 / MAP COORDINATE":"02 / VIEW CENTRE",accent);
        std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f %c",std::abs(where.lat),where.lat>=0?'N':'S',std::abs(lon),lon>=0?'E':'W');
        text(r.x+10,r.y+26,buf,paper,2);
        float h=view.topography?venusField_.sample(where.lat,where.lon):0;
        if(venusAvailable()&&view.topography&&std::isfinite(h)) {
            std::snprintf(buf,sizeof(buf),"MAGELLAN / %+.0f M",double(h));
            text(r.x+10,r.y+52,buf,paper);
            text(r.x+10,r.y+67,"6051 KM DATUM / INTERPOLATED",muted);
        } else {
            text(r.x+10,r.y+52,!venusAvailable()?"DATA UNAVAILABLE / R TO RETRY":!view.topography?"OPAQUE CLOUDS / SURFACE HIDDEN":"NO ALTIMETRY COVERAGE",muted);
        }
        auto legend=layout_.legend(); panel(legend);
        text(legend.x+10,legend.y+10,view.topography?"SURFACE / INFERRED COLOR":"VENUS / VISIBLE-LIGHT HAZE",accent);
        if(view.topography) {
            text(legend.x+10,legend.y+29,"RADAR / SIMULATED ROCK HUES",paper);
            text(legend.x+10,legend.y+45,"HEIGHTS / MAGELLAN ALTIMETRY",muted);
            text(legend.x+10,legend.y+63,"GRAY / NO ALTIMETRY COVERAGE",muted);
        } else {
            text(legend.x+10,legend.y+31,"SOFT CLOUD-TOP HAZE",muted);
            text(legend.x+10,legend.y+47,"T / EXPLORE SURFACE",paper);
            text(legend.x+10,legend.y+63,"TERRAIN / MAGELLAN ALTIMETRY",muted);
        }
        rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.85f);
        text(20,layout_.height()-13,"DRAG / ORBIT   WHEEL / ZOOM   T / SURFACE   C / MAP   E / RELIEF",muted);
        std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    }
    flushUi();
}
}
