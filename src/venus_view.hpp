#pragma once
#include "venus.hpp"
#include "ui.hpp"
#include <SDL3/SDL.h>
namespace venus {
struct View {
    Clouds clouds;
    bool topography=false;
    float exaggeration=10;
    mars::Camera camera{20,30,2.8f},target=camera;
    int selected=-1;
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
