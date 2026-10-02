#pragma once
#include "callisto.hpp"
#include "ui.hpp"
#include <SDL3/SDL.h>

namespace callisto {
struct View {
    mars::Camera camera{12,304,2.8f},target=camera;
    int selected=-1;
    bool lighting=true;
    unsigned revision=0;
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
