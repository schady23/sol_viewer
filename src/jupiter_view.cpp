#include "jupiter_view.hpp"
namespace jupiter {
void View::clamp(mars::Camera& c) { c.lat=std::clamp(c.lat,-90.0,90.0); c.lon=mars::wrapLongitude(c.lon); c.distance=std::clamp(c.distance,1.06f,8.f); }
mars::Camera View::pose() const { auto c=camera; if(frame==Frame::Rotation) c.lon=mars::wrapLongitude(c.lon-clock.rotation()); return c; }
void View::setFrame(Frame value) {
    if(value==frame) return;
    double oldOffset=frame==Frame::Rotation?-clock.rotation():0,newOffset=value==Frame::Rotation?-clock.rotation():0;
    camera.lon+=oldOffset-newOffset; target.lon+=oldOffset-newOffset; frame=value;
    clamp(camera); clamp(target);
}
void View::home() { frame=Frame::Follow; target={-12,spotLongitude,2.6f}; selected=-1; }
void View::select(int i) {
    if(i<0||i>=4) return;
    setFrame(i==0?Frame::Spot:Frame::Follow); selected=i;
    auto f=features[i]; target={centricLatitude(f.latitude),f.longitude,f.distance};
}
void View::resetAtmosphere() {
    double offset=frame==Frame::Rotation?-clock.rotation():0;
    camera.lon+=offset; target.lon+=offset; clock.reset(); clamp(camera); clamp(target);
}
void View::drag(float dx,float dy,int height) {
    if(frame==Frame::Spot) setFrame(Frame::Follow);
    double speed=60.0/height*std::max(.06f,camera.distance-1);
    target.lon-=dx*speed; target.lat+=dy*speed; selected=-1; clamp(target);
}
void View::zoom(float amount) { target.distance=1+(target.distance-1)*std::exp(-amount*.13f); clamp(target); }
void View::key(SDL_Keycode key) {
    if(key>=SDLK_1&&key<=SDLK_4) select(int(key-SDLK_1));
    if(key==SDLK_HOME) home();
    if(key==SDLK_SPACE) clock.paused=!clock.paused;
    if(key==SDLK_S) clock.cycleRate();
    if(key==SDLK_V) setFrame(frame==Frame::Rotation?Frame::Follow:Frame::Rotation);
    if(key==SDLK_R) resetAtmosphere();
}
void View::click(const mars::ViewLayout& l,float x,float y) {
    if(l.jupiterView) {
        for(int i=0;i<4;++i) if(l.landmarkButton(i).contains(x,y)) select(i);
        if(l.homeButton().contains(x,y)) home();
        if(l.jupiterFrame().contains(x,y)) key(SDLK_V);
    }
    if(l.jupiterPause().contains(x,y)) key(SDLK_SPACE);
    if(l.jupiterSlower().contains(x,y)) clock.cycleRate(-1);
    if(l.jupiterFaster().contains(x,y)) clock.cycleRate();
    if(l.jupiterReset().contains(x,y)) resetAtmosphere();
}
void View::update(double seconds,const bool* keys,bool focused) {
    double dt=std::clamp(seconds,0.0,.05);
    if(focused) {
        float dx=float((keys[SDL_SCANCODE_RIGHT]?1:0)-(keys[SDL_SCANCODE_LEFT]?1:0));
        float dy=float((keys[SDL_SCANCODE_UP]?1:0)-(keys[SDL_SCANCODE_DOWN]?1:0));
        if(dx!=0||dy!=0) drag(float(-dx*dt*210),float(dy*dt*210),360);
        if(keys[SDL_SCANCODE_EQUALS]||keys[SDL_SCANCODE_KP_PLUS]) zoom(float(dt*10));
        if(keys[SDL_SCANCODE_MINUS]||keys[SDL_SCANCODE_KP_MINUS]) zoom(float(-dt*10));
    }
    clamp(target); double ease=1-std::exp(-dt*12);
    camera.lat+=(target.lat-camera.lat)*ease; camera.lon+=std::remainder(target.lon-camera.lon,360.0)*ease;
    camera.distance+=float((target.distance-camera.distance)*ease); clamp(camera);
}
}
