#include "earth_view.hpp"
#include "guide.hpp"
#include "navigation.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() {
    try {
        earth::View view;
        view.clouds.visible=false; view.clouds.paused=true; view.clouds.seconds=17;
        for(int i=0;i<4;++i) {
            view.key(SDLK_1+i);
            check(view.selected==i,"Landmark keyboard routing");
            check(view.target.lat==earth::landmarks[i].lat&&view.target.lon==earth::landmarks[i].lon,"Landmark camera anchor");
            check(!view.clouds.visible&&view.clouds.paused&&view.clouds.seconds==17,"Landmark resets cloud preferences");
        }
        check(view.topography,"Trench selection did not reveal seafloor");
        view.key(SDLK_C); check(!view.clouds.visible,"Hidden cloud toggle changed saved preference");
        view.key(SDLK_T); view.key(SDLK_C); check(view.clouds.visible&&!view.topography,"Returning to natural color lost cloud controls");
        view.key(SDLK_SPACE); check(!view.clouds.paused,"Cloud pause shortcut");
        view.key(SDLK_R); check(view.clouds.seconds==0,"Cloud reset shortcut");
        view.key(SDLK_E); check(view.exaggeration==1,"Relief shortcut");
        view.select(-1); view.select(4); check(view.selected==3,"Invalid landmark changes selection");
        view.select(1,false); view.topography=false;
        view.select(1,false); check(!view.topography,"CLI camera setup reveals other body's terrain");
        for(float scale:{.75f,1.f,1.5f,2.f}) {
            mars::ViewLayout layout; layout.marsView=false; layout.earthView=layout.earthControls=true;
            layout.resize(800,500,scale);
            auto r=layout.landmarkButton(1); view.click(layout,r.x+2,r.y+2);
            check(view.selected==1&&view.topography,"Trench click does not match keyboard");
            layout.earthView=false; layout.earthControls=false; layout.systemView=true;
            const auto seconds=view.clouds.seconds; const auto paused=view.clouds.paused;
            const auto visible=view.clouds.visible; const auto relief=view.exaggeration;
            for(auto control:{layout.earthMap(),layout.earthPause(),layout.earthReset(),layout.earthToggle(),layout.earthRelief()})
                view.click(layout,control.x+2,control.y+2);
            check(view.topography&&view.clouds.seconds==seconds&&view.clouds.paused==paused&&view.clouds.visible==visible&&view.exaggeration==relief,
                  "Hidden Earth controls respond to Moon-system clicks");
        }
        solar::Guide guide; view.select(0); guide.sync(solar::BodyId::Earth,view.selected);
        check(guide.locationOpen,"Landmark notes did not open");
        view.zoom(2); guide.sync(solar::BodyId::Earth,view.selected); check(guide.locationOpen,"Zoom unexpectedly clears landmark notes");
        view.drag(20,10,500); guide.sync(solar::BodyId::Earth,view.selected); check(!guide.locationOpen,"Dragging away retains stale notes");
        view.home(); check(view.selected==-1,"Home retains stale landmark");
        std::array<bool,SDL_SCANCODE_COUNT> keys{};
        keys[SDL_SCANCODE_RIGHT]=true; auto longitude=view.target.lon;
        view.update(.01,keys.data(),false); check(view.target.lon==longitude,"Unfocused globe accepts keyboard movement");
        view.update(.01,keys.data(),true); check(view.target.lon!=longitude,"Focused globe ignores keyboard movement");
        view.zoom(10000); check(view.target.distance>=1.18f,"Camera passes through Earth");
        view.zoom(-10000); check(view.target.distance<=8,"Camera zoom is unbounded");
        std::cout<<"PASS: Earth landmarks, seafloor reveal, input ownership, cloud preferences, notes and camera bounds\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
