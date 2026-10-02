#include "renderer.hpp"
#include "io_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
void Renderer::initIo() {
    const char* executable=SDL_GetBasePath(); std::filesystem::path base=executable?executable:".";
    std::filesystem::path path;
    for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"})
        if(std::filesystem::exists(root/"io-voyager-galileo.rgb")) { path=root/"io-voyager-galileo.rgb"; break; }
    if(path.empty()) throw std::runtime_error("Missing assets/io-voyager-galileo.rgb; see docs/IO.md");
    auto surface=io::Image::load(path);
    ioProgram_=program(io::vertex,io::fragment);
    auto mesh=io::makeFacetedGlobe(surface);
    gl_.GenVertexArrays(1,&ioVao_); gl_.BindVertexArray(ioVao_);
    gl_.GenBuffers(1,&ioVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,ioVbo_);
    using Vertex=io::FacetVertex;
    static_assert(sizeof(Vertex)==16,"Io facet packing changed");
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,color)));
    ioVertexCount_=GLsizei(mesh.size()); check();
    std::cout<<"Io: USGS Voyager / Galileo color mosaic / 4096 x 2048 / "<<ioVertexCount_/3<<" flat-shaded triangles / no elevation model\n";
}
void Renderer::clearIo() {
    gl_.DeleteBuffers(1,&ioVbo_);
    gl_.DeleteVertexArrays(1,&ioVao_); gl_.DeleteProgram(ioProgram_);
    ioVbo_=ioVao_=ioProgram_=0; ioVertexCount_=0;
}
void Renderer::prepareIo(const io::View& view) {
    if(!ioError_.empty()&&view.revision!=ioRevision_) ioAttempted_=false;
    ioRevision_=view.revision;
    if(!ioAttempted_) {
        ioAttempted_=true;
        try { initIo(); ioError_.clear(); }
        catch(const std::exception& e) { clearIo(); ioError_=e.what(); SDL_Log("Io unavailable: %s",e.what()); while(gl_.GetError()!=GL_NO_ERROR) {} }
    }
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
}
void Renderer::drawIo(const Camera& camera,Vec3 centre,float radius,float spin,bool lighting) {
    if(!ioAvailable()) return;
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(ioProgram_); gl_.BindVertexArray(ioVao_);
    auto vec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(ioProgram_,name),v.x,v.y,v.z); };
    auto scalar=[&](const char* name,float v) { gl_.Uniform1f(gl_.GetUniformLocation(ioProgram_,name),v); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    vec("lightDirection",normalized(camera.backward()*.8f+camera.up()*.5f-camera.right()*.5f));
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight); scalar("spin",spin);
    gl_.Uniform1i(gl_.GetUniformLocation(ioProgram_,"lighting"),lighting?1:0);
    gl_.DrawArrays(GL_TRIANGLES,0,ioVertexCount_);
    gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::ioGlobe(const io::View& view) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    spaceBackground(camera);
    drawIo(camera,{},1,0,view.lighting);
}
void Renderer::ioHud(const io::View& view,float mx,float my,float fps,bool fullscreen) {
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f); border(r,line,.45f); };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:.82f);
        if(r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"IO",paper,3);
    panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / VOLCANIC REGIONS",muted);
    for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+io::regions[i].name,view.selected==i);
    button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN"); button(layout_.homeButton(),"HOME / RESET");
    auto sc=layout_.scalePanel(); panel(sc);
    bool fit=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
    char buf[160]; std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fit?" FIT":"");
    text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
    auto p=layout_.ioPanel(); panel(p);
    button(layout_.ioLighting(),view.lighting?"L / ATLAS LIGHTING":"L / UNLIT FACETS",!view.lighting);
    text(p.x+10,p.y+40,"VOYAGER + GALILEO / USGS",accent);
    text(p.x+10,p.y+58,"STYLIZED / TRIANGLE COLORS",muted);
    text(p.x+10,p.y+76,ioAvailable()?"SHAPE / MEAN-RADIUS SPHERE":"MAP UNAVAILABLE / SEE TERMINAL",muted);
    if(!ioAvailable()) button(layout_.ioRetry(),"R / RETRY SURFACE MAP");
    else text(p.x+10,p.y+94,"ARCHIVAL / STATIC VIEW",muted);
    auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    auto hit=layout_.overHud(mx,my)?std::nullopt:pick(mx,my,camera);
    auto where=hit.value_or(Location{view.camera.lat,view.camera.lon});
    auto r=layout_.readout(); panel(r); text(r.x+10,r.y+9,hit?"02 / MAP COORDINATE":"02 / VIEW CENTRE",accent);
    std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(where.lat),where.lat>=0?'N':'S',where.lon); text(r.x+10,r.y+26,buf,paper,2);
    text(r.x+10,r.y+52,"PLANETOCENTRIC / EAST LONGITUDE",muted);
    text(r.x+10,r.y+67,!ioAvailable()?"MAP UNAVAILABLE / R TO RETRY":std::abs(where.lat)>85?"POLAR COLOR / SOURCE INTERPOLATION":"IMAGE MAP / NO ELEVATION DATA",muted);
    auto legend=layout_.legend(); panel(legend);
    text(legend.x+10,legend.y+10,"IO / VOLCANIC SURFACE",accent);
    text(legend.x+10,legend.y+29,"COLOR / STYLIZED MOSAIC",paper);
    text(legend.x+10,legend.y+45,"MIXED DATES + RESOLUTION",muted);
    text(legend.x+10,legend.y+63,"I / FIELD GUIDE",muted);
    rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.85f);
    text(20,layout_.height()-13,"DRAG / ORBIT   WHEEL / ZOOM   1-4 / REGION   L / LIGHTING",muted);
    std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    flushUi();
}
}
