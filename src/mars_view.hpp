#pragma once
#include "renderer.hpp"
#include <atomic>
#include <memory>
namespace mars {
struct MarsView {
    Camera camera,target,atlasCamera;
    Environment environment;
    SatelliteOrbit orbit;
    NumberEdit edit;
    int selected=-1;
    float exaggeration=10;
    bool orbitStarted=false,smokeDone=false;
    void stopEdit(SDL_Window* window);
    bool commitEdit(SDL_Window* window);
    void setOrbit(bool active,SDL_Window* window);
    void home(SDL_Window* window);
    void select(int i,SDL_Window* window);
    void drag(float dx,float dy,int height);
    void zoom(float steps);
    void key(SDL_Keycode key,SDL_Window* window);
    void click(const ViewLayout& layout,float x,float y,SDL_Window* window);
    void update(double dt,const bool* keys,bool focused);
    void draw(Renderer& renderer,const HeightField& field,float mouseX,float mouseY,float fps,bool hud,bool fullscreen,bool smoke);
};
class MarsLoader {
public:
    enum class Status { Unloaded,Loading,Ready,Failed };
    Status status=Status::Unloaded;
    std::unique_ptr<HeightField> field;
    std::string error;
    std::atomic<int> phase{0}; // 0 archived samples; 1 relief mesh; 2 main-thread upload.
    ~MarsLoader();
    void request(const std::filesystem::path& path,int columns);
    void poll(Renderer& renderer);
private:
    SDL_Thread* thread_=nullptr;
    std::atomic<bool> done_{false};
    std::filesystem::path path_;
    int columns_=5760;
    Mesh pending_;
    static int SDLCALL load(void* self);
};
}
