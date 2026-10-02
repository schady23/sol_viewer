#include "satellites_view.hpp"
#include "placeholders.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
template<class F> void rejects(F fn) { bool failed=false; try { fn(); } catch(const std::exception&) { failed=true; } check(failed,"Invalid moon asset accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("saturn-moon-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Expected asset directory"); using namespace solar;
        const auto& moons=saturnianMoons(); auto childrenIds=children(BodyId::Saturn);
        check(childrenIds.size()==7,"Major Saturn moon catalog");
        check(std::abs(systemBodyRadius(BodyId::Saturn,BodyId::Saturn)*saturnSystemKmPerUnit-60268)<.004,"Saturn equatorial scale");
        for(int i=0;i<7;++i) {
            const auto& m=moons[i]; const auto& d=satellites::definition(m.id);
            check(childrenIds[i]==m.id&&satellites::ids[satellites::slot(m.id)]==m.id,"Saturn ordering or explorer slot");
            check(std::abs(laneRadius(BodyId::Saturn,i)*saturnSystemKmPerUnit-m.orbitKm)<.15,"Saturn orbit scale");
            auto position=mapPosition(BodyId::Saturn,m.id);
            check(std::abs(std::sqrt(mars::dot(position,position))*saturnSystemKmPerUnit-m.orbitKm)<.4,"Moon position differs from mean orbital radius");
            auto mesh=satellites::loadSurface(m.id,std::filesystem::path(argv[1])/d.asset);
            check(mesh.size()>400000&&mesh.size()<1100000,"Unexpected Saturn model size");
            double volume=0,maxRadius=0,red=0,blue=0;
            for(std::size_t j=0;j<mesh.size();j+=3) {
                red+=mesh[j].color[0]; blue+=mesh[j].color[2];
                auto a=mesh[j].direction,b=mesh[j+1].direction,c=mesh[j+2].direction;
                check(mars::dot(mars::cross(b-a,c-a),a)>0,"Degenerate or reversed Saturn moon facet");
                volume+=(double(a.x)*(double(b.y)*c.z-double(b.z)*c.y)+double(a.y)*(double(b.z)*c.x-double(b.x)*c.z)+double(a.z)*(double(b.x)*c.y-double(b.y)*c.x))/6;
                for(int k=0;k<3;++k) {
                    auto p=mesh[j+k].direction; maxRadius=std::max(maxRadius,double(std::sqrt(mars::dot(p,p))));
                    check(mesh[j+k].color==mesh[j].color&&mesh[j+k].color[3]==255,"Invalid flat image colors");
                }
            }
            if(m.id==BodyId::Mimas) check(std::abs(red-blue)/(mesh.size()/3*255)<.08,"Mimas diffuse texture replaced by its blue normal map");
            auto equivalent=std::cbrt(volume*3/(4*mars::pi));
            auto physical=equivalent*systemBodyRadius(BodyId::Saturn,m.id)*saturnSystemKmPerUnit;
            check(std::abs(physical-m.radiusKm)<.002,"Moon volume does not match JPL mean radius");
            check(maxRadius>.9999&&maxRadius<1.0001,"Lost NASA model proportions");
            check(hasGuide(m.id)&&locationCount(m.id)==4&&planetArticle(m.id).facts.size()>=10,"Missing Saturn moon guide");
            for(int j=0;j<4;++j) check(std::string(locationArticle(m.id,j).paragraphs[0]).size()>200,"Missing model view notes");
            satellites::View view{m.id}; view.select(2); auto pose=view.target;
            view.home(); check(view.selected==-1&&view.target.distance==2.8f,"Moon home"); view.key(SDLK_R); check(view.revision==1,"Moon retry");
            Navigation nav; nav.start({ViewKind::Globe,m.id}); check(nav.satellite()&&nav.detailed(),"New moon dispatch");
            nav.up(); for(int j=0;j<60;++j) nav.update(.01);
            check(nav.view()==ViewKey{ViewKind::System,BodyId::Saturn}&&nav.state().selected==m.id,"Moon parent restoration");
            (void)pose;
        }
        check(std::abs(moons[6].orbitKm/moons[5].orbitKm-3561700./1221900.)<1e-9,"Iapetus / Titan distance proportions");
        satellites::View titan{BodyId::Titan}; check(titan.haze,"Titan should open behind haze"); titan.key(SDLK_T); check(!titan.haze,"Titan surface toggle");
        titan.key(SDLK_T); titan.select(0); check(!titan.haze,"Titan preset must reveal model surface");
        satellites::View icy{BodyId::Mimas}; icy.key(SDLK_T); check(icy.haze,"Titan toggle changed another moon");
        Navigation system; system.start({ViewKind::System,BodyId::Saturn});
        mars::ViewLayout layout; layout.resize(2560,1600,1); layout.marsView=false; layout.saturnSystem=layout.saturnControls=layout.guideAvailable=true; layout.bodyRows=8;
        auto scene=makeScene(system,layout); check(scene.trueSaturnScale&&scene.bodies.size()==8&&scene.lanes.size()==7,"Physical Saturn scene");
        for(const auto& l:scene.labels) {
            check(l.visible,"Whole Saturn system does not fit");
            if(l.shown) check(pickBody(scene,layout,l.rect.x+1,l.rect.y+1,true)==l.id,"Tiny moon label selection");
        }
        for(auto r:{layout.saturnWhole(),layout.saturnInner(),layout.saturnApproach()}) check(layout.overHud(r.x+1,r.y+1),"Saturn framing does not capture clicks");
        system.frameSaturn(1); for(int j=0;j<60;++j) system.update(.01); check(std::abs(system.state().camera.distance-saturnInnerDistance)<.01,"Inner moon framing");
        system.frameSaturn(2); for(int j=0;j<200;++j) system.update(.01); auto saved=system.state().camera;
        check(std::abs(saved.distance-saturnApproachDistance)<.01,"Saturn approach framing");
        system.select(BodyId::Titan); system.open(); for(int j=0;j<60;++j) system.update(.01); system.up(); for(int j=0;j<60;++j) system.update(.01);
        check(std::abs(system.state().camera.distance-saved.distance)<1e-5&&system.state().selected==BodyId::Titan,"System pose lost on moon visit");
        system.zoom(1000); for(int j=0;j<60;++j) system.update(.01); check(std::abs(system.state().target.distance-saturnSystemMinDistance)<1e-8,"Saturn near bound");
        system.zoom(-1000); check(system.state().target.distance==saturnSystemMaxDistance,"Saturn far bound");
        Scratch corrupt; auto path=std::filesystem::path(argv[1])/satellites::definition(BodyId::Titan).asset;
        { std::ifstream in(path,std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary); out<<in.rdbuf(); out.put('x'); }
        rejects([&] { satellites::loadSurface(BodyId::Titan,corrupt.path); });
        { std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<"TITAN_MESH 1 999999999 0000000000000000\n"; }
        rejects([&] { satellites::loadSurface(BodyId::Titan,corrupt.path); });
        std::cout<<"Saturn moon sources, shape/volume scale, camera, picking, guides and haze passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
