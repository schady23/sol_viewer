#include "placeholders.hpp"
#include "sun.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
using namespace solar;
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
void finish(Navigation& nav) { for(int i=0;i<60;++i) nav.update(.01); check(!nav.transitioning(),"Transition did not finish"); }
int main() {
    try {
        check(bodies().size()==37&&children(BodyId::Sun).size()==8,"Catalog counts");
        check(children(BodyId::Jupiter).size()==4,"Representative Jupiter system");
        check(std::abs(double(solarBodyRadius(BodyId::Sun))/solarBodyRadius(BodyId::Earth)-695700/6378.1366)<1e-4,"Sun/Earth physical size ratio");
        check(std::abs(double(laneRadius(BodyId::Sun,7))/laneRadius(BodyId::Sun,2)-30.06992276/1.00000261)<1e-5,"Neptune/Earth distance ratio");
        // A diameter and an orbital distance must use the same conversion;
        // otherwise a realistic-looking orbit layout can still inflate planets.
        check(std::abs(2*double(solarBodyRadius(BodyId::Earth))/laneRadius(BodyId::Sun,2)-12756.2732/(149597870.7*1.00000261))<1e-10,"Body sizes and orbital distances use different scales");
        constexpr double moonRadii[]={1821.49,1560.8,2631.2,2410.3};
        constexpr double moonOrbits[]={421800,671100,1070400,1882700};
        auto moons=children(BodyId::Jupiter);
        for(std::size_t i=0;i<moons.size();++i) {
            double radiusRatio=systemBodyRadius(BodyId::Jupiter,moons[i])/systemBodyRadius(BodyId::Jupiter,BodyId::Jupiter);
            check(std::abs(radiusRatio-moonRadii[i]/71492)<1e-7,"Galilean moon radius ratio");
            auto position=mapPosition(BodyId::Jupiter,moons[i]);
            double distance=std::sqrt(mars::dot(position,position));
            check(std::abs(distance/systemBodyRadius(BodyId::Jupiter,BodyId::Jupiter)-moonOrbits[i]/71492)<1e-5,"Galilean mean orbital distance");
            check(std::abs(distance-laneRadius(BodyId::Jupiter,i))<1e-6,"Moon does not sit on orbit guide");
        }
        std::set<std::string> names;
        for(const auto& b:bodies()) {
            check(names.insert(b.name).second,"Duplicate body name"); check(bodyNamed(b.name)==b.id,"Name lookup");
            BodyId id=b.id; int depth=0;
            while(id!=BodyId::Sun) { id=body(id).parent; check(++depth<=2,"Invalid parent or cycle"); }
            check(b.displayRadius>0,"Invalid display size");
        }
        check(molaDataset().pixelsPerDegree==16&&molaDataset().referenceRadiusKm==3396,"MOLA descriptor changed");
        bool rejected=false; try { bodyNamed("not-a-body"); } catch(...) { rejected=true; } check(rejected,"Unknown body accepted");
        sun::Clock activity,otherActivity;
        for(int i=0;i<1000;++i) activity.advance(.01);
        for(int i=0;i<400;++i) otherActivity.advance(.025);
        check(std::abs(activity.seconds-otherActivity.seconds)<1e-9,"Solar activity depends on frame rate");
        activity.paused=true; auto stamp=activity.seconds; activity.advance(.1);
        check(activity.seconds==stamp,"Solar activity pause fails");
        activity.paused=false; activity.seconds=sun::loopSeconds-.1; activity.advance(.1);
        check(activity.seconds<1e-9,"Solar activity loop fails");
        activity.cycle(); check(activity.speed==3,"Solar activity speed fails");
        Navigation solarClose; solarClose.start({ViewKind::Globe,BodyId::Sun});
        mars::ViewLayout solarLayout; solarLayout.marsView=false; solarLayout.sunView=true; solarLayout.resize(1600,1000,1);
        auto solarScene=makeScene(solarClose,solarLayout);
        check(solarScene.bodies.size()==1&&solarScene.bodies[0].id==BodyId::Sun,"Sun explorer contains a reference body");
        for(auto button:{solarLayout.sunPause(),solarLayout.sunSpeed(),solarLayout.sunReset(),solarLayout.sunClose(),solarLayout.sunCrown(),solarLayout.sunScale()}) check(solarLayout.overHud(button.x+1,button.y+1),"Solar controls leak scene input");
        Navigation nav; nav.start({}); nav.select(BodyId::Jupiter);
        nav.drag(34,-12); nav.zoom(2); for(int i=0;i<400;++i) nav.update(.01);
        auto solarCamera=nav.state().camera;
        nav.open(); nav.open(); // Repeated enter while transitioning must not enqueue another level.
        finish(nav); check(nav.view()==ViewKey{ViewKind::System,BodyId::Jupiter},"Planet must open local system");
        nav.select(BodyId::Europa); nav.drag(-40,25); nav.zoom(-1); finish(nav);
        auto localCamera=nav.state().camera;
        nav.open(); finish(nav); check(nav.view()==ViewKey{ViewKind::Globe,BodyId::Europa},"Moon globe routing");
        nav.drag(400,800); nav.zoom(1000); finish(nav);
        check(std::isfinite(nav.camera().position().x)&&nav.state().target.distance>=1.35,"Camera limits");
        nav.up(); finish(nav);
        check(nav.state().selected==BodyId::Europa,"Parent selection lost");
        check(std::abs(nav.state().camera.lon-localCamera.lon)<.01,"Local camera not restored");
        nav.up(); finish(nav);
        check(nav.state().selected==BodyId::Jupiter,"Solar selection lost");
        check(std::abs(nav.state().camera.distance-solarCamera.distance)<1e-5,"Solar zoom not restored");
        Navigation atlas; atlas.start({}); atlas.frameSolar(false);
        for(int i=0;i<200;++i) atlas.update(.01);
        atlas.select(BodyId::Mercury); atlas.open(); finish(atlas); atlas.up(); finish(atlas);
        check(atlas.state().selected==BodyId::Mercury&&std::abs(atlas.state().camera.distance-solarInnerDistance)<1e-7,"Inner framing lost on system round trip");
        atlas.frameSolar(true); for(int i=0;i<200;++i) atlas.update(.01);
        check(std::abs(atlas.state().camera.distance-solarOverviewDistance)<1e-7,"Whole solar framing");
        atlas.zoom(1000); check(atlas.state().target.distance==solarMinDistance,"Solar zoom-in limit");
        check(solarMinDistance*.75-solarBodyRadius(BodyId::Sun)>mars::Camera::nearPlane,"Sun clips during close transition");
        atlas.zoom(-1000); check(atlas.state().target.distance==solarMaxDistance,"Solar zoom-out limit");
        check(solarMaxDistance+laneRadius(BodyId::Sun,7)<mars::Camera::farPlane,"Outer planets exceed far plane");
        atlas.resetCamera(); check(atlas.state().target.distance==solarOverviewDistance,"Solar reset uses old schematic framing");
        for(const auto& b:bodies()) {
            if(b.kind==BodyKind::Region) continue;
            nav.start({ViewKind::Globe,b.id}); check(nav.view().focus==b.id,"Direct globe launch");
            auto expected=Navigation::parent(nav.view()); nav.up(); finish(nav); check(nav.view()==expected,"Globe parent");
        }
        nav.start({ViewKind::Globe,BodyId::Mars}); check(nav.mars(),"Mars dispatch");
        nav.up(); finish(nav); check(nav.view()==ViewKey{ViewKind::System,BodyId::Mars},"Mars local return");
        nav.open(); finish(nav); check(nav.mars(),"Mars return selection");
        for(int i=0;i<100;++i) { nav.up(); finish(nav); nav.open(); finish(nav); }
        check(nav.mars(),"Repeated round trips");
        nav.start({ViewKind::Globe,BodyId::Jupiter}); check(nav.jupiter()&&nav.detailed(),"Jupiter detailed dispatch");
        nav.up(); finish(nav); check(nav.view()==ViewKey{ViewKind::System,BodyId::Jupiter},"Jupiter local return");
        nav.open(); finish(nav); check(nav.jupiter(),"Jupiter selection not restored");
        Navigation earthSystem; earthSystem.start({ViewKind::System,BodyId::Earth});
        double er=systemBodyRadius(BodyId::Earth,BodyId::Earth),mr=systemBodyRadius(BodyId::Earth,BodyId::Moon);
        check(std::abs(mr/er-1737.4/6371.)<1e-7,"Earth/Moon mean radius ratio");
        check(std::abs(laneRadius(BodyId::Earth,0)/er-384400./6371.)<1e-5,"Earth/Moon distances use a different scale");
        check(std::abs(std::sqrt(mars::dot(mapPosition(BodyId::Earth,BodyId::Moon),mapPosition(BodyId::Earth,BodyId::Moon)))-laneRadius(BodyId::Earth,0))<1e-6,"Moon is not on its reference orbit");
        earthSystem.frameEarth(false); for(int i=0;i<200;++i) earthSystem.update(.01);
        check(std::abs(earthSystem.camera().distance-earthApproachDistance)<1e-6,"Approach Earth framing");
        earthSystem.open(); finish(earthSystem); check(earthSystem.earth(),"Earth globe route");
        earthSystem.up(); finish(earthSystem); earthSystem.select(BodyId::Moon);
        earthSystem.open(); finish(earthSystem); check(earthSystem.moon(),"Moon globe route");
        earthSystem.up(); finish(earthSystem);
        check(earthSystem.state().selected==BodyId::Moon&&std::abs(earthSystem.camera().distance-earthApproachDistance)<1e-6,"Earth/Moon return loses system framing");
        earthSystem.frameEarth(true); for(int i=0;i<200;++i) earthSystem.update(.01);
        check(std::abs(earthSystem.camera().distance-earthSystemDistance)<1e-6,"Whole Earth system framing");
        earthSystem.zoom(1000); check(earthSystem.state().target.distance==earthSystemMinDistance,"Earth system near limit");
        check(earthSystemMinDistance*.75-er>mars::Camera::nearPlane,"Earth near-plane clipping during transition");
        earthSystem.zoom(-1000); check(earthSystem.state().target.distance==earthSystemMaxDistance,"Earth system far limit");
        check(earthSystemMaxDistance*1.12+laneRadius(BodyId::Earth,0)+mr<mars::Camera::farPlane,"Moon far-plane clipping during transition");
        earthSystem.resetCamera(); check(earthSystem.state().target.distance==earthSystemDistance,"Earth system reset");
        Navigation jovian; jovian.start({ViewKind::System,BodyId::Jupiter});
        jovian.select(BodyId::Ganymede); jovian.frameJovian(false);
        for(int i=0;i<200;++i) jovian.update(.01);
        check(std::abs(jovian.camera().distance-.38)<1e-6,"Approach framing");
        jovian.open(); finish(jovian); jovian.up(); finish(jovian);
        check(jovian.state().selected==BodyId::Ganymede&&std::abs(jovian.camera().distance-.38)<1e-6,"Jovian close framing lost on return");
        jovian.frameJovian(true); for(int i=0;i<200;++i) jovian.update(.01);
        check(std::abs(jovian.camera().distance-7.8)<1e-6,"Whole-system framing");
        jovian.zoom(1000); check(jovian.state().target.distance==.27,"Jovian minimum distance");
        jovian.zoom(-1000); check(jovian.state().target.distance==10,"Jovian maximum distance");
        // Selected-body approaches stay within the system and retain physical scale.
        for(auto planet:children(BodyId::Sun)) {
            Navigation focus; focus.start({ViewKind::System,planet});
            for(auto id:focus.visibleBodies()) {
                focus.resetCamera(); for(int i=0;i<300;++i) focus.update(.01);
                auto before=focus.state().camera;
                focus.selectAndApproach(id);
                check(focus.state().selected==id&&focus.state().approached==id,"UI selection does not approach selected body");
                check(focus.view()==ViewKey{ViewKind::System,planet}&&!focus.transitioning(),"Approach opens a globe");
                check(focus.state().camera.distance==before.distance,"Approach snaps instead of easing");
                for(int i=0;i<300;++i) focus.update(.01);
                auto centre=mapPosition(planet,id); auto& pose=focus.state().camera;
                check(std::abs(pose.x-centre.x)+std::abs(pose.y-centre.y)+std::abs(pose.z-centre.z)<1e-8,"Approach misses selected body");
                double radius=systemBodyRadius(planet,id);
                check(pose.distance>radius*3&&pose.distance<radius*16,"Approach ignores selected size or rings");
                check(pose.distance-radius>focus.camera().clipNear,"Small moon clips at approach");
                auto saved=pose; focus.open(); finish(focus); focus.up(); finish(focus);
                check(focus.state().selected==id&&focus.state().approached==id&&std::abs(focus.state().camera.x-saved.x)<1e-8,"Focused camera not restored after globe");
                focus.zoom(1000); focus.update(.25);
                check(std::abs(focus.state().target.distance-radius*1.5)<1e-10,"Focused zoom uses planet-system minimum");
                for(int i=0;i<300;++i) focus.update(.01);
                check(focus.state().camera.distance-radius>focus.camera().clipNear,"Focused near limit clips body");
                for(auto dims:{std::pair<int,int>{800,1200},{1600,900}}) {
                    mars::ViewLayout layout; layout.marsView=false; layout.systemView=true; layout.resize(dims.first,dims.second,1);
                    auto scene=makeScene(focus,layout);
                    bool centred=false;
                    for(auto label:scene.labels) if(label.id==id) centred=label.visible&&std::abs(label.x-layout.width()*.5)<.5&&std::abs(label.y-layout.height()*.5)<.5;
                    check(centred,"Focused body is not centred in portrait/landscape");
                    check(layout.overHud(layout.earthApproach().x+1,layout.earthApproach().y+1),"Approach button leaks scene input");
                }
                focus.resetCamera(); for(int i=0;i<300;++i) focus.update(.01);
                check(focus.state().approached==BodyId::None&&std::abs(focus.state().camera.x)+std::abs(focus.state().camera.y)+std::abs(focus.state().camera.z)<1e-8,"Reset retains moon focus");
            }
            focus.select(focus.visibleBodies().back()); focus.approachSelected();
            if(planet==BodyId::Earth) focus.frameEarth(true);
            else if(planet==BodyId::Mars) focus.frameMars(true);
            else if(planet==BodyId::Jupiter) focus.frameJovian(true);
            else if(planet==BodyId::Saturn) focus.frameSaturn(0);
            else if(planet==BodyId::Uranus||planet==BodyId::Neptune) focus.frameOuter(0);
            else focus.resetCamera();
            check(focus.state().target.x==0&&focus.state().target.y==0&&focus.state().target.z==0&&focus.state().approached==BodyId::None,"Whole-system frame retains body offset");
        }
        for(const auto& physical:solarBodies()) {
            Navigation autoSolar; autoSolar.start({});
            auto before=autoSolar.state().camera;
            autoSolar.selectAndApproach(physical.id);
            check(autoSolar.view().kind==ViewKind::Solar&&!autoSolar.transitioning(),"Solar selection opens another view");
            check(autoSolar.state().approached==physical.id&&autoSolar.state().camera.distance==before.distance,"Solar selection does not ease its approach");
            for(int i=0;i<400;++i) autoSolar.update(.01);
            double radius=solarBodyRadius(physical.id);
            check(autoSolar.state().camera.distance>radius*3&&autoSolar.state().camera.distance<radius*16,"Solar approach ignores body size or rings");
            for(auto dims:{std::pair<int,int>{800,1200},{1600,900}}) {
                mars::ViewLayout layout; layout.marsView=false; layout.solarView=true; layout.resize(dims.first,dims.second,1);
                auto scene=makeScene(autoSolar,layout);
                bool centred=false;
                for(auto label:scene.labels) if(label.id==physical.id) {
                    centred=label.visible&&std::abs(label.x-layout.width()*.5)<.01&&std::abs(label.y-layout.height()*.5)<.01;
                    check(pickBody(scene,layout,label.x,label.y,false)==physical.id,"Approached solar body is not pickable");
                }
                check(centred,"Solar body loses precision or clips at close range");
                check(scene.camera.distance-radius>scene.camera.clipNear,"Solar approach clips body");
                for(auto body:scene.bodies) {
                    check(std::abs(body.radius-solarBodyRadius(body.id))<1e-10,"Approach changes physical size");
                    auto original=body.centre+scene.renderOrigin;
                    check(std::sqrt(mars::dot(original-mapPosition(BodyId::Sun,body.id),original-mapPosition(BodyId::Sun,body.id)))<1e-6,"Rebasing changes physical position");
                }
                check(std::sqrt(mars::dot(scene.orbitCentre+scene.renderOrigin,scene.orbitCentre+scene.renderOrigin))<1e-9,"Orbit guides do not follow solar origin");
            }
            auto saved=autoSolar.state().camera;
            autoSolar.open(); finish(autoSolar); autoSolar.up(); finish(autoSolar);
            check(autoSolar.state().approached==physical.id&&std::abs(autoSolar.state().camera.distance-saved.distance)<1e-12,"Solar approach lost on return");
            autoSolar.frameSolar(false);
            check(autoSolar.state().approached==BodyId::None&&autoSolar.state().target.x==0&&autoSolar.state().target.y==0&&autoSolar.state().target.z==0,"Inner solar frame retains planet focus");
            autoSolar.selectAndApproach(physical.id); autoSolar.frameSolar(true);
            check(autoSolar.state().approached==BodyId::None&&autoSolar.state().target.distance==solarOverviewDistance,"Whole solar frame retains planet focus");
            autoSolar.selectAndApproach(physical.id); autoSolar.resetCamera();
            check(autoSolar.state().approached==BodyId::None&&autoSolar.state().target.x==0&&autoSolar.state().target.distance==solarOverviewDistance,"Solar Home retains planet focus");
        }
        Navigation selection; selection.start({}); selection.cycleSelection(1);
        check(selection.state().selected==BodyId::Mercury&&selection.state().approached==BodyId::Mercury,"Keyboard selection does not approach");
        selection.selectAndApproach(BodyId::Moon);
        check(selection.state().selected==BodyId::Mercury,"Invalid selection changes focus");
        auto g=makePlaceholderGeometry(); check(g.sphere.size()/3==960,"Unexpected shared mesh cost");
        for(auto p:g.sphere) check(std::abs(mars::dot(p,p)-1)<1e-5,"Sphere must be unit sized");
        for(int width:{800,1242,2560}) for(int height:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout layout; layout.marsView=false; layout.resize(width,height,scale);
            check(layout.openBody().y+layout.openBody().h<=layout.readout().y,"Navigation controls overlap readout");
            for(const auto& b:bodies()) {
                if(b.kind==BodyKind::Region) continue;
                nav.start({ViewKind::Globe,b.id}); auto scene=makeScene(nav,layout);
                check(scene.bodies.size()==1,"Unexpected globe bodies");
                Navigation fresh; fresh.start({ViewKind::Globe,b.id});
                auto fitted=makeScene(fresh,layout);
                double radius=b.style==2?1.9:1.0;
                double halfFov=std::atan(fitted.camera.lens*std::min(1.f,float(width)/height));
                check(std::asin(radius/fitted.camera.distance)<halfFov,"Default globe/rings must fit portrait and landscape");
                auto l=scene.labels.front();
                check(pickBody(scene,layout,l.x,l.y,false)==b.id,"Globe picking mismatch");
            }
            Navigation overview; overview.start({}); auto scene=makeScene(overview,layout);
            check(scene.bodies.size()==9,"Overview contains moons");
            check(scene.trueSolarScale,"Overview is not physical scale");
            double halfFov=std::atan(scene.camera.lens*std::min(1.f,float(width)/height));
            check(std::asin(laneRadius(BodyId::Sun,7)/scene.camera.distance)<halfFov,"Neptune orbit does not fit whole-system camera");
            for(const auto& b:scene.bodies) {
                const auto& p=solarBody(b.id);
                check(std::abs(b.radius/solarUnitsPerAu*astronomicalUnitKm-p.radiusKm)<.1,"Overview uses schematic body size");
                check(std::abs(std::sqrt(mars::dot(b.centre,b.centre))/solarUnitsPerAu-p.orbitAu)<1e-5,"Overview uses schematic orbital distance");
            }
            for(const auto& l:scene.labels) {
                check(l.visible,"Subpixel planet locator missing");
                check(pickBody(scene,layout,l.x,l.y,false)==l.id,"Subpixel planet locator unpickable");
            }
            for(const auto& l:scene.labels) if(l.shown) {
                check(pickBody(scene,layout,l.rect.x+l.rect.w*.5f,l.rect.y+10,true)==l.id,"Label picking mismatch");
            }
            check(pickBody(scene,layout,30,130,true)==BodyId::None,"HUD did not block scene pick");
            layout.solarView=true;
            for(auto r:{layout.solarWhole(),layout.solarInner()}) {
                check(layout.overHud(r.x+1,r.y+1),"Solar framing control does not capture input");
                check(r.y+r.h<layout.legend().y,"Solar framing controls overlap legend");
            }
            layout.solarView=false;
            SceneFrame overlap;
            overlap.bodies={{BodyId::Jupiter,{0,0,0},.2f},{BodyId::Europa,overlap.camera.outward()*.8f,.1f}};
            check(pickBody(overlap,layout,layout.width()*.5f,layout.height()*.5f,false)==BodyId::Europa,"Nearest overlapping body must win");
            layout.earthControls=true; layout.bodyRows=2;
            Navigation earthFrame; earthFrame.start({ViewKind::System,BodyId::Earth});
            auto e=makeScene(earthFrame,layout);
            check(e.trueEarthScale&&e.bodies.size()==2,"Earth system scale/body count");
            check(std::asin((laneRadius(BodyId::Earth,0)+mr)/e.camera.distance)<halfFov,"Lunar orbit does not fit the camera");
            for(const auto& l:e.labels) {
                check(l.visible,"Earth system body locator missing");
                check(pickBody(e,layout,l.x,l.y,false)==l.id,"Earth system body unpickable");
                if(l.shown) check(pickBody(e,layout,l.rect.x+2,l.rect.y+2,true)==l.id,"Earth system label unpickable");
            }
            layout.earthControls=false; layout.bodyRows=9;
            Navigation system; system.start({ViewKind::System,BodyId::Jupiter});
            auto j=makeScene(system,layout);
            check(j.bodies.size()==5&&j.labels.size()==5,"Jovian body projection lost");
            for(const auto& label:j.labels) {
                check(pickBody(j,layout,label.x,label.y,false)==label.id,"True-size moon centre unpickable");
                if(label.shown) check(pickBody(j,layout,label.rect.x+2,label.rect.y+2,true)==label.id,"True-size moon label unpickable");
            }
        }
        Navigation martian; martian.start({ViewKind::System,BodyId::Mars});
        check(std::abs(martian.camera().distance-marsSystemDistance)<1e-6,"Mars whole-system framing");
        check(std::abs(systemBodyRadius(BodyId::Mars,BodyId::Mars)*marsSystemKmPerUnit-marsRadiusKm)<.001,"Mars physical radius");
        check(std::abs(laneRadius(BodyId::Mars,0)*marsSystemKmPerUnit-9375)<.001&&std::abs(laneRadius(BodyId::Mars,1)*marsSystemKmPerUnit-23457)<.002,"Mars physical orbital distances");
        check(std::abs(laneRadius(BodyId::Mars,1)/laneRadius(BodyId::Mars,0)-23457./9375)<1e-6,"Mars orbit proportions");
        for(int w:{800,1600,2560}) for(int h:{500,1000,1600}) {
            mars::ViewLayout l; l.resize(w,h,1); l.marsView=false; l.marsSystem=true;
            auto scene=makeScene(martian,l); check(scene.trueMarsScale&&scene.bodies.size()==3,"Mars system physical scene");
            check(scene.bodies[1].radius<scene.bodies[0].radius*.005&&scene.bodies[2].radius<scene.bodies[0].radius*.003,"Mars moons still inflated");
            for(const auto& label:scene.labels) if(label.shown) check(pickBody(scene,l,float(label.rect.x+1),float(label.rect.y+1),true)==label.id,"Mars label picking at physical size");
            for(auto r:{l.marsWhole(),l.marsApproach()}) check(l.overHud(r.x+1,r.y+1),"Mars framing controls not captured");
        }
        martian.frameMars(false); for(int i=0;i<200;++i) martian.update(.01);
        check(std::abs(martian.camera().distance-marsApproachDistance)<1e-6,"Mars approach framing");
        martian.select(BodyId::Phobos); martian.open(); finish(martian); martian.up(); finish(martian);
        check(martian.state().selected==BodyId::Phobos&&std::abs(martian.camera().distance-marsApproachDistance)<1e-6,"Mars moon return framing");
        martian.zoom(1000); martian.update(.25); check(martian.state().target.distance==marsSystemMinDistance,"Mars close zoom limit");
        martian.zoom(-1000); martian.update(.25); check(martian.state().target.distance==marsSystemMaxDistance,"Mars far zoom limit");
        std::cout<<"PASS: catalog, hierarchy, transitions, saved cameras, all bodies, Mars routing, mesh, selection and scaled UI\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
