#include "saturn_view.hpp"
#include "navigation.hpp"
#include "guide.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* why) { if(!good) throw std::runtime_error(why); }
template<class F> void rejects(F fn) { bool failed=false; try { fn(); } catch(const std::exception&) { failed=true; } check(failed,"Invalid Saturn input accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("saturn-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        using namespace saturn;
        check(argc==2,"Expected assets directory");
        Clock a,b; for(int i=0;i<1000;++i) a.advance(.01); for(int i=0;i<400;++i) b.advance(.025);
        check(std::abs(a.seconds-b.seconds)<1e-7&&std::abs(a.seconds-36000)<1e-7,"Clock depends on frame subdivision");
        a.paused=true; auto stopped=a.seconds; a.advance(.1); check(a.seconds==stopped,"Saturn pause advances time");
        a.reset(); check(a.paused&&a.seconds==0&&a.revision==1,"Reset loses pause or retry revision");
        a.paused=false; a.setRate(1); a.advance(.1); a.advance(NAN); a.advance(-1); check(a.seconds==.1,"Bad clock interval");
        rejects([&] { a.setRate(2); }); a.setRate(3600); a.cycleRate(); check(rates[a.speed]==1,"Rate cycle"); a.cycleRate(-1); check(rates[a.speed]==3600,"Reverse rate cycle");
        a.seconds=loopSeconds; check(a.rotation()==0&&a.loopFraction()==0,"Saturn cloud and rotation loop do not close");
        for(double lat:{-90.,-80.,-35.,0.,35.,80.,90.}) check(std::abs(graphicLatitude(centricLatitude(lat))-lat)<1e-8,"Oblate coordinates round trip");
        for(double lat=-90;lat<=90;lat+=.1) check(std::isfinite(jetSpeed(lat))&&std::abs(jetSpeed(lat))<400,"Invalid authored jets");
        auto rings=makeRings(128); check(rings.size()==128*6,"Ring mesh count");
        for(std::size_t i=0;i<rings.size();i+=3) {
            auto aa=rings[i],bb=rings[i+1],cc=rings[i+2];
            check(mars::dot(mars::cross(bb-aa,cc-aa),mars::Vec3{0,1,0})>0,"Degenerate or reversed ring triangle");
            for(int j=0;j<3;++j) {
                auto p=rings[i+j]; double r=std::sqrt(mars::dot(p,p));
                check(p.y==0&&(std::abs(r-ringInnerKm/equatorialKm)<1e-6||std::abs(r-ringOuterKm/equatorialKm)<1e-6),"Ring plane or physical radius");
            }
        }
        rejects([] { makeRings(0); }); rejects([] { makeRings(100000); });
        View view; view.clock.seconds=rotationSeconds*.25; auto original=view.pose();
        view.setFrame(Frame::Rotation); check(std::abs(view.pose().lon-original.lon)<1e-7,"Frame change jumps camera");
        view.clock.seconds+=rotationSeconds*.25; auto before=view.pose();
        view.resetAtmosphere(); check(std::abs(view.pose().lon-before.lon)<1e-7&&view.clock.seconds==0,"Atmosphere reset jumps camera");
        for(int i=0;i<4;++i) { view.select(i); check(view.selected==i&&std::abs(graphicLatitude(view.target.lat)-features[i].latitude)<1e-7,"Saturn region target"); }
        view.zoom(1000); check(view.target.distance==1.12f,"Saturn near zoom"); view.zoom(-1000); check(view.target.distance==12,"Saturn far zoom");
        view.drag(3,4,0); check(std::isfinite(view.target.lat)&&view.selected==-1,"Zero-height drag"); view.home(); check(view.target.distance==6.2f&&view.selected==-1,"Saturn home framing");
        for(int w:{800,1242,2560}) for(int h:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout l; l.resize(w,h,scale); l.marsView=false; l.saturnView=l.saturnControls=l.guideAvailable=true; l.locationAvailable=l.locationOpen=true;
            for(auto r:{l.saturnPause(),l.saturnSlower(),l.saturnFaster(),l.saturnFrame(),l.saturnReset()}) {
                check(r.y+r.h<l.legend().y&&l.overHud(r.x+1,r.y+1),"Saturn controls overlap or fail capture");
            }
            check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Saturn notes overlap readout");
            mars::Camera camera{0,0,4};
            auto hit=pickCloud(l.width()*.5f,l.height()*.5f,camera,l);
            check(hit&&std::abs(hit->lat)<1e-6&&std::abs(hit->lon)<1e-6,"Cloud-centre coordinate picking");
            check(!pickCloud(0,0,camera,l),"Ring/background picked as cloud globe");
        }
        using namespace solar;
        Navigation nav; nav.start({ViewKind::Globe,BodyId::Saturn}); check(nav.saturn()&&nav.detailed(),"Saturn globe dispatch");
        nav.up(); for(int i=0;i<60;++i) nav.update(.01); check(nav.view()==ViewKey{ViewKind::System,BodyId::Saturn}&&nav.state().selected==BodyId::Saturn,"Saturn parent restore");
        check(hasGuide(BodyId::Saturn)&&locationCount(BodyId::Saturn)==4&&planetArticle(BodyId::Saturn).facts.size()>=8,"Saturn guide");
        rejects([] { locationArticle(BodyId::Saturn,-1); }); rejects([] { locationArticle(BodyId::Saturn,4); });
        auto root=std::filesystem::path(argv[1]); auto cloud=Image::load(root/"saturn-vtad.rgb"); auto ring=RingImage::load(root/"saturn-rings.rgba");
        check(cloud.width==2048&&cloud.height==1024&&cloud.rgb.size()==2048*1024*3,"Saturn cloud dimensions");
        check(ring.rgba.size()==4096*4&&ring.rgba[3]==0&&ring.rgba[2000*4+3]>240,"Ring transparency/reference strip");
        Scratch bad;
        for(auto filename:{"saturn-vtad.rgb","saturn-rings.rgba"}) {
            auto load=[&] { if(std::string(filename)=="saturn-vtad.rgb") Image::load(bad.path); else RingImage::load(bad.path); };
            { std::ifstream in(root/filename,std::ios::binary); std::ofstream out(bad.path,std::ios::binary); out<<in.rdbuf(); }
            { std::fstream f(bad.path,std::ios::binary|std::ios::in|std::ios::out); f.seekg(-1,std::ios::end); char b=0; f.get(b); f.seekp(-1,std::ios::end); f.put(char(b^1)); }
            rejects(load);
            { std::ifstream in(root/filename,std::ios::binary); std::ofstream out(bad.path,std::ios::binary|std::ios::trunc); out<<in.rdbuf(); out.put('x'); } rejects(load);
            { std::ofstream out(bad.path,std::ios::binary|std::ios::trunc); out<<"SATURN_RGB 1 8192 4096 0000000000000000\n"; } rejects(load);
        }
        std::cout<<"Saturn clock, coordinates, rings, controls, guide, navigation and checked assets passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
