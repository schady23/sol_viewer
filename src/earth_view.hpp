#pragma once
#include "earth.hpp"
#include "ui.hpp"
#include <SDL3/SDL.h>
namespace earth {
struct View {
    Clouds clouds;
    bool topography=false;
    float exaggeration=10;
    mars::Camera camera{18,20,2.8f},target=camera;
    int selected=-1;
    void home();
    void select(int index,bool revealSeafloor=true);
    void drag(float dx,float dy,int height);
    void zoom(float amount);
    void key(SDL_Keycode key);
    void click(const mars::ViewLayout& layout,float x,float y);
    void update(double dt,const bool* keys,bool focused);
    static void clamp(mars::Camera& camera);
};
}
