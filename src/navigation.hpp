#pragma once
#include "catalog.hpp"
#include <array>
namespace solar {
constexpr double solarOverviewDistance=9.5,solarInnerDistance=.52;
constexpr double solarMinDistance=.008,solarMaxDistance=12;
constexpr double earthSystemDistance=11,earthApproachDistance=.22;
constexpr double earthSystemMinDistance=.175,earthSystemMaxDistance=14;
constexpr double marsSystemDistance=7.2,marsApproachDistance=1.12;
constexpr double marsSystemMinDistance=.95,marsSystemMaxDistance=10;
constexpr double saturnSystemDistance=11.3,saturnInnerDistance=1.8,saturnApproachDistance=.40;
constexpr double saturnSystemMinDistance=.165,saturnSystemMaxDistance=14;
enum class ViewKind { Solar, System, Globe };
struct ViewKey {
    ViewKind kind=ViewKind::Solar; BodyId focus=BodyId::Sun;
    bool operator==(ViewKey b) const { return kind==b.kind&&focus==b.focus; }
    bool operator!=(ViewKey b) const { return !(*this==b); }
};
struct MapCamera {
    double lat=58,lon=270,distance=3.3,x=0,y=0,z=0;
    mars::Camera pose() const;
    void clamp(bool globe,bool jovian=false,bool solar=false,bool earthSystem=false,bool marsSystem=false,bool saturnSystem=false,bool outerSystem=false);
};
struct ViewState { MapCamera camera,target; BodyId selected=BodyId::None,approached=BodyId::None; bool visited=false; };
struct Crumb { std::string label; ViewKey view; };
class Navigation {
public:
    ViewKey view() const { return current_; }
    bool belt() const { return current_.kind==ViewKind::System&&body(current_.focus).kind==BodyKind::Region; }
    bool sun() const { return current_==ViewKey{ViewKind::Globe,BodyId::Sun}; }
    bool mars() const { return current_==ViewKey{ViewKind::Globe,BodyId::Mars}; }
    bool mercury() const { return current_==ViewKey{ViewKind::Globe,BodyId::Mercury}; }
    bool saturn() const { return current_==ViewKey{ViewKind::Globe,BodyId::Saturn}; }
    bool jupiter() const { return current_==ViewKey{ViewKind::Globe,BodyId::Jupiter}; }
    bool moon() const { return current_==ViewKey{ViewKind::Globe,BodyId::Moon}; }
    bool earth() const { return current_==ViewKey{ViewKind::Globe,BodyId::Earth}; }
    // Earth can remain in the scene while a different body owns the controls.
    bool earthControls() const {
        return earth()||(current_==ViewKey{ViewKind::System,BodyId::Earth}&&state().selected==BodyId::Earth);
    }
    bool venus() const { return current_==ViewKey{ViewKind::Globe,BodyId::Venus}; }
    bool callisto() const { return current_==ViewKey{ViewKind::Globe,BodyId::Callisto}; }
    bool io() const { return current_==ViewKey{ViewKind::Globe,BodyId::Io}; }
    bool satellite() const { return current_.kind==ViewKind::Globe&&(body(current_.focus).kind==BodyKind::DwarfPlanet||current_.focus==BodyId::Phobos||current_.focus==BodyId::Deimos||current_.focus==BodyId::Europa||current_.focus==BodyId::Ganymede||body(current_.focus).parent==BodyId::Saturn||body(current_.focus).parent==BodyId::Uranus||body(current_.focus).parent==BodyId::Neptune||current_.focus==BodyId::Uranus||current_.focus==BodyId::Neptune); }
    bool detailed() const { return saturn()||mars()||mercury()||moon()||jupiter()||earth()||venus()||io()||callisto()||satellite(); }
    ViewState& state();
    const ViewState& state() const;
    std::vector<BodyId> visibleBodies() const;
    std::vector<Crumb> breadcrumbs() const;
    void start(ViewKey view); // Also initializes its ancestors for direct launches.
    void select(BodyId body);
    void selectAndApproach(BodyId body); // User selection in an overview also frames the body.
    void cycleSelection(int delta);
    void open();
    void up();
    void go(ViewKey view);
    void update(double dt);
    void drag(double dx,double dy);
    void zoom(double steps);
    void resetCamera();
    void approachSelected();
    void frameJovian(bool wholeSystem);
    void frameSolar(bool wholeSystem);
    void frameEarth(bool wholeSystem);
    void frameMars(bool wholeSystem);
    void frameOuter(int framing);
    void frameSaturn(int framing); // 0 whole, 1 inner moons, 2 approach Saturn.
    mars::Camera camera(mars::Vec3 renderOrigin={}) const;
    bool transitioning() const { return transition_>=0; }
    float fade() const;
    static ViewKey parent(ViewKey key);
private:
    void clampTarget();
    void centreSystem();
    static int slot(ViewKey key);
    ViewState& ensure(ViewKey key);
    std::array<ViewState,1+2*bodyCount> saved_{};
    ViewKey current_,destination_;
    double transition_=-1;
    mars::Vec3 transitionFocus_{};
    bool descending_=false;
};
}
