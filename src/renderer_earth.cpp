#include "renderer.hpp"
#include "earth_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
void Renderer::initEarth() {
    const char* executable=SDL_GetBasePath(); std::filesystem::path base=executable?executable:".";
    std::filesystem::path path;
    for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"})
        if(std::filesystem::exists(root/"earth2004.rgb")) { path=root/"earth2004.rgb"; break; }
    if(path.empty()) throw std::runtime_error("Missing assets/earth2004.rgb; see docs/EARTH.md");
    auto source=earth::Image::load(path);
    auto cloud=earth::Image::loadClouds(path.parent_path()/"earth-clouds.gray");
    earthField_=earth::HeightField::load(path.parent_path()/"earth2022.dem");
    earthProgram_=program(earth::vertex,earth::fragment);
    gl_.ActiveTexture(GL_TEXTURE0); gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
    gl_.GenTextures(1,&earthCloudMap_); gl_.BindTexture(GL_TEXTURE_2D,earthCloudMap_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_R8,cloud.width,cloud.height,0,GL_RED,GL_UNSIGNED_BYTE,cloud.rgb.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl_.GenerateMipmap(GL_TEXTURE_2D);
    auto noise=earth::cloudNoise();
    gl_.GenTextures(1,&earthNoise_); gl_.BindTexture(GL_TEXTURE_3D,earthNoise_);
    gl_.TexImage3D(GL_TEXTURE_3D,0,GL_R8,128,128,128,0,GL_RED,GL_UNSIGNED_BYTE,noise.data());
    gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    for(auto axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}) gl_.TexParameteri(GL_TEXTURE_3D,axis,GL_REPEAT);
    gl_.GenerateMipmap(GL_TEXTURE_3D);
    auto uploadMap=[&](GLuint& texture,int internal,int width,int height,GLenum format,GLenum type,const void* pixels) {
        gl_.GenTextures(1,&texture); gl_.BindTexture(GL_TEXTURE_2D,texture);
        gl_.TexImage2D(GL_TEXTURE_2D,0,internal,width,height,0,format,type,pixels);
        gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        gl_.GenerateMipmap(GL_TEXTURE_2D);
    };
    uploadMap(earthSurface_,GL_RGB8,source.width,source.height,GL_RGB,GL_UNSIGNED_BYTE,source.rgb.data());
    std::vector<float> heights(earthField_.meters.begin(),earthField_.meters.end());
    uploadMap(earthElevation_,GL_R32F,earth::HeightField::width,earth::HeightField::height,GL_RED,GL_FLOAT,heights.data());
    // Match the source grid: shared, indexed vertices preserve the dateline and poles.
    auto mesh=earth::makeGlobe(earthField_);
    gl_.GenVertexArrays(1,&earthVao_); gl_.BindVertexArray(earthVao_);
    gl_.GenBuffers(1,&earthVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,earthVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.vertices.size()*sizeof(TerrainVertex)),mesh.vertices.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),nullptr);
    gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),reinterpret_cast<void*>(offsetof(TerrainVertex,meters)));
    gl_.GenBuffers(1,&earthIbo_); gl_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,earthIbo_);
    gl_.BufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(mesh.indices.size()*sizeof(std::uint32_t)),mesh.indices.data(),GL_STATIC_DRAW);
    earthVertexCount_=GLsizei(mesh.indices.size());
    auto shell=makeSampledGlobe(earth::cloudColumns,[](double,double) { return 0.f; });
    gl_.GenVertexArrays(1,&earthCloudVao_); gl_.BindVertexArray(earthCloudVao_);
    gl_.GenBuffers(1,&earthCloudVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,earthCloudVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(shell.vertices.size()*sizeof(TerrainVertex)),shell.vertices.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TerrainVertex),nullptr);
    gl_.GenBuffers(1,&earthCloudIbo_); gl_.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,earthCloudIbo_);
    gl_.BufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(shell.indices.size()*sizeof(std::uint32_t)),shell.indices.data(),GL_STATIC_DRAW);
    earthCloudIndexCount_=GLsizei(shell.indices.size()); check();
    std::cout<<"Earth: continuous Blue Marble / native NOAA relief / layered GEOS-5 clouds / "<<earthVertexCount_/3<<" triangles / 60 s cloud loop\n";
}
void Renderer::clearEarth() {
    gl_.DeleteVertexArrays(1,&earthCloudVao_); gl_.DeleteBuffers(1,&earthCloudVbo_); gl_.DeleteBuffers(1,&earthCloudIbo_);
    earthCloudVao_=earthCloudVbo_=earthCloudIbo_=0; earthCloudIndexCount_=0;
    gl_.DeleteTextures(1,&earthSurface_); gl_.DeleteTextures(1,&earthElevation_);
    gl_.DeleteBuffers(1,&earthIbo_); gl_.DeleteTextures(1,&earthCloudMap_); gl_.DeleteTextures(1,&earthNoise_);
    gl_.DeleteBuffers(1,&earthVbo_); gl_.DeleteVertexArrays(1,&earthVao_); gl_.DeleteProgram(earthProgram_);
    earthSurface_=earthElevation_=earthIbo_=earthCloudMap_=earthNoise_=earthVbo_=earthVao_=earthProgram_=0; earthVertexCount_=0; earthField_={};
}
void Renderer::prepareEarth(const earth::View& view) {
    const auto& clouds=view.clouds;
    if(!earthError_.empty()&&clouds.revision!=earthRevision_) earthAttempted_=false;
    earthRevision_=clouds.revision;
    if(!earthAttempted_) {
        earthAttempted_=true;
        try { initEarth(); earthError_.clear(); }
        catch(const std::exception& e) { clearEarth(); earthError_=e.what(); SDL_Log("Earth unavailable: %s",e.what()); while(gl_.GetError()!=GL_NO_ERROR) {} }
    }
    earthPhase_=clouds.phase(); earthClouds_=clouds.visible&&!view.topography;
    earthTopography_=view.topography; earthExaggeration_=view.exaggeration;
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
}
void Renderer::drawEarth(const Camera& camera,Vec3 centre,float radius,bool physicalScale) {
    if(!earthAvailable()) return;
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(earthProgram_); gl_.BindVertexArray(earthVao_);
    auto vec=[&](const char* name,Vec3 value) { gl_.Uniform3f(gl_.GetUniformLocation(earthProgram_,name),value.x,value.y,value.z); };
    auto scalar=[&](const char* name,float value) { gl_.Uniform1f(gl_.GetUniformLocation(earthProgram_,name),value); };
    auto integer=[&](const char* name,int value) { gl_.Uniform1i(gl_.GetUniformLocation(earthProgram_,name),value); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    vec("eyeLocal",(camera.position()-centre)*(1/radius)); vec("loopPhase",earthPhase_);
    vec("lightDirection",normalized(camera.backward()*.80f+camera.up()*.5f-camera.right()*.5f));
    float relief=physicalScale?1:earthExaggeration_;
    scalar("exaggeration",relief); integer("topography",earthTopography_?1:0);
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    gl_.ActiveTexture(GL_TEXTURE1); gl_.BindTexture(GL_TEXTURE_3D,earthNoise_); integer("noiseVolume",1);
    gl_.ActiveTexture(GL_TEXTURE2); gl_.BindTexture(GL_TEXTURE_2D,earthCloudMap_); integer("cloudMap",2);
    gl_.ActiveTexture(GL_TEXTURE3); gl_.BindTexture(GL_TEXTURE_2D,earthSurface_); integer("surfaceMap",3);
    gl_.ActiveTexture(GL_TEXTURE4); gl_.BindTexture(GL_TEXTURE_2D,earthElevation_); integer("elevationMap",4);
    integer("cloudsVisible",earthClouds_?1:0); integer("cloudPass",0); scalar("shell",1);
    gl_.DrawElements(GL_TRIANGLES,earthVertexCount_,GL_UNSIGNED_INT,nullptr);
    if(earthClouds_) {
        integer("cloudPass",1); scalar("shell",earth::cloudShellRadius(relief,0));
        gl_.Enable(GL_BLEND); gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        gl_.DepthMask(GL_FALSE); // Cloud transparency must not move the surface coordinate probe.
        // A dedicated shell avoids coarse overview-sphere chords cutting into Earth.
        gl_.BindVertexArray(earthCloudVao_);
        gl_.DrawElements(GL_TRIANGLES,earthCloudIndexCount_,GL_UNSIGNED_INT,nullptr);
        integer("cloudPass",2); scalar("shell",earth::cloudShellRadius(relief,1));
        gl_.DrawElements(GL_TRIANGLES,earthCloudIndexCount_,GL_UNSIGNED_INT,nullptr);
        gl_.DepthMask(GL_TRUE); gl_.Disable(GL_BLEND);
    }
    gl_.ActiveTexture(GL_TEXTURE0); gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::earthGlobe(const earth::View& view) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    spaceBackground(camera);
    drawEarth(camera,{},1);
}
void Renderer::earthHud(const earth::View& view,float mx,float my,float fps,bool fullscreen,bool globe) {
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f); border(r,line,.45f); };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:.82f);
        if(r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    char buf[160]; auto p=layout_.earthPanel(); panel(p);
    button(layout_.earthToggle(),view.topography?"CLOUDS HIDDEN / TOPOGRAPHY":view.clouds.visible?"C / CLOUDS ON":"C / CLOUDS OFF",view.clouds.visible&&!view.topography);
    button(layout_.earthMap(),view.topography?"T / OCEAN-FREE TOPOGRAPHY":"T / NATURAL COLOR",view.topography);
    button(layout_.earthRelief(),!globe?"RELIEF / 1X SYSTEM SCALE":view.exaggeration==10?"E / RELIEF 10X":"E / RELIEF 1X");
    button(layout_.earthPause(),view.clouds.paused?"SPACE / RESUME CLOUDS":"SPACE / PAUSE CLOUDS",view.clouds.paused);
    button(layout_.earthReset(),earthAvailable()?"R / RESTART LOOP":"R / RETRY MAP");
    text(p.x+10,p.y+160,"LAYERED CLOUDS / 60 SEC LOOP",accent);
    rect(float(p.x+10),float(p.y+177),186,2,line,.5f);
    rect(float(p.x+10),float(p.y+177),float(186*view.clouds.fraction()),2,accent);
    text(p.x+10,p.y+190,"ILLUSTRATIVE / NOT LIVE WEATHER",muted);
    if(globe) {
        panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"EARTH",paper,3);
        panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / GEOGRAPHIC LANDMARKS",muted);
        for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+earth::landmarks[i].name,view.selected==i);
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
        float h=earthField_.sample(where.lat,where.lon);
        if(earthAvailable()) {
            std::snprintf(buf,sizeof(buf),"ETOPO 2022 / %+.0f M %s",double(h),h<0?"SEAFLOOR":"SURFACE");
            text(r.x+10,r.y+52,buf,paper);
            text(r.x+10,r.y+67,"EGM2008 / 9 ARCMIN / INTERPOLATED",muted);
        } else text(r.x+10,r.y+52,"EARTH DATA UNAVAILABLE / SEE TERMINAL",muted);
        auto legend=layout_.legend(); panel(legend);
        text(legend.x+10,legend.y+10,view.topography?"ELEVATION / METRES":"BLUE MARBLE / JULY 2004",accent);
        if(view.topography) {
            const Vec3 colors[]={{.075f,.11f,.22f},{.16f,.34f,.46f},{.60f,.79f,.75f},{.26f,.46f,.27f},
                {.51f,.61f,.33f},{.77f,.67f,.43f},{.57f,.39f,.29f},{.97f,.95f,.88f}};
            for(int i=0;i<8;++i) rect(float(legend.x+10+i*23),float(legend.y+27),23,7,colors[i]);
            text(legend.x+10,legend.y+42,"-11000    0          +8000",muted);
            text(legend.x+10,legend.y+61,"OCEANS REMOVED / 1 KM CONTOURS",muted);
        } else {
            text(legend.x+10,legend.y+31,"NASA COLOR / NOAA TERRAIN",muted);
            text(legend.x+10,legend.y+47,"T / EXPLORE TOPOGRAPHY",paper);
            text(legend.x+10,legend.y+63,"REGIONAL DETAIL / NOT STREET LEVEL",muted);
        }
        rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.85f);
        text(20,layout_.height()-13,"DRAG / ORBIT   WHEEL / ZOOM   T / TOPOGRAPHY   C / CLOUDS   E / RELIEF",muted);
        std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    }
    flushUi();
}
}
