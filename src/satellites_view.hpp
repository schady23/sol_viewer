#pragma once
#include "satellites.hpp"
#include "ice_giants.hpp"
#include "ui.hpp"
#include <SDL3/SDL.h>

namespace satellites {
struct View {
    solar::BodyId body=solar::BodyId::Phobos;
    explicit View(solar::BodyId id=solar::BodyId::Phobos):body(id) { home(); camera=target; }
    mars::Camera camera{-8,100,2.8f},target=camera;
    int selected=-1;
    bool lighting=true,haze=true;
    unsigned revision=0;
    ice::Clock clock;
    void home();
    void select(int index);
    void drag(float dx,float dy,int height);
    void zoom(float amount);
    void key(SDL_Keycode key);
    void click(const mars::ViewLayout& layout,float x,float y);
    void update(double dt,const bool* keys,bool focused);
    static void clamp(mars::Camera& camera);
};
}
