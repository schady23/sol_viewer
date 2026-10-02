#include "satellites_view.hpp"
#include "placeholders.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void finish(solar::Navigation& nav) { for(int i=0;i<80;++i) nav.update(.02); }
double volume(const std::vector<faceted::FacetVertex>& mesh) {
    double v=0;
    for(std::size_t j=0;j<mesh.size();j+=3) {
        auto a=mesh[j].direction,b=mesh[j+1].direction,c=mesh[j+2].direction;
        v+=(double(a.x)*(double(b.y)*c.z-double(b.z)*c.y)+double(a.y)*(double(b.z)*c.x-double(b.x)*c.z)+double(a.z)*(double(b.x)*c.y-double(b.y)*c.x))/6;
    }
    return v;
}
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Expected asset directory");
        using namespace solar;
        for(auto planet:{BodyId::Uranus,BodyId::Neptune}) {
            check(children(planet).size()==(planet==BodyId::Uranus?5:2),"Outer moon catalog order");
            auto cloud=satellites::loadSurface(planet,std::filesystem::path(argv[1])/satellites::definition(planet).asset);
            check(cloud.size()==96768*3,"Pinned NASA cloud shell changed");
            auto shell=satellites::makeCloudShell(planet);
            check(shell.size()==589824*3,"High-detail shell triangle budget changed");
            float q=float(ice::polarKm(planet)/ice::equatorialKm(planet));
            for(std::size_t i=0;i<shell.size();i+=37) {
                auto p=shell[i].direction;
                check(std::abs(p.x*p.x+p.y*p.y/(q*q)+p.z*p.z-1)<2e-6,"Dense shell changes physical ellipsoid");
            }
            auto name=planet==BodyId::Uranus?"uranus-clouds.rgb":"neptune-clouds.rgb";
            auto magic=planet==BodyId::Uranus?"URANUS_CLOUD_RGB":"NEPTUNE_CLOUD_RGB";
            auto pixels=faceted::loadRgb(std::filesystem::path(argv[1])/name,magic,body(planet).name,2048,1024);
            check(pixels.size()==2048*1024*3,"Cloud atlas dimensions");
            bool rejected=false;
            try { faceted::loadRgb(std::filesystem::path(argv[1])/name,"WRONG_CLOUD",body(planet).name,2048,1024); } catch(const std::exception&) { rejected=true; }
            check(rejected,"Cloud atlas identity mismatch accepted");
            float pole=0,equator=0;
            for(const auto& v:cloud) { pole=std::max(pole,std::abs(v.direction.y)); equator=std::max(equator,std::sqrt(v.direction.x*v.direction.x+v.direction.z*v.direction.z)); }
            check(std::abs(pole/equator-ice::polarKm(planet)/ice::equatorialKm(planet))<4e-5,"Cloud shell flattening or axis conversion wrong");
            check(std::abs(systemBodyRadius(planet,planet)*outerSystemKmPerUnit-ice::equatorialKm(planet))<.002,"Planet physical scale");
            auto axis=ice::rotate({0,1,0},ice::tilt(planet));
            check(std::abs(mars::dot(axis,{0,1,0})-std::cos(ice::tilt(planet)*mars::radians))<1e-6,"Physical axial tilt");
            ice::Clock clock,other;
            for(int i=0;i<1000;++i) clock.advance(.01,planet);
            for(int i=0;i<400;++i) other.advance(.025,planet);
            check(std::abs(clock.seconds-other.seconds)<1e-6,"Frame-rate-dependent atmospheric time");
            clock.paused=true; auto before=clock.seconds; clock.advance(.1,planet); check(clock.seconds==before,"Pause advances gas");
            clock.paused=false; clock.seconds=ice::rotationSeconds(planet)*10-360;
            clock.advance(.1,planet); check(std::abs(clock.seconds)<1e-6,"Loop does not close");
            satellites::View globe(planet); globe.key(SDLK_SPACE); globe.key(SDLK_S); globe.key(SDLK_R);
            check(globe.clock.paused&&globe.clock.speed==0&&globe.clock.seconds==0,"Atmosphere controls");
            for(int i=0;i<4;++i) {
                globe.select(i); auto r=satellites::definition(planet).regions[i];
                auto expected=ice::rotate(mars::direction(r.lat,r.lon),ice::tilt(planet));
                check(mars::dot(mars::direction(globe.target.lat,globe.target.lon),expected)>.999999,"Tilted cloud preset points elsewhere");
            }
            check(hasGuide(planet)&&planetArticle(planet).facts.size()>=10,"Ice giant field guide");
            Navigation nav; nav.start({ViewKind::System,planet});
            nav.frameOuter(2); finish(nav); check(std::abs(nav.state().target.distance-.18)<1e-8,"Planet approach framing");
            nav.zoom(1000); finish(nav); check(nav.state().target.distance==.072,"Outer close limit");
            nav.frameOuter(0); finish(nav);
            mars::ViewLayout layout; layout.resize(2560,1600,1); layout.marsView=false; layout.outerSystem=true; layout.guideAvailable=true; layout.bodyRows=int(nav.visibleBodies().size());
            auto scene=makeScene(nav,layout); check(scene.trueOuterScale&&scene.laneTilts.size()==children(planet).size(),"Physical outer scene flags");
            for(std::size_t i=0;i<outerMoons(planet).size();++i) {
                auto m=outerMoons(planet)[i];
                auto mesh=satellites::loadSurface(m.id,std::filesystem::path(argv[1])/satellites::definition(m.id).asset);
                auto equivalent=std::cbrt(volume(mesh)*3/(4*mars::pi));
                check(std::abs(equivalent*systemBodyRadius(planet,m.id)*outerSystemKmPerUnit-m.radiusKm)<.002,"Moon mesh volume not physically calibrated");
                auto position=mapPosition(planet,m.id);
                check(std::abs(std::sqrt(mars::dot(position,position))*outerSystemKmPerUnit-m.orbitKm)<.08,"Orbital distance not physically scaled");
                check(std::abs(scene.laneTilts[i]-ice::tilt(planet)-m.inclination)<1e-5,"Orbit guide inclination mismatch");
                bool found=false;
                for(const auto& label:scene.labels) if(label.id==m.id&&label.visible) {
                    check(pickBody(scene,layout,label.x,label.y,false)==m.id,"Tiny moon locator cannot be selected"); found=true;
                }
                check(found,"Whole system loses moon from view");
                nav.select(m.id); auto camera=nav.state().camera;
                nav.open(); finish(nav); check(nav.satellite()&&nav.detailed(),"Moon explorer navigation");
                nav.up(); finish(nav); check(nav.state().selected==m.id&&std::abs(nav.state().camera.distance-camera.distance)<1e-6,"Returning moon loses system selection or camera");
                check(hasGuide(m.id)&&planetArticle(m.id).facts.size()>=10,"Moon physical guide facts");
            }
            for(auto ring:ice::rings(planet)) check(ring.radiusKm-ring.widthKm/2>ice::equatorialKm(planet)&&ring.widthKm>0,"Ring intersects planet");
        }
        std::cout<<"Outer worlds: sources, sizes, tilted guides, picking, time and navigation passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
