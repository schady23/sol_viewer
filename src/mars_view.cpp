#include "mars_view.hpp"
#include <iostream>
namespace mars {
namespace {
double number(const std::string& value) {
    std::size_t used=0; double n;
    try { n=std::stod(value,&used); }
    catch(const std::exception&) { throw std::runtime_error("ENTER A VALID NUMBER"); }
    if(used!=value.size()||!std::isfinite(n)) throw std::runtime_error("ENTER A FINITE NUMBER");
    return n;
}
}
void MarsView::stopEdit(SDL_Window* window) { edit={}; SDL_StopTextInput(window); }
bool MarsView::commitEdit(SDL_Window* window) {
    if(edit.field<0) return true;
    try {
        auto el=orbit.elements(); double v=number(edit.value);
        if(edit.field==0) el.periapsisKm=v; else if(edit.field==1) el.apoapsisKm=v; else el.inclinationDeg=v;
        orbit.configure(el); stopEdit(window); return true;
    } catch(const std::exception& e) { edit.error=e.what(); return false; }
}
void MarsView::setOrbit(bool active,SDL_Window* window) {
    stopEdit(window);
    if(active&&!orbit.enabled) {
        atlasCamera=camera;
        if(!orbitStarted) { orbit.reset(camera.lon); orbitStarted=true; }
    }
    if(!active&&orbit.enabled) { camera=atlasCamera; target=atlasCamera; }
    orbit.enabled=active;
}
void MarsView::home(SDL_Window* window) { setOrbit(false,window); target={}; selected=-1; }
void MarsView::select(int i,SDL_Window* window) {
    setOrbit(false,window); selected=i; auto p=landmarks[i]; target={p.lat,p.lon,p.distance};
}
void MarsView::drag(float dx,float dy,int height) {
    if(orbit.enabled) {
        orbit.lookYaw=std::remainder(orbit.lookYaw-dx*.22,360.0);
        orbit.lookPitch=std::clamp(orbit.lookPitch+dy*.18,-80.0,80.0);
    } else {
        double speed=60.0/height*std::max(.085f,camera.distance-1);
        target.lon-=dx*speed; target.lat+=dy*speed; target.clamp(); selected=-1;
    }
}
void MarsView::zoom(float amount) {
    if(orbit.enabled) orbit.fovDegrees=std::clamp(orbit.fovDegrees*std::exp(-amount*.08),25.0,90.0);
    else target.distance=1+(target.distance-1)*std::exp(-amount*.13f);
}
void MarsView::key(SDL_Keycode key,SDL_Window* window) {
    if(edit.field>=0) {
        if(key==SDLK_ESCAPE) stopEdit(window);
        else if(key==SDLK_RETURN||key==SDLK_KP_ENTER) commitEdit(window);
        else if(key==SDLK_BACKSPACE) {
            if(edit.replace) edit.value.clear(); else if(!edit.value.empty()) edit.value.pop_back();
            edit.replace=false; edit.error.clear();
        }
        return;
    }
    if(key>=SDLK_1&&key<=SDLK_4) select(int(key-SDLK_1),window);
    if(key==SDLK_HOME) home(window);
    if(key==SDLK_E&&!orbit.enabled) exaggeration=exaggeration==10?1:10;
    if(key==SDLK_O) setOrbit(!orbit.enabled,window);
    if(key==SDLK_SPACE&&orbit.enabled) orbit.paused=!orbit.paused;
    if(key==SDLK_S&&orbit.enabled) orbit.cycleSpeed();
    if(key==SDLK_V&&orbit.enabled) { orbit.limbView=!orbit.limbView; orbit.resetLook(); }
    if(key==SDLK_T) environment.topography=!environment.topography;
    if(key==SDLK_P) environment.paused=!environment.paused;
    if(key==SDLK_D) environment.previewStorm({camera.lat,camera.lon});
}
void MarsView::click(const ViewLayout& layout,float x,float y,SDL_Window* window) {
    if(!orbit.enabled) for(int i=0;i<4;++i) if(layout.landmarkButton(i).contains(x,y)) select(i,window);
    if(!orbit.enabled&&layout.reliefButton().contains(x,y)) exaggeration=exaggeration==10?1:10;
    if(layout.homeButton().contains(x,y)) home(window);
    if(layout.orbitButton().contains(x,y)) setOrbit(!orbit.enabled,window);
    if(layout.satellite&&orbit.enabled) {
        for(int i=0;i<3;++i) {
            if(layout.orbitValue(i).contains(x,y)) {
                auto el=orbit.elements();
                double v=i==0?el.periapsisKm:i==1?el.apoapsisKm:el.inclinationDeg;
                edit={i,std::to_string(v),"",true}; SDL_StartTextInput(window);
            }
            int sign=layout.orbitPlus(i).contains(x,y)?1:layout.orbitMinus(i).contains(x,y)?-1:0;
            if(sign) {
                auto el=orbit.elements();
                if(i==0) el.periapsisKm+=50*sign;
                else if(i==1) el.apoapsisKm+=100*sign;
                else el.inclinationDeg+=5*sign;
                try { orbit.configure(el); edit.error.clear(); }
                catch(const std::exception& e) { edit.error=e.what(); }
            }
        }
        if(layout.orbitPause().contains(x,y)) orbit.paused=!orbit.paused;
        if(layout.orbitSpeed().contains(x,y)) orbit.cycleSpeed();
        if(layout.orbitView().contains(x,y)) { orbit.limbView=!orbit.limbView; orbit.resetLook(); }
    }
    if(layout.environmentOpen) {
        if(layout.layerButton(0).contains(x,y)) environment.ice=!environment.ice;
        if(layout.layerButton(1).contains(x,y)) environment.clouds=!environment.clouds;
        if(layout.layerButton(2).contains(x,y)) environment.dust=!environment.dust;
        if(layout.layerButton(3).contains(x,y)) environment.topography=!environment.topography;
        if(layout.pauseButton().contains(x,y)) environment.paused=!environment.paused;
        if(layout.stormButton().contains(x,y)) environment.previewStorm({camera.lat,camera.lon});
    }
}
void MarsView::update(double seconds,const bool* keys,bool focused) {
    float dt=float(std::clamp(seconds,0.0,.05));
    if(edit.field<0&&!orbit.enabled&&focused) {
        double speed=dt*35*std::max(0.12f,camera.distance-1);
        if(keys[SDL_SCANCODE_LEFT]) { target.lon-=speed; selected=-1; }
        if(keys[SDL_SCANCODE_RIGHT]) { target.lon+=speed; selected=-1; }
        if(keys[SDL_SCANCODE_UP]) { target.lat+=speed; selected=-1; }
        if(keys[SDL_SCANCODE_DOWN]) { target.lat-=speed; selected=-1; }
        if(keys[SDL_SCANCODE_EQUALS]||keys[SDL_SCANCODE_KP_PLUS]) target.distance=1+(target.distance-1)*std::exp(-dt*1.4f);
        if(keys[SDL_SCANCODE_MINUS]||keys[SDL_SCANCODE_KP_MINUS]) target.distance=1+(target.distance-1)*std::exp(dt*1.4f);
    }
    if(orbit.enabled) { orbit.advance(seconds); camera=orbit.camera(); }
    else {
        target.clamp(); float ease=1-std::exp(-dt*12);
        camera.lat+=(target.lat-camera.lat)*ease;
        camera.lon+=std::remainder(target.lon-camera.lon,360.0)*ease;
        camera.distance+=(target.distance-camera.distance)*ease; camera.clamp();
    }
    environment.update(seconds);
}
void MarsView::draw(Renderer& renderer,const HeightField& field,float mouseX,float mouseY,float fps,bool hud,bool fullscreen,bool smoke) {
    float relief=orbit.enabled?1:exaggeration;
    renderer.terrain(camera,relief,environment);
    auto hit=hud && renderer.layout().overHud(mouseX,mouseY)?std::nullopt:renderer.pick(mouseX,mouseY,camera);
    mars::Probe probe;
    probe.underPointer=hit.has_value();
    auto centre=hit?hit:orbit.enabled?renderer.pick(renderer.layout().width()*0.5f,renderer.layout().height()*0.5f,camera):
                                   std::optional<mars::Location>{{camera.lat,camera.lon}};
    probe.subsatellite=!centre&&orbit.enabled;
    probe.location=centre.value_or(mars::Location{camera.lat,camera.lon});
    probe.meters=field.sample(probe.location.lat,probe.location.lon);
    float checkX=renderer.layout().width()*0.54f,checkY=renderer.layout().height()*0.5f;
    auto beforeWeather=!smokeDone&&smoke?renderer.pick(checkX,checkY,camera):std::nullopt;
    renderer.weather(camera,relief,environment);
    if(!smokeDone && smoke) {
        auto afterWeather=renderer.pick(checkX,checkY,camera);
        if(beforeWeather.has_value()!=afterWeather.has_value() || (beforeWeather&&
           (beforeWeather->lat!=afterWeather->lat||beforeWeather->lon!=afterWeather->lon)))
            throw std::runtime_error("Weather changed terrain picking depth");
        auto center=renderer.pick(renderer.layout().width()*0.5f,renderer.layout().height()*0.5f,camera);
        if(!orbit.enabled&&(!center || mars::dot(mars::direction(center->lat,center->lon),camera.outward())<0.9999f))
            throw std::runtime_error("Render smoke check failed: centre depth/coordinates");
        std::cout<<"Centre probe: "<<probe.location.lat<<" N, "<<probe.location.lon<<" E, "<<probe.meters<<" m\n";
    }
    if(smoke) smokeDone=true;
    if(hud) renderer.hud(camera,field,probe,relief,selected,mouseX,mouseY,fps,fullscreen,environment,orbit,edit);
}
MarsLoader::~MarsLoader() { if(thread_) SDL_WaitThread(thread_,nullptr); }
int SDLCALL MarsLoader::load(void* user) {
    auto& self=*static_cast<MarsLoader*>(user);
    try {
        self.field=std::make_unique<HeightField>(HeightField::load(self.path_));
        self.phase.store(1,std::memory_order_relaxed);
        self.pending_=makeGlobe(*self.field,self.columns_);
    } catch(const std::exception& e) { self.error=e.what(); }
    self.phase.store(2,std::memory_order_relaxed);
    self.done_.store(true,std::memory_order_release); return 0;
}
void MarsLoader::request(const std::filesystem::path& path,int columns) {
    if(status==Status::Loading||status==Status::Ready) return;
    path_=path; columns_=columns; error.clear(); field.reset(); pending_={};
    phase=0; done_=false; status=Status::Loading;
    thread_=SDL_CreateThread(load,"Mars data preparation",this);
    if(!thread_) { error=SDL_GetError(); status=Status::Failed; }
}
void MarsLoader::poll(Renderer& renderer) {
    if(status!=Status::Loading||!done_.load(std::memory_order_acquire)) return;
    SDL_WaitThread(thread_,nullptr); thread_=nullptr;
    if(error.empty()) {
        try { renderer.uploadTerrain(pending_); }
        catch(const std::exception& e) { renderer.clearTerrain(); error=e.what(); }
    }
    pending_={};
    status=error.empty()?Status::Ready:Status::Failed;
    if(status==Status::Failed) { field.reset(); std::cerr<<"Mars unavailable: "<<error<<'\n'; }
    else std::cout<<"Mars ready: "<<renderer.triangles()<<" triangles / 16 ppd readouts\n";
}
}
