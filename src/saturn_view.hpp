#pragma once
#include "saturn.hpp"
#include "ui.hpp"
#include <SDL3/SDL.h>
#include <optional>
namespace saturn {
enum class Frame { Follow,Rotation };
struct View {
    Clock clock;
    mars::Camera camera{24,130,6.2f},target=camera;
    Frame frame=Frame::Follow;
    int selected=-1;
    void home();
    void select(int index);
    void setFrame(Frame value);
    void resetAtmosphere();
    void drag(float dx,float dy,int height);
    void zoom(float steps);
    void key(SDL_Keycode key);
    void click(const mars::ViewLayout& layout,float x,float y);
    void update(double dt,const bool* keys,bool focused);
    mars::Camera pose() const;
    static void clamp(mars::Camera& camera);
};
std::optional<mars::Location> pickCloud(float x,float y,const mars::Camera& camera,const mars::ViewLayout& layout);
}
