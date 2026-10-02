#pragma once
#include "renderer.hpp"
#include <atomic>
#include <memory>
namespace moon {
struct View {
    mars::Camera camera{12,5,2.68f},target=camera;
    int selected=-1,page=0,meshPpd=16;
    void turnPage(int delta);
    float exaggeration=10;
    bool heightColors=false,smokeDone=false;
    void home();
    void select(int i);
    void drag(float dx,float dy,int height);
    void zoom(float steps);
    void key(SDL_Keycode key);
    void click(const mars::ViewLayout& layout,float x,float y);
    void update(double seconds,const bool* keys,bool focused);
    void draw(mars::Renderer& renderer,const HeightField& field,float mouseX,float mouseY,float fps,bool hud,bool fullscreen,bool smoke);
    static void clamp(mars::Camera& camera);
};
class Loader {
public:
    enum class Status { Unloaded,Loading,Ready,Failed };
    Status status=Status::Unloaded;
    std::unique_ptr<HeightField> field;
    std::string error;
    std::atomic<int> phase{0}; // 0 archived samples; 1 relief mesh; 2 main-thread upload.
    ~Loader();
    void request(const std::filesystem::path& path,int columns);
    void poll(mars::Renderer& renderer);
private:
    SDL_Thread* thread_=nullptr;
    std::atomic<bool> done_{false};
    std::filesystem::path path_;
    int columns_=5760;
    mars::Mesh pending_;
    std::vector<float> slopes_;
    static int SDLCALL load(void* self);
};
}
