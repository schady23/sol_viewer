#include "renderer.hpp"
#include "ice_giants.hpp"
#include "belts.hpp"
#include <cstdio>
namespace mars {
namespace {
constexpr Vec3 ink{.028f,.032f,.031f},muted{.56f,.61f,.57f},paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
const char* vertex=R"GLSL(#version 330 core
layout(location=0) in vec3 position;
uniform vec3 centre,eye,rightward,upward,backward;
uniform float radius,aspect,lens,tilt,clipNear;
out vec3 point,local;
void main() {
    float c=cos(tilt),s=sin(tilt);
    point=vec3(c*position.x-s*position.y,s*position.x+c*position.y,position.z)*radius; local=position;
    vec3 rel=point+centre-eye; float z=-dot(rel,backward);
    float near=clipNear,far=20.0;
    gl_Position=vec4(dot(rel,rightward)/(lens*aspect),dot(rel,upward)/lens,
                    (far+near)/(far-near)*z-2.0*far*near/(far-near),z);
})GLSL";
const char* fragment=R"GLSL(#version 330 core
in vec3 point,local;
uniform vec3 tint;
uniform float style;
out vec4 color;
void main() {
    float light=1.0;
    if(style<2.5) {
        vec3 n=normalize(cross(dFdx(point),dFdy(point)));
        if(dot(n,point)<0.0) n=-n;
        light=.45+.14*floor(max(0.0,dot(n,normalize(vec3(-.5,.8,.6))))*4.0);
    }
    float bands=style>.5&&style<2.5?mix(.80,1.0,step(.45,fract(local.y*4.0))):1.0;
    color=vec4(tint*light*bands,1.0);
})GLSL";
}
void Renderer::initPlaceholders() {
    bodyProgram_=program(vertex,fragment);
    auto g=solar::makePlaceholderGeometry();
    sphereCount_=GLsizei(g.sphere.size()); ringCount_=GLsizei(g.ring.size()); circleCount_=GLsizei(g.circle.size());
    g.sphere.insert(g.sphere.end(),g.ring.begin(),g.ring.end());
    g.sphere.insert(g.sphere.end(),g.circle.begin(),g.circle.end());
    gl_.GenVertexArrays(1,&bodyVao_); gl_.BindVertexArray(bodyVao_);
    gl_.GenBuffers(1,&bodyVbo_); gl_.BindBuffer(GL_ARRAY_BUFFER,bodyVbo_);
    gl_.BufferData(GL_ARRAY_BUFFER,GLsizeiptr(g.sphere.size()*sizeof(Vec3)),g.sphere.data(),GL_STATIC_DRAW);
    gl_.EnableVertexAttribArray(0); gl_.VertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3),nullptr);
}
void Renderer::placeholders(const solar::SceneFrame& scene) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    spaceBackground(scene.camera);
    gl_.Enable(GL_DEPTH_TEST); gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    gl_.UseProgram(bodyProgram_); gl_.BindVertexArray(bodyVao_);
    auto vec=[&](const char* key,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(bodyProgram_,key),v.x,v.y,v.z); };
    auto scalar=[&](const char* key,float v) { gl_.Uniform1f(gl_.GetUniformLocation(bodyProgram_,key),v); };
    vec("eye",scene.camera.position()); vec("rightward",scene.camera.right());
    vec("upward",scene.camera.up()); vec("backward",scene.camera.backward());
    scalar("aspect",float(layout_.pixelWidth)/layout_.pixelHeight); scalar("lens",scene.camera.lens); scalar("clipNear",scene.camera.clipNear);
    scalar("style",3); vec("tint",{.13f,.17f,.15f}); vec("centre",scene.orbitCentre);
    for(std::size_t i=0;i<scene.lanes.size();++i) {
        scalar("radius",scene.lanes[i]); scalar("tilt",scene.laneTilts.empty()?0.f:float(scene.laneTilts[i]*radians));
        gl_.DrawArrays(GL_LINES,sphereCount_+ringCount_,circleCount_);
    }
    scalar("tilt",0);
    for(const auto& b:scene.bodies) {
        if(b.id==solar::BodyId::Sun&&sunProgram_) continue;
        if(b.id==solar::BodyId::Saturn&&saturnAvailable()) continue;
        if(b.id==solar::BodyId::Jupiter&&jupiterAvailable()) continue;
        if(b.id==solar::BodyId::Venus&&venusAvailable()) continue;
        if(b.id==solar::BodyId::Earth&&earthAvailable()) continue;
        if(satelliteAvailable(b.id)) continue;
        if(b.id==solar::BodyId::Io&&ioAvailable()) continue;
        if(b.id==solar::BodyId::Callisto&&callistoAvailable()) continue;
        if(scene.surfacePreviews&&previewAvailable(b.id)) continue;
        const auto& def=solar::body(b.id);
        vec("centre",b.centre); scalar("radius",b.radius); vec("tint",def.color); scalar("style",float(def.style));
        gl_.DrawArrays(GL_TRIANGLES,0,sphereCount_);
        if(def.style==2) { scalar("style",3); vec("tint",def.color*.65f); gl_.DrawArrays(GL_TRIANGLES,sphereCount_,ringCount_); }
    }
    gl_.Disable(GL_DEPTH_TEST);
    if(scene.surfacePreviews) for(const auto& b:scene.bodies) if(previewAvailable(b.id))
        drawTerrain(scene.camera,(scene.trueSolarScale||scene.trueEarthScale||scene.trueMarsScale)?1:10,Environment{},b.id==solar::BodyId::Mars?2:b.id==solar::BodyId::Moon?5:3,false,b.centre,b.radius);
    for(const auto& b:scene.bodies) if(satelliteAvailable(b.id)) {
        // Spherical atlases face their zero longitude toward Jupiter; NASA
        // visualization models retain their authored axes in the schematic view.
        float spin=!satellites::definition(b.id).model&&dot(b.centre,b.centre)>0?float((location(b.centre).lon+180)*radians):0;
        drawSatellite(b.id,scene.camera,b.centre,b.radius,spin);
    }
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Venus) drawVenus(scene.camera,b.centre,b.radius,true);
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Earth) drawEarth(scene.camera,b.centre,b.radius,true);
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Callisto) {
        float spin=dot(b.centre,b.centre)>0?float((location(b.centre).lon+180)*radians):0;
        drawCallisto(scene.camera,b.centre,b.radius,spin);
    }
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Io) {
        // Orient the zero-longitude face toward Jupiter in the static system.
        float spin=dot(b.centre,b.centre)>0?float((location(b.centre).lon+180)*radians):0;
        drawIo(scene.camera,b.centre,b.radius,spin);
    }
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Jupiter)
        drawJupiter(scene.camera,b.centre,b.radius,jupiterRotation_);
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Saturn)
        drawSaturn(scene.camera,b.centre,b.radius,saturnRotation_);
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Sun) drawSun(scene.camera,b.centre,b.radius);
    for(const auto& b:scene.bodies) if(ice::planet(b.id)) drawOuterRings(b.id,scene.camera,b.centre,b.radius);
    for(const auto& b:scene.bodies) if(b.id==solar::BodyId::Haumea&&satelliteAvailable(b.id)) drawHaumeaRing(scene.camera,b.centre,b.radius);
    drawBelts(scene);
}
void Renderer::navigationHud(const solar::Navigation& nav,const solar::SceneFrame& scene,float mx,float my,
                             bool fullscreen,float fps,const std::string& status,bool loading) {
    ui_.clear();
    auto button=[&](Rect r,const std::string& label,bool selected=false) {
        bool hover=r.contains(mx,my);
        rect(float(r.x),float(r.y),float(r.w),float(r.h),selected?accent:ink,selected?.95f:.86f);
        if(hover) border(r,paper,.7f);
        buttonLabel(r.x+6,r.y+(r.h-7)/2,label,selected?ink:paper);
    };
    auto panel=[&](Rect r) { rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.84f); rect(float(r.x),float(r.y),float(r.w),1,muted,.6f); };
    auto view=nav.view(); bool globe=view.kind==solar::ViewKind::Globe;
    bool jovian=view==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Jupiter};
    bool solar=view.kind==solar::ViewKind::Solar;
    bool belt=nav.belt();
    bool earthSystem=scene.trueEarthScale,martian=scene.trueMarsScale,saturnSystem=scene.trueSaturnScale,outer=scene.trueOuterScale;
    if(!nav.detailed()||!status.empty()) {
        rect(20,20,3,42,accent);
        text(32,26,view.kind==solar::ViewKind::Solar?"SOLAR SYSTEM":solar::body(view.focus).name,paper,3);
        text(32,54,view.kind==solar::ViewKind::Solar?"01 / INTERPLANETARY ATLAS":nav.sun()?"03 / SOLAR ACTIVITY":globe?"03 / REFERENCE GLOBE":belt?"02 / HELIOCENTRIC ORBITAL REGION":"02 / LOCAL SYSTEM",muted);
        button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN");
        button(layout_.homeButton(),"HOME / RESET");
        auto sc=layout_.scalePanel(); panel(sc);
        bool fit=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
        char buf[160]; std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fit?" FIT":"");
        text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
        if(solar) {
            panel(layout_.solarPanel());
            button(layout_.solarWhole(),"F / WHOLE SOLAR SYSTEM");
            button(layout_.solarInner(),"N / INNER PLANETS");
            text(layout_.width()-216,184,"1 AU = 149.6 MILLION KM",muted);
            if(nav.state().approached!=solar::BodyId::None) {
                auto bodyCentre=solar::mapPosition(solar::BodyId::Sun,nav.state().approached)-scene.renderOrigin;
                auto offset=scene.camera.position()-bodyCentre;
                double km=std::sqrt(double(dot(offset,offset)))/solar::solarUnitsPerAu*solar::astronomicalUnitKm;
                std::snprintf(buf,sizeof(buf),"%.0f KM FROM %s",km,solar::body(nav.state().approached).name);
            } else std::snprintf(buf,sizeof(buf),"CAMERA / %.3f AU FROM SUN",double(scene.camera.distance)/solar::solarUnitsPerAu);
            text(layout_.width()-216,204,buf,muted);
        }
        if(belt) {
            Rect p{layout_.width()-226,120,206,194}; panel(p);
            text(p.x+10,p.y+12,"ORBITAL DISTRIBUTION",accent);
            text(p.x+10,p.y+34,view.focus==solar::BodyId::AsteroidBelt?"MAIN BELT / ~2.1-3.3 AU":"MAIN KUIPER / ~30-50 AU",paper);
            text(p.x+10,p.y+54,view.focus==solar::BodyId::AsteroidBelt?"BETWEEN MARS AND JUPITER":"BEYOND NEPTUNE",muted);
            text(p.x+10,p.y+80,"DOTS / SAMPLE LOCATORS",muted);
            text(p.x+10,p.y+96,"NOT VISIBLE ROCK SIZES",muted);
            text(p.x+10,p.y+118,view.focus==solar::BodyId::AsteroidBelt?"RESONANCES / KIRKWOOD GAPS":"COLD / HOT / RESONANT /",muted);
            text(p.x+10,p.y+134,view.focus==solar::BodyId::AsteroidBelt?"GAPS IN ORBITAL SEMIMAJOR A":"SCATTERED POPULATIONS",muted);
            text(p.x+10,p.y+157,"IDEALIZED / STATIC ATLAS",muted);
            text(p.x+10,p.y+174,"MOST OF THIS VOLUME IS EMPTY",paper);
        }
        if(outer) {
            button(layout_.saturnWhole(),"F / WHOLE SYSTEM");
            button(layout_.saturnInner(),"N / INNER SYSTEM");
            button(layout_.saturnApproach(),"A / APPROACH SELECTED");
        }
        if(saturnSystem) {
            button(layout_.saturnWhole(),"F / WHOLE SATURN SYSTEM");
            button(layout_.saturnInner(),"N / INNER MOONS");
            button(layout_.saturnApproach(),"A / APPROACH SELECTED");
        }
        if(martian) {
            panel(layout_.marsSystemPanel()); button(layout_.marsWhole(),"F / WHOLE MARS SYSTEM");
            button(layout_.marsApproach(),"A / APPROACH SELECTED");
            text(layout_.width()-216,184,"SIZES + DISTANCES TO SCALE",muted);
            text(layout_.width()-216,204,"STATIC / MEAN ORBITS",muted);
        }
        auto ids=nav.visibleBodies();
        if(!nav.sun()) {
        panel(layout_.bodyList());
        text(28,layout_.bodyList().y+8,globe?"BODY / FOCUSED":belt?"REGION / DWARF PLANETS":"BODIES / SELECT TO APPROACH",muted);
        for(std::size_t i=0;i<ids.size();++i) {
            auto id=ids[i]; auto r=layout_.bodyButton(int(i));
            button(r,solar::body(id).name,id==nav.state().selected);
            rect(float(r.x+r.w-10),float(r.y+8),4,4,solar::body(id).color);
        }
        }
        if(solar) {
            button(layout_.asteroidBelt(),"MAIN ASTEROID BELT",nav.state().selected==solar::BodyId::AsteroidBelt);
            button(layout_.kuiperBelt(),"KUIPER BELT",nav.state().selected==solar::BodyId::KuiperBelt);
        }
        if(view.kind==solar::ViewKind::System&&!outer&&!saturnSystem&&!martian&&!jovian) {
            button(layout_.earthWhole(),belt?"F / WHOLE REGION":"F / WHOLE SYSTEM");
            button(layout_.earthApproach(),"A / APPROACH SELECTED");
        }
        if(!globe) button(layout_.openBody(),"ENTER / OPEN SELECTED");
        else if(nav.detailed()&&!loading) button(layout_.openBody(),"R / RETRY SURFACE DATA");
        auto info=layout_.readout(); panel(info);
        const auto& selected=solar::body(nav.state().selected);
        text(info.x+10,info.y+10,selected.name,paper,2);
        std::string type=selected.kind==solar::BodyKind::Star?"STAR":selected.kind==solar::BodyKind::Planet?"PLANET":selected.kind==solar::BodyKind::DwarfPlanet?"DWARF PLANET":selected.kind==solar::BodyKind::Region?"ORBITAL REGION":"MOON";
        text(info.x+10,info.y+33,type+" / "+(selected.kind==solar::BodyKind::Region?"SPARSE ORBITAL SAMPLES":selected.id>=solar::BodyId::Haumea?"UNTEXTURED REFERENCE SHAPE":selected.id==solar::BodyId::Sun&&sunProgram_?"SELF LUMINOUS / PLASMA":selected.id==solar::BodyId::Mars?"MOLA / SOURCE DATA":selected.id==solar::BodyId::Mercury?"MDIS / SOURCE DATA":selected.id==solar::BodyId::Moon?"LOLA / SOURCE DATA":selected.id==solar::BodyId::Venus&&venusAvailable()?(venusTopography_?"INFERRED SURFACE COLOR":"ILLUSTRATIVE CLOUD SHELL"):selected.id==solar::BodyId::Proteus?"UNTEXTURED REFERENCE SPHERE":ice::planet(selected.id)&&satelliteAvailable(selected.id)?"ILLUSTRATIVE ATMOSPHERE + RINGS":satelliteAvailable(selected.id)?(satellites::definition(selected.id).model?"NASA TEXTURED SHAPE":"TINTED VOYAGER / GALILEO"):selected.id==solar::BodyId::Callisto&&callistoAvailable()?"TINTED VOYAGER / GALILEO":selected.id==solar::BodyId::Io&&ioAvailable()?"VOYAGER / GALILEO MOSAIC":selected.id==solar::BodyId::Earth&&earthAvailable()?(earthTopography_?"ETOPO 2022 / TOPOGRAPHY":"BLUE MARBLE + ETOPO"):selected.id==solar::BodyId::Saturn&&saturnAvailable()?"MODELED ATMOSPHERE + RINGS":selected.id==solar::BodyId::Jupiter&&jupiterAvailable()?"STYLIZED ATMOSPHERE":"PLACEHOLDER"),accent);
        if(belt||selected.kind==solar::BodyKind::Region) {
            if(solar::belts::isDwarf(selected.id)) {
                const auto& d=solar::belts::dwarf(selected.id);
                std::snprintf(buf,sizeof(buf),"%s RADIUS / %.1f KM",selected.id==solar::BodyId::Haumea?"EST. EQUIV.":"REF.",d.radiusKm);
                text(info.x+10,info.y+51,buf,muted);
                std::snprintf(buf,sizeof(buf),"a %.2f AU / e %.3f / i %.1f DEG",d.a,d.e,d.i); text(info.x+10,info.y+66,buf,muted);
            } else {
                bool main=selected.id==solar::BodyId::AsteroidBelt||view.focus==solar::BodyId::AsteroidBelt;
                text(info.x+10,info.y+51,selected.id==solar::BodyId::Sun?"MEAN RADIUS / 695,700 KM":main?"MAIN BELT / ABOUT 2.1-3.3 AU":"MAIN BELT / ABOUT 30-50 AU",muted);
                text(info.x+10,info.y+66,main?"CERES / ENTER TO OPEN":"4 DWARFS / ERIS IN SCATTERED DISK",muted);
            }
        } else if(globe) {
            text(info.x+10,info.y+51,nav.sun()?"RADIUS / 695,700 KM":loading?"ARCHIVE SCAN IN PROGRESS":"SURFACE DATA NOT LOADED",muted);
            text(info.x+10,info.y+66,nav.sun()?"EMISSION COLOR / AUTHORED":"NO MEASURED ELEVATION",muted);
        } else if(solar) {
            const auto& physical=solar::solarBody(selected.id);
            std::snprintf(buf,sizeof(buf),"%s RADIUS / %.1f KM",selected.id==solar::BodyId::Sun?"MEAN":"EQ.",physical.radiusKm);
            text(info.x+10,info.y+51,buf,muted);
            if(selected.id==solar::BodyId::Sun) text(info.x+10,info.y+66,"8 PLANETS / SELECT AND ENTER",muted);
            else {
                std::snprintf(buf,sizeof(buf),"REFERENCE ORBIT / %.3f AU",physical.orbitAu);
                text(info.x+10,info.y+66,buf,muted);
            }
        } else if(outer) {
            double radius=ice::equatorialKm(view.focus),orbit=0;
            for(const auto& m:solar::outerMoons(view.focus)) if(m.id==selected.id) { radius=m.radiusKm; orbit=m.orbitKm; }
            std::snprintf(buf,sizeof(buf),"RADIUS / %.2f KM",radius); text(info.x+10,info.y+51,buf,paper);
            std::snprintf(buf,sizeof(buf),"MEAN ORBIT / %.0f KM",orbit); text(info.x+10,info.y+67,buf,muted);
        } else if(saturnSystem) {
            double radius=solar::saturnRadiusKm,orbit=0;
            for(const auto& m:solar::saturnianMoons()) if(m.id==selected.id) { radius=m.radiusKm; orbit=m.orbitKm; }
            std::snprintf(buf,sizeof(buf),"%s RADIUS / %.2f KM",selected.id==solar::BodyId::Saturn?"EQ.":"MEAN",radius);
            text(info.x+10,info.y+51,buf,muted);
            if(orbit>0) { std::snprintf(buf,sizeof(buf),"MEAN ORBIT / %.0f KM",orbit); text(info.x+10,info.y+66,buf,muted); }
            else text(info.x+10,info.y+66,"RINGS TO SCALE / ENTER TO OPEN",muted);
        } else if(martian) {
            double radius=solar::marsRadiusKm,orbit=0;
            for(const auto& m:solar::martianMoons()) if(m.id==selected.id) { radius=m.radiusKm; orbit=m.orbitKm; }
            std::snprintf(buf,sizeof(buf),"%s RADIUS / %.2f KM",selected.id==solar::BodyId::Mars?"EQ.":"MEAN",radius);
            text(info.x+10,info.y+51,buf,muted);
            if(orbit>0) { std::snprintf(buf,sizeof(buf),"MEAN ORBIT / %.0f KM FROM CENTRE",orbit); text(info.x+10,info.y+66,buf,muted); }
            else text(info.x+10,info.y+66,"RELIEF 1X / ENTER TO OPEN",muted);
        } else if(earthSystem) {
            std::snprintf(buf,sizeof(buf),"MEAN RADIUS / %.1f KM",selected.id==solar::BodyId::Earth?solar::earthRadiusKm:solar::moonRadiusKm);
            text(info.x+10,info.y+51,buf,muted);
            text(info.x+10,info.y+66,"RELIEF 1X / ENTER TO OPEN",muted);
        } else {
            auto count=solar::children(selected.id).size();
            std::snprintf(buf,sizeof(buf),"%zu %s INCLUDED",count,selected.kind==solar::BodyKind::Star?"PLANETS":"MOONS");
            text(info.x+10,info.y+51,buf,muted);
            text(info.x+10,info.y+66,scene.surfacePreviews&&previewAvailable(selected.id)?"ARCHIVE RELIEF PREVIEW / HEIGHT 10X":"DOUBLE CLICK OR ENTER TO OPEN",muted);
        }
        auto legend=layout_.legend(); panel(legend);
        text(legend.x+10,legend.y+10,belt?"BELT / PHYSICAL DISTANCES":nav.sun()?"SUN / SOLAR ATMOSPHERE":globe?"REFERENCE GLOBE":solar?"SOLAR SYSTEM / TRUE SCALE":jovian?"JOVIAN SYSTEM / TRUE SCALE":earthSystem?"EARTH / MOON / TRUE SCALE":martian?"MARS SYSTEM / TRUE SCALE":outer?"ICE GIANT SYSTEM / TRUE SCALE":saturnSystem?"SATURN SYSTEM / TRUE SCALE":"SCHEMATIC / STATIC LAYOUT",accent);
        text(legend.x+10,legend.y+29,belt?"DWARF SIZES TO SAME SCALE":nav.sun()?"GRANULATION + ACTIVE REGIONS":globe?(previewAvailable(view.focus)?"ARCHIVE RELIEF PREVIEW / 10X":"SHAPE / VISUAL PLACEHOLDER"):solar?"BODY SIZES + DISTANCES":jovian?"MEAN ORBITAL DISTANCES":earthSystem?"384,400 KM / CENTRE TO CENTRE":martian||saturnSystem||outer?"MEAN ORBITAL DISTANCES":"DISTANCES COMPRESSED",muted);
        text(legend.x+10,legend.y+44,belt?"DOTS = ENLARGED LOCATORS":nav.sun()?"FLARES + EXPANDING PLASMA":globe?"DETAIL FOLLOWS SOURCE DATA":solar?"CROSSES = LOCATOR MARKERS":jovian||earthSystem||martian||saturnSystem||outer?"BODY SIZES TO SCALE":"BODY SIZES ENLARGED",muted);
        text(legend.x+10,legend.y+63,belt?"ELLIPSES / NOT LIVE EPHEMERIS":nav.sun()?"ILLUSTRATIVE / NOT LIVE WEATHER":globe?"ROTATE / DRAG":solar?"STATIC / CIRCULAR ORBITS":jovian||earthSystem||martian||saturnSystem||outer?"STATIC / MEAN DISTANCES":"POSITIONS NOT EPHEMERIDES",muted);
        for(const auto& l:scene.labels) {
            bool active=l.id==nav.state().selected;
            if(l.visible&&!layout_.overHud(l.x,l.y)&&!globe) {
                if((solar||belt||martian||saturnSystem||outer)&&l.radius<2) {
                    auto tint=active?accent:solar::body(l.id).color;
                    rect(l.x-5,l.y,3,1,tint); rect(l.x+2,l.y,3,1,tint);
                    rect(l.x,l.y-5,1,3,tint); rect(l.x,l.y+2,1,3,tint);
                    if(l.shown) {
                        float end=float(l.rect.x>l.x?l.rect.x:l.rect.x+l.rect.w),middle=l.rect.y+l.rect.h*.5f;
                        rect(std::min(l.x,end),l.y,std::abs(end-l.x),1,tint,.32f);
                        rect(end,std::min(l.y,middle),1,std::abs(middle-l.y),tint,.32f);
                    }
                }
                int r=int(std::max(l.radius+4,7.0f));
                if(active) border({int(l.x)-r,int(l.y)-r,2*r,2*r},accent,.8f);
            }
            if(l.shown) button(l.rect,solar::body(l.id).name,active);
        }
        rect(20,float(layout_.height()-21),float(layout_.width()-40),21,ink,.9f);
        text(20,layout_.height()-13,globe?"DRAG / ROTATE   WHEEL / ZOOM   ALT+LEFT / UP":"DRAG / ROTATE   WHEEL / ZOOM   UP/DOWN / SELECT   ENTER / OPEN",muted);
        std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(layout_.width()-74,layout_.height()-13,buf,muted);
    }
    auto crumbs=nav.breadcrumbs(); auto boxes=solar::crumbRects(nav,layout_);
    for(std::size_t i=0;i<crumbs.size();++i) {
        button(boxes[i],crumbs[i].label,crumbs[i].view==view);
        if(i+1<crumbs.size()) text(boxes[i].x+boxes[i].w,boxes[i].y+6,">",muted);
    }
    if(view.kind!=solar::ViewKind::Solar) button(layout_.navigationUp(),(layout_.marsView||layout_.mercuryView||layout_.moonView||layout_.jupiterView||layout_.saturnView||layout_.earthView||layout_.venusView||layout_.ioView||layout_.callistoView||layout_.satelliteView)?"UP":"ALT+LEFT / UP");
    if(loading) {
        Rect r{(layout_.width()-336)/2,layout_.height()-174,336,48}; panel(r);
        text(r.x+12,r.y+9,"SCANNING / "+std::string(solar::body(view.focus).name),accent);
        text(r.x+12,r.y+27,status,paper);
        for(int i=0;i<12;++i) rect(float(r.x+241+i*6),float(r.y+10),3,5,accent,std::fmod(scanPhase_*12-i+12,12)<3?1:.2f);
    } else if(!status.empty()) {
        Rect r{(layout_.width()-342)/2,(layout_.height()-112)/2,342,112}; panel(r);
        text(r.x+12,r.y+12,nav.moon()?"MOON / LOLA":nav.mercury()?"MERCURY / MDIS":"MARS / MOLA DATA",accent,2);
        text(r.x+12,r.y+40,status,paper);
        text(r.x+12,r.y+62,"PREPARE: SEE README / DATA TOOLS",muted);
        text(r.x+12,r.y+82,"ALT+LEFT / UP    R / RETRY",muted);
    }
    flushUi();
}
void Renderer::transition(float alpha) {
    if(alpha<=0) return;
    ui_.clear(); rect(0,0,float(layout_.width()),float(layout_.height()),ink,alpha); flushUi();
}
}
