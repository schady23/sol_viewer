#include "earth_view.hpp"
namespace earth {
void View::clamp(mars::Camera& c) {
    c.lat=std::clamp(c.lat,-90.0,90.0); c.lon=mars::wrapLongitude(c.lon);
    c.distance=std::clamp(c.distance,1.18f,8.f); // Regional DEM and appearance, not ground-level terrain.
}
void View::home() { target={18,20,2.8f}; selected=-1; }
void View::select(int index,bool revealSeafloor) {
    if(index<0||index>=int(landmarks.size())) return;
    const auto& place=landmarks[index];
    selected=index; target={place.lat,place.lon,place.distance};
    if(revealSeafloor&&place.seafloor) topography=true;
}
void View::drag(float dx,float dy,int height) {
    double speed=60.0/height*std::max(.06f,camera.distance-1);
    target.lon-=dx*speed; target.lat+=dy*speed; selected=-1; clamp(target);
}
void View::zoom(float amount) { target.distance=1+(target.distance-1)*std::exp(-amount*.13f); clamp(target); }
void View::key(SDL_Keycode key) {
    if(key>=SDLK_1&&key<=SDLK_4) select(int(key-SDLK_1));
    if(key==SDLK_HOME) home();
    if(key==SDLK_T) topography=!topography;
    if(key==SDLK_E) exaggeration=exaggeration==10?1:10;
    if(key==SDLK_C&&!topography) clouds.visible=!clouds.visible;
    if(key==SDLK_SPACE) clouds.paused=!clouds.paused;
    if(key==SDLK_R) clouds.reset();
}
void View::click(const mars::ViewLayout& l,float x,float y) {
    if(!l.earthControls) return;
    if(l.earthView) {
        for(int i=0;i<4;++i) if(l.landmarkButton(i).contains(x,y)) select(i);
        if(l.homeButton().contains(x,y)) home();
    }
    if(l.earthMap().contains(x,y)) key(SDLK_T);
    if(l.earthView&&l.earthRelief().contains(x,y)) key(SDLK_E);
    if(l.earthToggle().contains(x,y)) key(SDLK_C);
    if(l.earthPause().contains(x,y)) key(SDLK_SPACE);
    if(l.earthReset().contains(x,y)) key(SDLK_R);
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
