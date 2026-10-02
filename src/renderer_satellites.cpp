#include "renderer.hpp"
#include "outer_shaders.hpp"
#include "belts.hpp"
#include "satellites_shaders.hpp"
#include <cstdio>
#include <iostream>
namespace mars {
bool Renderer::satelliteAvailable(solar::BodyId id) const {
    return satellites::supports(id)&&satellites_.at(satellites::slot(id)).vao!=0;
}
void Renderer::clearSatellite(GpuSatellite& s) {
    gl_.DeleteBuffers(1,&s.vbo); gl_.DeleteVertexArrays(1,&s.vao);
    gl_.DeleteBuffers(1,&s.ringVbo); gl_.DeleteVertexArrays(1,&s.ringVao);
    gl_.DeleteTextures(1,&s.cloudMap); gl_.DeleteTextures(1,&s.noise);
    s.vbo=s.vao=s.ringVbo=s.ringVao=s.cloudMap=s.noise=0; s.count=s.ringCount=0;
}
void Renderer::prepareSatellite(const satellites::View& view) {
    auto& s=satellites_.at(satellites::slot(view.body));
    const auto& d=satellites::definition(view.body);
    s.haze=view.haze;
    if(ice::planet(view.body)) { s.spin=float(view.clock.rotation(view.body)*radians); s.phase=float(view.clock.phase(view.body)); }
    if(!s.error.empty()&&s.revision!=view.revision) s.attempted=false;
    s.revision=view.revision;
    if(!s.attempted) {
        s.attempted=true;
        try {
            const char* executable=SDL_GetBasePath(); std::filesystem::path base=executable?executable:".";
            std::filesystem::path path;
            for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"})
                if(std::filesystem::exists(root/d.asset)) { path=root/d.asset; break; }
            if(path.empty()) throw std::runtime_error(std::string("Missing assets/")+d.asset+"; see docs/BELTS.md, docs/OUTER_WORLDS.md, docs/SATELLITES.md and docs/SATURN_MOONS.md");
            auto mesh=satellites::loadSurface(view.body,path);
            if(ice::planet(view.body)) {
                auto name=view.body==solar::BodyId::Uranus?"uranus-clouds.rgb":"neptune-clouds.rgb";
                auto magic=view.body==solar::BodyId::Uranus?"URANUS_CLOUD_RGB":"NEPTUNE_CLOUD_RGB";
                auto pixels=faceted::loadRgb(path.parent_path()/name,magic,solar::body(view.body).name,2048,1024);
                mesh=satellites::makeCloudShell(view.body);
                gl_.ActiveTexture(GL_TEXTURE0); gl_.GenTextures(1,&s.cloudMap); gl_.BindTexture(GL_TEXTURE_2D,s.cloudMap);
                gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
                gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGB8,2048,1024,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
                gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
                gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
                gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); gl_.GenerateMipmap(GL_TEXTURE_2D);
                std::vector<unsigned char> noise(64*64*64*4); std::uint32_t seed=0x841b22f1u;
                for(auto& v:noise) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; v=static_cast<unsigned char>(seed>>24); }
                gl_.ActiveTexture(GL_TEXTURE1); gl_.GenTextures(1,&s.noise); gl_.BindTexture(GL_TEXTURE_3D,s.noise);
                gl_.TexImage3D(GL_TEXTURE_3D,0,GL_RGBA8,64,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,noise.data());
                gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl_.TexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                for(auto axis:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}) gl_.TexParameteri(GL_TEXTURE_3D,axis,GL_REPEAT);
                gl_.ActiveTexture(GL_TEXTURE0);
            }
            if(!satelliteProgram_) satelliteProgram_=program(satellites::outerVertex,satellites::fragment);
            gl_.GenVertexArrays(1,&s.vao); gl_.BindVertexArray(s.vao);
            gl_.GenBuffers(1,&s.vbo); gl_.BindBuffer(GL_ARRAY_BUFFER,s.vbo);
            using Vertex=faceted::FacetVertex;
            static_assert(sizeof(Vertex)==16,"Satellite facet packing changed");
            gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
            gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
            gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,color)));
            s.count=GLsizei(mesh.size());
            if(ice::planet(view.body)) {
                // One VAO contains the narrow and diffuse rings in source order.
                std::vector<Vertex> rings;
                double eq=ice::equatorialKm(view.body);
                for(const auto& ring:ice::rings(view.body)) for(int j=0;j<2048;++j) {
                    auto a=direction(0,j*360./2048),b=direction(0,(j+1)*360./2048);
                    float inner=float((ring.radiusKm-ring.widthKm/2)/eq),outer=float((ring.radiusKm+ring.widthKm/2)/eq);
                    for(auto p:{a*inner,b*inner,a*outer,b*inner,b*outer,a*outer}) rings.push_back({p,{128,128,128,255}});
                }
                gl_.GenVertexArrays(1,&s.ringVao); gl_.BindVertexArray(s.ringVao);
                gl_.GenBuffers(1,&s.ringVbo); gl_.BindBuffer(GL_ARRAY_BUFFER,s.ringVbo);
                gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(rings.size()*sizeof(Vertex)),rings.data(),GL_STATIC_DRAW);
                gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
                gl_.EnableVertexAttribArray(1); gl_.VertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,color)));
                s.ringCount=GLsizei(rings.size());
            }
            check(); s.error.clear();
            std::cout<<solar::body(view.body).name<<": "<<s.count/3<<" faceted triangles / "<<((view.body==solar::BodyId::Proteus||view.body>=solar::BodyId::Haumea)?"untextured reference shape":d.model?"NASA textured visualization":"USGS archival image atlas")<<'\n';
        } catch(const std::exception& e) {
            clearSatellite(s); s.error=e.what(); SDL_Log("Satellite unavailable: %s",e.what());
            while(gl_.GetError()!=GL_NO_ERROR) {}
        }
    }
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_);
}
void Renderer::drawSatellite(solar::BodyId body,const Camera& camera,Vec3 centre,float radius,float spin,bool lighting) {
    if(!satelliteAvailable(body)) return;
    const auto& surface=satellites_.at(satellites::slot(body));
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.Enable(GL_DEPTH_TEST); gl_.Enable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(satelliteProgram_); gl_.BindVertexArray(surface.vao);
    auto vec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(satelliteProgram_,name),v.x,v.y,v.z); };
    auto scalar=[&](const char* name,float v) { gl_.Uniform1f(gl_.GetUniformLocation(satelliteProgram_,name),v); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    vec("lightDirection",normalized(camera.backward()*.8f+camera.up()*.5f-camera.right()*.5f));
    vec("eyeLocal",(camera.position()-centre)*(1/radius));
    bool giant=ice::planet(body);
    if(giant) spin=surface.spin;
    scalar("tilt",giant?float(ice::tilt(body)*radians):0.f);
    scalar("axisRatio",giant?float(ice::polarKm(body)/ice::equatorialKm(body)):1.f);
    scalar("phase",surface.phase);
    // Explicitly restore bindings after other globe renderers use these units.
    gl_.ActiveTexture(GL_TEXTURE0); gl_.BindTexture(GL_TEXTURE_2D,surface.cloudMap);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"cloudMap"),0);
    gl_.ActiveTexture(GL_TEXTURE1); gl_.BindTexture(GL_TEXTURE_3D,surface.noise);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"cloudNoise"),1);
    gl_.ActiveTexture(GL_TEXTURE0);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"icePlanet"),giant?(body==solar::BodyId::Uranus?1:2):0);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"ringMode"),0);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"titanHaze"),body==solar::BodyId::Titan&&surface.haze?1:0);
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight); scalar("spin",spin);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"lighting"),lighting?1:0);
    gl_.DrawArrays(GL_TRIANGLES,0,surface.count);
    gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::drawOuterRings(solar::BodyId body,const Camera& camera,Vec3 centre,float radius) {
    if(!ice::planet(body)||!satelliteAvailable(body)) return;
    // Draw physical annuli with the same camera uniforms as the cloud shell,
    // with depth tests against all opaque bodies and without depth writes.
    const auto& s=satellites_.at(satellites::slot(body));
    gl_.UseProgram(satelliteProgram_); gl_.BindVertexArray(s.ringVao);
    auto vec=[&](const char* n,Vec3 p) { gl_.Uniform3f(gl_.GetUniformLocation(satelliteProgram_,n),p.x,p.y,p.z); };
    auto scalar=[&](const char* n,float v) { gl_.Uniform1f(gl_.GetUniformLocation(satelliteProgram_,n),v); };
    vec("eye",camera.position()); vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward()); vec("centre",centre);
    scalar("radius",radius); scalar("lens",camera.lens); scalar("clipNear",camera.clipNear); scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight);
    scalar("tilt",float(ice::tilt(body)*radians));
    gl_.Uniform1f(gl_.GetUniformLocation(satelliteProgram_,"spin"),0);
    gl_.Uniform1i(gl_.GetUniformLocation(satelliteProgram_,"ringMode"),1);
    gl_.Enable(GL_DEPTH_TEST); gl_.Disable(GL_CULL_FACE); gl_.Enable(GL_BLEND);
    gl_.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); gl_.DepthMask(GL_FALSE);
    int first=0;
    for(const auto& ring:ice::rings(body)) {
        gl_.Uniform1f(gl_.GetUniformLocation(satelliteProgram_,"ringOpacity"),ring.opacity);
        gl_.DrawArrays(GL_TRIANGLES,first,2048*6); first+=2048*6;
    }
    gl_.DepthMask(GL_TRUE); gl_.Disable(GL_BLEND); gl_.Disable(GL_DEPTH_TEST);
}
void Renderer::satelliteGlobe(const satellites::View& view) {
    auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    if(!satelliteAvailable(view.body)) {
        solar::SceneFrame scene; scene.camera=camera; scene.bodies.push_back({view.body,{},1});
        placeholders(scene); return;
    }
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    spaceBackground(camera);
    drawSatellite(view.body,camera,{},1,0,view.lighting);
    if(ice::planet(view.body)) drawOuterRings(view.body,camera,{},1);
    if(view.body==solar::BodyId::Haumea) drawHaumeaRing(camera,{},1);
}
void Renderer::outerSystemHud(const satellites::View& view,float mx,float my) {
    if(!ice::planet(view.body)) return;
    constexpr Vec3 ink{.028f,.032f,.031f},paper{.92f,.94f,.88f},muted{.66f,.70f,.65f};
    ui_.clear(); auto p=layout_.ioPanel(); rect(float(p.x),float(p.y),float(p.w),float(p.h),ink,.88f);
    auto button=[&](Rect r,const std::string& label) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f);
        if(r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,paper);
    };
    button(layout_.ioLighting(),view.clock.paused?"SPACE / PLAY ATMOSPHERE":"SPACE / PAUSE ATMOSPHERE");
    button(layout_.icePause(),"S / TIME "+std::to_string(int(ice::rates[view.clock.speed]))+"X");
    button(layout_.iceSpeed(),"R / RESET ATMOSPHERE");
    text(p.x+10,p.y+96,"ORBITS / STATIC MEAN LAYOUT",muted);

    flushUi();
}
void Renderer::satelliteHud(const satellites::View& view,float mx,float my,float fps,bool fullscreen) {
    const auto& d=satellites::definition(view.body);
    bool available=satelliteAvailable(view.body),giant=ice::planet(view.body),reference=view.body==solar::BodyId::Proteus||view.body>=solar::BodyId::Haumea;
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.88f); border(r,line,.45f); };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:.82f);
        if(r.contains(mx,my)) border(r,paper,.8f);
        buttonLabel(r.x+8,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,solar::body(view.body).name,paper,3);
    panel(layout_.destinations()); text(29,layout_.destinations().y+7,giant?"01 / ATMOSPHERIC VIEWS":d.model?"01 / MODEL VIEWS":"01 / SURFACE REGIONS",muted);
    for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+d.regions[i].name,view.selected==i);
    button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN"); button(layout_.homeButton(),"HOME / RESET");
    auto sc=layout_.scalePanel(); panel(sc);
    bool fit=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
    char buf[160]; std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fit?" FIT":"");
    text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
    auto p=layout_.ioPanel(); panel(p);
    button(layout_.ioLighting(),view.lighting?"L / LIGHTING":"L / UNLIT",!view.lighting);
    if(giant) {
        button(layout_.icePause(),view.clock.paused?"SPACE / PLAY":"SPACE / PAUSE",view.clock.paused);
        button(layout_.iceSpeed(),"S / TIME "+std::to_string(int(ice::rates[view.clock.speed]))+"X");
        button(layout_.iceReset(),available?"R / RESET ATMOSPHERE":"R / RETRY ATMOSPHERE");
        text(p.x+10,p.y+130,"CLOUD MOTION / ILLUSTRATIVE",muted);
    } else
    if(view.body==solar::BodyId::Titan) {
        button(layout_.titanHaze(),view.haze?"T / REVEAL SURFACE":"T / RESTORE HAZE",!view.haze);
        text(p.x+10,p.y+72,view.haze?"AMBER HAZE / ILLUSTRATIVE":"NASA / SURFACE VISUALIZATION",accent);
        text(p.x+10,p.y+94,"STATIC / NO ELEVATION DATA",muted);
        if(!available) button(layout_.ioRetry(),"R / RETRY SURFACE MAP");
    } else {
    text(p.x+10,p.y+40,reference?"UNTEXTURED / REFERENCE SHAPE":d.model?"NASA / TEXTURED 3D MODEL":"VOYAGER + GALILEO / USGS",accent);
    text(p.x+10,p.y+58,"STYLIZED / TRIANGLE COLORS",muted);
    text(p.x+10,p.y+76,available?reference?(view.body==solar::BodyId::Haumea?"SHAPE / TRIAXIAL ELLIPSOID":"SHAPE / REFERENCE SPHERE"):d.model?"SHAPE / SUPPLIED NASA MESH":"SHAPE / MEAN-RADIUS SPHERE":"MAP UNAVAILABLE / SEE TERMINAL",muted);
    if(!available) button(layout_.ioRetry(),"R / RETRY SURFACE MAP");
    else text(p.x+10,p.y+94,reference&&solar::belts::isDwarf(view.body)?"ESTIMATED / UNVISITED WORLD":"ARCHIVAL / STATIC VIEW",muted);
    }
    auto camera=view.camera; camera.lens*=std::max(1.f,float(layout_.pixelHeight)/layout_.pixelWidth);
    auto hit=d.model||layout_.overHud(mx,my)?std::nullopt:pick(mx,my,camera);
    auto where=hit.value_or(Location{view.camera.lat,view.camera.lon});
    auto r=layout_.readout(); panel(r); text(r.x+10,r.y+9,d.model?"02 / MODEL CAMERA ANGLES":hit?"02 / MAP COORDINATE":"02 / VIEW CENTRE",accent);
    if(d.model) std::snprintf(buf,sizeof(buf),"TILT %+.2f / TURN %.2f",where.lat,where.lon);
    else std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(where.lat),where.lat>=0?'N':'S',where.lon);
    text(r.x+10,r.y+26,buf,paper,2);
    text(r.x+10,r.y+52,d.model?"MODEL AXES / NOT GEOGRAPHIC":"PLANETOCENTRIC / EAST LONGITUDE",muted);
    text(r.x+10,r.y+67,!available?"MAP UNAVAILABLE / R TO RETRY":d.model?"VISUALIZATION / NO HEIGHT READOUT":std::abs(where.lat)>85?"POLAR COLOR / SOURCE INTERPOLATION":"IMAGE MAP / NO ELEVATION DATA",muted);
    auto legend=layout_.legend(); panel(legend);
    text(legend.x+10,legend.y+10,giant?"ICE GIANT / CLOUD TOPS":solar::belts::isDwarf(view.body)?"DWARF PLANET / REFERENCE":reference?"PROTEUS / REFERENCE SPHERE":view.body==solar::BodyId::Titan?"TITAN / HAZE + SURFACE":d.model?"MOON / NASA VISUALIZATION":"ICY MOON / IMAGE ATLAS",accent);
    text(legend.x+10,legend.y+29,giant?"OBLATE GLOBE / FAINT RINGS":reference?"NO GLOBAL IMAGE MAP":view.body==solar::BodyId::Titan&&view.haze?"AMBER HAZE / ILLUSTRATIVE":d.model?"AUTHORED SHAPE + TEXTURE":"TINT / ARTISTIC ICE PALETTE",paper);
    text(legend.x+10,legend.y+45,giant?(view.body==solar::BodyId::Uranus?"97.77 DEG / AXIAL TILT":"28.32 DEG / AXIAL TILT"):reference?"I / SOURCE IMAGE AND LIMITS":view.body==solar::BodyId::Titan&&view.haze?"T / REVEAL SURFACE":d.model?"ORIGINAL MODEL UV MAPPING":"MIXED DATES + RESOLUTION",muted);
    text(legend.x+10,legend.y+63,"I / FIELD GUIDE",muted);
    rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.85f);
    text(20,layout_.height()-13,"DRAG / ORBIT   WHEEL / ZOOM   1-4 / PRESET   L / LIGHTING",muted);
    std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    flushUi();
}
}
