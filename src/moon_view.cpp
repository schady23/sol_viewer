#include "moon_view.hpp"
#include <iostream>
namespace moon {
void View::clamp(mars::Camera& c) {
    c.lat=std::clamp(c.lat,-90.0,90.0); c.lon=mars::wrapLongitude(c.lon);
    c.distance=std::clamp(c.distance,minimumDistance,mars::Camera::maxDistance);
}
void View::home() { target={12,5,2.68f}; selected=-1; page=0; }
void View::turnPage(int delta) { page=(page+delta%pageCount+pageCount)%pageCount; }
void View::select(int i) { if(i<0||i>=landmarkCount) return; page=i/pageSize; selected=i; auto p=landmarks[i]; target={p.lat,p.lon,p.distance}; }
void View::drag(float dx,float dy,int height) {
    double speed=60.0/height*std::max(.035f,camera.distance-1);
    target.lon-=dx*speed; target.lat+=dy*speed; clamp(target); selected=-1;
}
void View::zoom(float steps) { target.distance=1+(target.distance-1)*std::exp(-steps*.13f); clamp(target); }
void View::key(SDL_Keycode key) {
    if(key>=SDLK_1&&key<=SDLK_4) select(page*pageSize+int(key-SDLK_1));
    if(key==SDLK_COMMA) turnPage(-1);
    if(key==SDLK_PERIOD) turnPage(1);
    if(key==SDLK_HOME) home();
    if(key==SDLK_E) exaggeration=exaggeration==10?1:10;
    if(key==SDLK_C) heightColors=!heightColors;
}
void View::click(const mars::ViewLayout& layout,float x,float y) {
    for(int i=0;i<4;++i) if(layout.landmarkButton(i).contains(x,y)) select(page*pageSize+i);
    if(layout.locationPrev().contains(x,y)) turnPage(-1);
    if(layout.locationNext().contains(x,y)) turnPage(1);
    if(layout.reliefButton().contains(x,y)) exaggeration=exaggeration==10?1:10;
    if(layout.homeButton().contains(x,y)) home();
    if(layout.environmentButton().contains(x,y)) heightColors=!heightColors;
}
void View::update(double seconds,const bool* keys,bool focused) {
    float dt=float(std::clamp(seconds,0.0,.05));
    if(focused) {
        double speed=dt*35*std::max(.05f,camera.distance-1);
        if(keys[SDL_SCANCODE_LEFT]) { target.lon-=speed; selected=-1; }
        if(keys[SDL_SCANCODE_RIGHT]) { target.lon+=speed; selected=-1; }
        if(keys[SDL_SCANCODE_UP]) { target.lat+=speed; selected=-1; }
        if(keys[SDL_SCANCODE_DOWN]) { target.lat-=speed; selected=-1; }
        if(keys[SDL_SCANCODE_EQUALS]||keys[SDL_SCANCODE_KP_PLUS]) target.distance=1+(target.distance-1)*std::exp(-dt*1.4f);
        if(keys[SDL_SCANCODE_MINUS]||keys[SDL_SCANCODE_KP_MINUS]) target.distance=1+(target.distance-1)*std::exp(dt*1.4f);
    }
    clamp(target); float ease=1-std::exp(-dt*12);
    camera.lat+=(target.lat-camera.lat)*ease;
    camera.lon+=std::remainder(target.lon-camera.lon,360.0)*ease;
    camera.distance+=(target.distance-camera.distance)*ease; clamp(camera);
}
void View::draw(mars::Renderer& renderer,const HeightField& field,float mx,float my,float fps,bool hud,bool fullscreen,bool smoke) {
    renderer.terrain(camera,exaggeration,mars::Environment{},4,heightColors);
    auto hit=hud&&renderer.layout().overHud(mx,my)?std::nullopt:renderer.pick(mx,my,camera);
    mars::Probe probe;
    probe.underPointer=hit.has_value(); probe.location=hit.value_or(mars::Location{camera.lat,camera.lon});
    probe.meters=field.sample(probe.location.lat,probe.location.lon);
    if(smoke&&!smokeDone) {
        auto centre=renderer.pick(renderer.layout().width()*.5f,renderer.layout().height()*.5f,camera);
        if(std::isfinite(field.sample(camera.lat,camera.lon))&&(!centre ||
           mars::dot(mars::direction(centre->lat,centre->lon),camera.outward())<.9999f))
            throw std::runtime_error("Moon centre depth/coordinate smoke check failed");
        std::cout<<"Moon centre probe: "<<camera.lat<<" N, "<<camera.lon<<" E, "
                 <<field.sample(camera.lat,camera.lon)<<" m / 1737.4 km sphere\n";
        smokeDone=true;
    }
    if(hud) renderer.moonHud(camera,field,probe,exaggeration,selected,page,mx,my,fps,fullscreen,heightColors,meshPpd);
}
Loader::~Loader() { if(thread_) SDL_WaitThread(thread_,nullptr); }
int SDLCALL Loader::load(void* user) {
    auto& self=*static_cast<Loader*>(user);
    try {
        self.field=std::make_unique<HeightField>(HeightField::load(self.path_));
        self.phase.store(1,std::memory_order_relaxed);
        self.slopes_=polarSlopes(*self.field);
        self.pending_=mars::makeSampledGlobe(self.columns_,[&](double lat,double lon) { return self.field->sample(lat,lon); });
    } catch(const std::exception& e) { self.error=e.what(); }
    self.phase.store(2,std::memory_order_relaxed);
    self.done_.store(true,std::memory_order_release); return 0;
}
void Loader::request(const std::filesystem::path& path,int columns) {
    if(status==Status::Loading||status==Status::Ready) return;
    path_=path; columns_=columns; error.clear(); field.reset(); pending_={};
    phase=0; done_=false; status=Status::Loading;
    thread_=SDL_CreateThread(load,"Moon data preparation",this);
    if(!thread_) { error=SDL_GetError(); status=Status::Failed; }
}
void Loader::poll(mars::Renderer& renderer) {
    if(status!=Status::Loading||!done_.load(std::memory_order_acquire)) return;
    SDL_WaitThread(thread_,nullptr); thread_=nullptr;
    if(error.empty()) {
        try { renderer.uploadMoonSlopes(slopes_); renderer.uploadTerrain(pending_,4); }
        catch(const std::exception& e) { renderer.clearTerrain(4); error=e.what(); }
    }
    pending_={}; slopes_={}; status=error.empty()?Status::Ready:Status::Failed;
    if(status==Status::Failed) { field.reset(); std::cerr<<"Moon unavailable: "<<error<<'\n'; }
    else std::cout<<"Moon ready: "<<renderer.triangles(4)<<" triangles / 64 ppd readouts / "<<"LOLA / 1737.4 km sphere\n";
}
}
