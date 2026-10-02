#include "navigation.hpp"
#include "ice_giants.hpp"
#include "belts.hpp"
#include <algorithm>
#include <stdexcept>
namespace solar {
mars::Camera MapCamera::pose() const {
    mars::Camera c; c.lat=lat; c.lon=lon; c.distance=float(distance);
    auto back=mars::direction(lat,lon), right=mars::direction(0,lon+90);
    c.flight=true; c.flightBack=back; c.flightRight=right; c.flightUp=mars::cross(back,right);
    c.flightPosition={float(x+back.x*distance),float(y+back.y*distance),float(z+back.z*distance)};
    return c;
}
void MapCamera::clamp(bool globe,bool jovian,bool solar,bool earthSystem,bool marsSystem,bool saturnSystem,bool outerSystem) {
    lat=std::clamp(lat,globe?-89.0:15.0,89.0); lon=mars::wrapLongitude(lon);
    distance=std::clamp(distance,solar?solarMinDistance:outerSystem?.072:saturnSystem?saturnSystemMinDistance:marsSystem?marsSystemMinDistance:earthSystem?earthSystemMinDistance:jovian?.27:globe?1.35:1.8,solar?solarMaxDistance:outerSystem?6.0:saturnSystem?saturnSystemMaxDistance:marsSystem?marsSystemMaxDistance:earthSystem?earthSystemMaxDistance:jovian?10.0:8.0);
}
int Navigation::slot(ViewKey key) {
    body(key.focus);
    return key.kind==ViewKind::Solar?0:1+int(key.focus)+(key.kind==ViewKind::Globe?bodyCount:0);
}
ViewState& Navigation::ensure(ViewKey key) {
    auto& s=saved_[slot(key)];
    if(!s.visited) {
        s.visited=true; s.selected=key.focus;
        if(key.kind==ViewKind::Solar) s.camera.distance=solarOverviewDistance;
        if(key.kind==ViewKind::Globe) { s.camera.lat=12; s.camera.lon=253; s.camera.distance=key.focus==BodyId::Haumea?5.8:key.focus==BodyId::Sun?4.8:body(key.focus).style==2?5.4:3.0; }
        else if(key.kind==ViewKind::System) { s.camera.distance=key.focus==BodyId::Jupiter?7.8:key.focus==BodyId::Earth?earthSystemDistance:key.focus==BodyId::Mars?marsSystemDistance:key.focus==BodyId::Saturn?saturnSystemDistance:ice::planet(key.focus)?(key.focus==BodyId::Uranus?1.9:1.3):3.0; if(key.focus==BodyId::Jupiter||key.focus==BodyId::Earth||key.focus==BodyId::Mars||key.focus==BodyId::Saturn||ice::planet(key.focus)) s.camera.lat=30; }
        if(key==ViewKey{ViewKind::System,BodyId::Uranus}) s.camera.lon=210;
        if(key.kind==ViewKind::System&&belts::region(key.focus)) { s.camera.distance=belts::distance(key.focus); s.camera.lat=64; }
        s.target=s.camera;
    }
    return s;
}
ViewState& Navigation::state() { return ensure(current_); }
const ViewState& Navigation::state() const { return saved_[slot(current_)]; }
std::vector<BodyId> Navigation::visibleBodies() const {
    std::vector<BodyId> ids{current_.focus};
    if(current_.kind!=ViewKind::Globe) {
        if(belt()) ids.push_back(BodyId::Sun);
        auto c=children(current_.focus); ids.insert(ids.end(),c.begin(),c.end());
    }
    return ids;
}
ViewKey Navigation::parent(ViewKey key) {
    if(key.kind!=ViewKind::Globe||key.focus==BodyId::Sun) return {};
    return {ViewKind::System,(body(key.focus).kind==BodyKind::Moon||belts::isDwarf(key.focus))?body(key.focus).parent:key.focus};
}
std::vector<Crumb> Navigation::breadcrumbs() const {
    std::vector<Crumb> result{{"SOL",{}}};
    if(current_.kind==ViewKind::Solar) return result;
    ViewKey system=current_.kind==ViewKind::System?current_:parent(current_);
    if(system.kind==ViewKind::System) result.push_back({std::string(body(system.focus).name)+(belts::region(system.focus)?"":" SYS"),system});
    if(current_.kind==ViewKind::Globe) result.push_back({body(current_.focus).name,current_});
    return result;
}
void Navigation::start(ViewKey key) {
    if(key.kind==ViewKind::Solar) key={};
    if(key.kind==ViewKind::Globe&&belts::region(key.focus)) key.kind=ViewKind::System;
    if(key.kind==ViewKind::System&&body(key.focus).kind!=BodyKind::Planet&&!belts::region(key.focus))
        throw std::runtime_error("A local system must focus a planet");
    ensure({});
    if(key.kind!=ViewKind::Solar) {
        auto p=parent(key);
        if(p.kind==ViewKind::System) { ensure(p).selected=key.focus; ensure({}).selected=p.focus; }
        else if(key.kind==ViewKind::System) ensure({}).selected=key.focus;
    }
    current_=key; ensure(key); transition_=-1;
}
void Navigation::select(BodyId id) {
    if(transitioning()) return;
    auto ids=visibleBodies();
    if(std::find(ids.begin(),ids.end(),id)!=ids.end()) state().selected=id;
}
void Navigation::selectAndApproach(BodyId id) {
    if(transitioning()) return;
    if(current_.kind==ViewKind::Solar&&belts::region(id)) { state().selected=id; go({ViewKind::System,id}); return; }
    auto ids=visibleBodies();
    if(std::find(ids.begin(),ids.end(),id)==ids.end()) return;
    select(id); approachSelected();
}
void Navigation::cycleSelection(int delta) {
    auto ids=visibleBodies(); auto it=std::find(ids.begin(),ids.end(),state().selected);
    int index=it==ids.end()?0:int(it-ids.begin());
    selectAndApproach(ids[(index+delta+int(ids.size()))%int(ids.size())]);
}
void Navigation::open() {
    if(current_.kind==ViewKind::Globe) return;
    auto id=state().selected; if(id==BodyId::None) return;
    if(belts::region(id)) { if(current_.kind==ViewKind::Solar) go({ViewKind::System,id}); return; }
    go({current_.kind==ViewKind::Solar&&id!=BodyId::Sun?ViewKind::System:ViewKind::Globe,id});
}
void Navigation::up() { go(parent(current_)); }
void Navigation::go(ViewKey key) {
    if(transitioning()||key==current_) return;
    ensure(key); destination_=key; transition_=0;
    descending_=int(key.kind)>int(current_.kind);
    transitionFocus_=descending_?mapPosition(current_.focus,key.focus)-mars::Vec3{float(state().camera.x),float(state().camera.y),float(state().camera.z)}:mars::Vec3{};
}
void Navigation::update(double dt) {
    dt=std::clamp(dt,0.0,0.25);
    if(transitioning()) {
        double previous=transition_; transition_+=dt;
        if(previous<0.22&&transition_>=0.22) current_=destination_;
        if(transition_>=0.44) transition_=-1;
        return;
    }
    auto& s=state(); clampTarget();
    double ease=1-std::exp(-dt*12);
    s.camera.lat+=(s.target.lat-s.camera.lat)*ease;
    s.camera.lon+=std::remainder(s.target.lon-s.camera.lon,360.0)*ease;
    s.camera.distance+=(s.target.distance-s.camera.distance)*ease;
    s.camera.x+=(s.target.x-s.camera.x)*ease;
    s.camera.y+=(s.target.y-s.camera.y)*ease;
    s.camera.z+=(s.target.z-s.camera.z)*ease;
}
void Navigation::drag(double dx,double dy) {
    if(transitioning()) return;
    state().target.lon-=dx*.25; state().target.lat+=dy*.25;
    clampTarget();
}
void Navigation::zoom(double steps) {
    if(transitioning()) return;
    state().target.distance*=std::exp(-steps*.10); clampTarget();
}
void Navigation::clampTarget() {
    auto& s=state();
    double distance=s.target.distance;
    if(belt()) {
        s.target.lat=std::clamp(s.target.lat,15.,89.); s.target.lon=mars::wrapLongitude(s.target.lon);
        double minimum=s.approached==BodyId::None?.1:systemBodyRadius(current_.focus,s.approached)*1.5;
        s.target.distance=std::clamp(distance,minimum,current_.focus==BodyId::AsteroidBelt?14.:12.); return;
    }
    s.target.clamp(current_.kind==ViewKind::Globe,current_==ViewKey{ViewKind::System,BodyId::Jupiter},current_.kind==ViewKind::Solar,current_==ViewKey{ViewKind::System,BodyId::Earth},current_==ViewKey{ViewKind::System,BodyId::Mars},current_==ViewKey{ViewKind::System,BodyId::Saturn},current_.kind==ViewKind::System&&ice::planet(current_.focus));
    if(current_.kind!=ViewKind::Globe&&s.approached!=BodyId::None) {
        double maximum=current_.kind==ViewKind::Solar?solarMaxDistance:ice::planet(current_.focus)?6.:current_.focus==BodyId::Saturn?saturnSystemMaxDistance:current_.focus==BodyId::Mars?marsSystemMaxDistance:current_.focus==BodyId::Earth?earthSystemMaxDistance:current_.focus==BodyId::Jupiter?10.:8.;
        double radius=current_.kind==ViewKind::Solar?solarBodyRadius(s.approached):systemBodyRadius(current_.focus,s.approached);
        s.target.distance=std::clamp(distance,radius*1.5,maximum);
    }
}
void Navigation::centreSystem() {
    auto& s=state(); s.approached=BodyId::None;
    s.target.x=s.target.y=s.target.z=0;
}
void Navigation::approachSelected() {
    if(transitioning()||current_.kind==ViewKind::Globe) return;
    auto& s=state();
    auto ids=visibleBodies();
    if(std::find(ids.begin(),ids.end(),s.selected)==ids.end()) return;
    if(belts::region(s.selected)) { resetCamera(); return; }
    auto centre=mapPosition(current_.focus,s.selected);
    s.approached=s.selected;
    s.target.x=centre.x; s.target.y=centre.y; s.target.z=centre.z;
    // Fit the body, including rings when approaching a ringed planet.
    double radius=current_.kind==ViewKind::Solar?solarBodyRadius(s.selected):systemBodyRadius(current_.focus,s.selected);
    if(s.selected==BodyId::Saturn) radius*=2.4;
    if(s.selected==BodyId::Haumea) radius*=2322./1161;
    if(ice::planet(s.selected)) radius*=ice::rings(s.selected).back().radiusKm/ice::equatorialKm(s.selected)*1.1;
    s.target.distance=radius*(s.selected==BodyId::Sun?5.8:3.4);
    clampTarget();
}
void Navigation::resetCamera() {
    if(transitioning()) return;
    auto& s=state(); s.approached=BodyId::None; s.target=MapCamera{};
    if(current_.kind==ViewKind::Solar) s.target.distance=solarOverviewDistance;
    if(current_.kind==ViewKind::Globe) { s.target.lat=12; s.target.lon=253; s.target.distance=current_.focus==BodyId::Haumea?5.8:current_.focus==BodyId::Sun?4.8:body(current_.focus).style==2?5.4:3.0; }
    else if(current_.kind==ViewKind::System) { s.target.distance=current_.focus==BodyId::Jupiter?7.8:current_.focus==BodyId::Earth?earthSystemDistance:current_.focus==BodyId::Mars?marsSystemDistance:current_.focus==BodyId::Saturn?saturnSystemDistance:ice::planet(current_.focus)?(current_.focus==BodyId::Uranus?1.9:1.3):3.0; if(current_.focus==BodyId::Jupiter||current_.focus==BodyId::Earth||current_.focus==BodyId::Mars||current_.focus==BodyId::Saturn||ice::planet(current_.focus)) s.target.lat=30; }
    if(current_==ViewKey{ViewKind::System,BodyId::Uranus}) s.target.lon=210;
    if(belt()) { s.target.distance=belts::distance(current_.focus); s.target.lat=64; }
}
void Navigation::frameSolar(bool wholeSystem) {
    if(transitioning()||current_.kind!=ViewKind::Solar) return;
    centreSystem();
    state().target.distance=wholeSystem?solarOverviewDistance:solarInnerDistance;
}
void Navigation::frameEarth(bool wholeSystem) {
    if(transitioning()||current_!=ViewKey{ViewKind::System,BodyId::Earth}) return;
    centreSystem();
    state().target.distance=wholeSystem?earthSystemDistance:earthApproachDistance;
    if(!wholeSystem) state().target.lat=18;
}
void Navigation::frameMars(bool wholeSystem) {
    if(transitioning()||current_!=ViewKey{ViewKind::System,BodyId::Mars}) return;
    centreSystem();
    state().target.distance=wholeSystem?marsSystemDistance:marsApproachDistance;
    if(!wholeSystem) state().target.lat=18;
}
void Navigation::frameOuter(int framing) {
    if(transitioning()||current_.kind!=ViewKind::System||!ice::planet(current_.focus)) return;
    centreSystem();
    bool u=current_.focus==BodyId::Uranus;
    state().target.distance=framing==0?(u?1.9:1.3):framing==1?.65:.18;
}
void Navigation::frameSaturn(int framing) {
    if(transitioning()||current_!=ViewKey{ViewKind::System,BodyId::Saturn}) return;
    centreSystem();
    state().target.distance=framing==0?saturnSystemDistance:framing==1?saturnInnerDistance:saturnApproachDistance;
    if(framing==2) state().target.lat=24;
}
void Navigation::frameJovian(bool wholeSystem) {
    if(transitioning()||current_!=ViewKey{ViewKind::System,BodyId::Jupiter}) return;
    centreSystem();
    state().target.distance=wholeSystem?7.8:.38;
    if(!wholeSystem) state().target.lat=18;
}
mars::Camera Navigation::camera(mars::Vec3 renderOrigin) const {
    MapCamera c=state().camera;
    // A small dolly plus fade joins independently scaled views. Saved poses are never modified.
    if(transitioning()) {
        if(transition_<.22) {
            double t=transition_/.22; t=t*t*(3-2*t);
            c.x+=transitionFocus_.x*t; c.y+=transitionFocus_.y*t; c.z+=transitionFocus_.z*t;
            c.distance*=descending_?1-.25*t:1+.12*t;
        } else c.distance*=1+.12*(1-(transition_-.22)/.22);
    }
    // Rebase before converting the double camera position to float. This
    // keeps small planets stable when approached at distant solar coordinates.
    c.x-=renderOrigin.x; c.y-=renderOrigin.y; c.z-=renderOrigin.z;
    auto result=c.pose();
    if(current_.kind==ViewKind::System||(current_.kind==ViewKind::Solar&&state().approached!=BodyId::None)) result.clipNear=float(std::min(double(mars::Camera::nearPlane),c.distance*.01));
    return result;
}
float Navigation::fade() const {
    if(!transitioning()) return 0;
    return float(std::clamp((0.12-std::abs(transition_-.22))/.08,0.0,1.0));
}
}
