#include "jupiter.hpp"
#include "ui.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* message) { if(!good) throw std::runtime_error(message); }
template<class F> void rejects(F fn) { bool caught=false; try { fn(); } catch(...) { caught=true; } check(caught,"Invalid input accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("jupiter-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        using namespace jupiter;
        Clock a,b; a.setRate(3600); b.setRate(3600);
        for(int i=0;i<1000;++i) a.advance(.01);
        for(int i=0;i<400;++i) b.advance(.025);
        check(std::abs(a.seconds-b.seconds)<1e-6,"Clock depends on frame subdivision");
        check(std::abs(a.seconds-36000)<1e-6,"Wrong time multiplier");
        a.paused=true; auto stopped=a.seconds; a.advance(.25); check(a.seconds==stopped,"Pause advances atmosphere");
        a.reset(); check(a.seconds==0&&a.revision==1&&a.paused,"Reset loses pause or epoch");
        a.paused=false; a.setRate(1); a.advance(.1); check(a.seconds==.1,"Real time wrong");
        a.advance(-1); a.advance(NAN); check(a.seconds==.1,"Bad dt accepted");
        rejects([&] { a.setRate(10); });
        rejects([&] { a.setRate(21600); });
        a.setRate(3600); a.cycleRate(); check(rates[a.speed]==1,"Speed cycle wrap");
        a.cycleRate(-1); check(rates[a.speed]==3600,"Speed reverse wrap");
        a.seconds=rotationSeconds*100+.25*rotationSeconds; check(std::abs(a.rotation()-90)<1e-7,"Rotation phase drifts");
        a.seconds=loopSeconds; check(a.loopFraction()==0&&a.rotation()==0,"Loop does not close both clouds and rotation");
        for(double fraction:{.01,.25,.5,.999}) {
            a.seconds=loopSeconds*fraction; b.seconds=a.seconds+loopSeconds*100;
            check(std::abs(a.loopFraction()-b.loopFraction())<1e-10,"Cloud phase drifts across loops");
            check(std::abs(a.rotation()-b.rotation())<1e-8,"Rotation and cloud loop not synchronized");
        }
        for(double lat:{-90.,-80.,-22.4,0.,24.,80.,90.})
            check(std::abs(graphicLatitude(centricLatitude(lat))-lat)<1e-8,"Latitude convention round trip");
        check(std::abs(centricLatitude(-22.4))<22.4,"Oblate latitude conversion direction");
        check(jetSpeed(-7)>100&&jetSpeed(-20)<0&&jetSpeed(24)>140&&jetSpeed(31)<0,"Opposing jet directions lost");
        check(jetSpeed(90)==0&&jetSpeed(-90)==0,"Polar wind singularity");
        for(double lat=-90;lat<=90;lat+=.01) check(std::isfinite(jetSpeed(lat))&&std::abs(jetSpeed(lat))<=153,"Invalid model wind");
        for(int w:{800,1242,2560}) for(int h:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout l; l.resize(w,h,scale); l.marsView=false; l.jupiterView=l.jupiterControls=l.guideAvailable=true;
            l.locationAvailable=l.locationOpen=true;
            for(auto r:{l.jupiterPause(),l.jupiterSlower(),l.jupiterFaster(),l.jupiterFrame(),l.jupiterReset()}) {
                check(r.x>=0&&r.x+r.w<=l.width()&&r.y+r.h<l.legend().y,"Atmosphere controls overlap bounds");
                check(l.overHud(r.x+1,r.y+1),"Atmosphere control fails pointer capture");
            }
            check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Jupiter notes overlap readout");
        }
        if(argc==2) {
            auto image=Image::load(argv[1]);
            check(image.width==3600&&image.height==1800&&image.rgb.size()==19440000,"Native cloud dimensions");
            auto pixel=[&](double lat,double lon,int channel) { return image.rgb[(int((90-lat)/180*image.height)*image.width+int(lon/360*image.width))*3+channel]; };
            check(pixel(-22.4,233.2,0)>pixel(-22.4,233.2,2)+40,"Red Spot anchor/orientation lost");
            check(pixel(89,100,0)==0&&pixel(-89,100,0)==0,"Missing polar source coverage changed");
            Scratch bad; { std::ofstream out(bad.path); out<<"JUPITER_RGB 1 8192 4096 0000000000000000\n"; }
            rejects([&] { Image::load(bad.path); });
            { std::ofstream out(bad.path); out<<"JUPITER_RGB 2 3600 1800 0000000000000000\n"; }
            rejects([&] { Image::load(bad.path); });
            { std::ofstream out(bad.path,std::ios::binary); out<<"JUPITER_RGB 2 3600 1800 0000000000000000\n";
              out.write(reinterpret_cast<const char*>(image.rgb.data()),std::streamsize(image.rgb.size())); }
            rejects([&] { Image::load(bad.path); });
            { std::ifstream in(argv[1],std::ios::binary); std::ofstream out(bad.path,std::ios::binary); out<<in.rdbuf()<<'x'; }
            rejects([&] { Image::load(bad.path); });
        }
        std::cout<<"PASS: Jupiter clock, pause/reset, rotation, coordinates, jets, scaled UI, source orientation and validation\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
