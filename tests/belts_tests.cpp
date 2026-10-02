#include "belts.hpp"
#include "placeholders.hpp"
#include "satellites_view.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b,const char* why) { if(!b) throw std::runtime_error(why); }
void settle(solar::Navigation& n) { for(int j=0;j<500;++j) n.update(.01); }
double length(mars::Vec3 p) { return std::sqrt(double(mars::dot(p,p))); }
}
int main(int argc,char** argv) {
    try {
        using namespace solar; check(argc==2,"Expected assets path");
        check(children(BodyId::Sun).size()==8,"Belt regions entered physical planet orbit table");
        check(children(BodyId::AsteroidBelt)==std::vector<BodyId>{BodyId::Ceres},"Ceres belongs to main belt");
        check(children(BodyId::KuiperBelt).size()==4,"Missing outer dwarf planets");
        for(auto id:{BodyId::AsteroidBelt,BodyId::KuiperBelt}) {
            check(hasGuide(id)&&locationCount(id)==0,"Belt guide routes to surface landmarks");
            Navigation n; n.start({}); n.selectAndApproach(id); settle(n);
            check(n.belt()&&n.view()==ViewKey{ViewKind::System,id},"Single POI selection must open region");
            for(auto dims:{std::pair<int,int>{800,500},{900,1600},{2560,1600}}) {
                mars::ViewLayout l; l.marsView=false; l.beltView=l.systemView=true; l.bodyRows=int(n.visibleBodies().size()); l.resize(dims.first,dims.second,1);
                auto scene=makeScene(n,l);
                check(scene.belt==id&&scene.bodies.size()==children(id).size()+1,"Region fabricated a physical central body");
                for(auto label:scene.labels) {
                    check(label.visible,"Region locator outside initial frame");
                    check(pickBody(scene,l,label.x,label.y,false)==label.id,"Region locator is not pickable");
                }
                check(l.openBody().y+l.openBody().h<=l.readout().y,"Region list overlaps readout");
            }
            auto samples=belts::samples(id),again=belts::samples(id); std::array<int,4> families{};
            for(std::size_t j=0;j<samples.size();++j) {
                auto s=samples[j]; check(length(s.point-again[j].point)==0,"Distribution is not reproducible");
                check(length(s.point)>=s.a*(1-s.e)-1e-4&&length(s.point)<=s.a*(1+s.e)+1e-4,"Sample outside its orbital radial limits");
                if(id==BodyId::AsteroidBelt) {
                    check(s.a>=2.1&&s.a<=3.3&&s.i<=20,"Main belt bounds");
                    check(std::abs(s.a-2.5)>=.025&&std::abs(s.a-2.82)>=.022,"Missing semimajor-axis Kirkwood gaps");
                } else { ++families[s.population]; if(s.population==0) check(s.a>=42&&s.a<=47&&s.i<=5,"Cold classical sample"); }
            }
            if(id==BodyId::KuiperBelt) for(auto count:families) check(count>0,"Missing Kuiper orbital family");
            for(auto dwarfId:children(id)) {
                const auto& d=belts::dwarf(dwarfId);
                auto peri=belts::orbit(d.a,d.e,d.i,d.node,d.peri,0,1),apo=belts::orbit(d.a,d.e,d.i,d.node,d.peri,180,1);
                check(std::abs(length(peri)-d.a*(1-d.e))<1e-5&&std::abs(length(apo)-d.a*(1+d.e))<1e-5,"Dwarf ellipse does not match perihelion/aphelion");
                double radius=systemBodyRadius(id,dwarfId);
                check(std::abs(radius/belts::units(id)*astronomicalUnitKm-d.boundingKm)<.001,"Dwarf radius not on orbital distance scale");
                n.selectAndApproach(dwarfId); settle(n);
                auto saved=n.state().camera;
                mars::ViewLayout l; l.marsView=false; l.resize(1600,1000,1); auto scene=makeScene(n,l);
                for(auto label:scene.labels) if(label.id==dwarfId) {
                    check(label.visible&&std::abs(label.x-l.width()*.5)<.01&&std::abs(label.y-l.height()*.5)<.01,"Rebased dwarf approach lost precision");
                    check(pickBody(scene,l,label.x,label.y,false)==dwarfId,"Approached dwarf unpickable");
                }
                check(scene.camera.clipNear<radius&&scene.camera.distance>radius*3,"Dwarf approach clips or ignores radius");
                n.open(); settle(n); check(n.satellite()&&n.view().focus==dwarfId,"Dwarf does not open textured/reference globe");
                check(n.breadcrumbs().size()==3,"Dwarf lost belt breadcrumb");
                n.up(); settle(n); check(n.belt()&&std::abs(n.state().camera.distance-saved.distance)<1e-12,"Belt approach lost on return");
                n.resetCamera(); settle(n); check(n.state().approached==BodyId::None,"Whole-region reset retains dwarf offset");
                const auto& definition=satellites::definition(dwarfId);
                auto mesh=satellites::loadSurface(dwarfId,std::filesystem::path(argv[1])/definition.asset);
                check(mesh.size()>10000,"Missing dwarf visual asset");
                if(dwarfId==BodyId::Haumea) {
                    float x=0,y=0,z=0; for(auto v:mesh) { x=std::max(x,std::abs(v.direction.x)); y=std::max(y,std::abs(v.direction.y)); z=std::max(z,std::abs(v.direction.z)); }
                    check(std::abs(y/x-513./1161)<1e-5&&std::abs(z/x-852./1161)<1e-5,"Haumea reference shape became spherical");
                    satellites::View view(dwarfId); check(view.camera.distance>5,"Haumea ring does not fit initial globe");
                }
            }
            n.up(); settle(n); check(n.view().kind==ViewKind::Solar,"Belt cannot return to Sol");
        }
        mars::ViewLayout l; l.marsView=false; l.solarView=true; l.resize(800,500,1);
        for(auto r:{l.asteroidBelt(),l.kuiperBelt()}) check(l.overHud(r.x+1,r.y+1)&&r.y+r.h<=l.openBody().y,"Belt POI overlaps or leaks input");
        check(l.asteroidBelt().x==l.bodyButton(0).x&&l.asteroidBelt().y>=l.bodyButton(8).y+l.bodyButton(8).h,"Belt POIs are not below planets on the left");
        check(l.openBody().y+l.openBody().h<=l.readout().y,"Solar belt controls overlap readout in small window");
        std::cout<<"PASS: belt POIs, dwarf navigation, physical scale, elliptical paths, distributions, precise approaches and assets\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
