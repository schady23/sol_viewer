#include "moon.hpp"
#include "catalog.hpp"
#include "navigation.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
namespace {
void check(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
void near(float a,float b) { check(std::abs(a-b)<.002f,"Moon sample mismatch"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught,"Invalid data accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("moon-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dem");
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc>=2,"Label argument required");
        auto label=mars::readText(argv[1]); moon::validateLabel(label);
        for(auto [from,to]:std::vector<std::pair<std::string,std::string>>{
            {"LSB_INTEGER","MSB_INTEGER"},{"SCALING_FACTOR        = 0.5","SCALING_FACTOR        = 1.0"},
            {"OFFSET                = 1737400.","OFFSET                = 0."},
            {"EAST","WEST"},{"5759.5","5760.5"},{"64 <pix/deg>","16 <pix/deg>"},{"V3.1","V2.0"},
            {"MEAN EARTH/POLAR AXIS OF DE421","PRINCIPAL AXIS"}}) {
            auto bad=label; auto offset=bad.find(from); check(offset!=std::string::npos,"Test label fixture changed");
            bad.replace(offset,from.size(),to); rejects([&] { moon::validateLabel(bad); });
        }
        for(auto extra:{"LINE_PREFIX_BYTES = 2","OFFSET = 1","MISSING_CONSTANT = -32768","INVALID_CONSTANT = 0"}) {
            rejects([&] { moon::validateLabel(label+"\n"+extra+"\n"); });
        }
        // One degree analytic field in half-metre DN, exercising signed values and cell centres.
        std::vector<std::int16_t> samples(360*180);
        for(int y=0;y<180;++y) for(int x=0;x<360;++x) samples[y*360+x]=std::int16_t(2*y+x-1000);
        moon::HeightField field(1,samples);
        near(field.sample(79.5,20.5),-480);
        near(field.sample(79,21),-479.25f);
        near(field.sample(79.5,0),-400.25f);
        near(field.sample(10,0),field.sample(10,360)); near(field.sample(10,-1),field.sample(10,359));
        near(field.sample(90,0),-410.25f); near(field.sample(90,143),field.sample(90,0));
        near(field.sample(-90,0),-231.25f); near(field.sample(-90,250),field.sample(-90,0));
        near(field.sample(120,20),field.sample(90,20));
        rejects([&] { field.sample(std::numeric_limits<double>::infinity(),0); });
        rejects([&] { moon::HeightField invalid(1,{0}); });
        auto mesh=mars::makeSampledGlobe(72,[&](double lat,double lon) { return field.sample(lat,lon); });
        std::map<std::pair<std::uint32_t,std::uint32_t>,int> edges;
        for(std::size_t i=0;i<mesh.indices.size();i+=3) {
            auto a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];
            auto p=mesh.vertices[a].direction,q=mesh.vertices[b].direction,r=mesh.vertices[c].direction;
            check(mars::dot(mars::cross(q-p,r-p),p)>0,"Bad Moon winding");
            for(auto e:{std::make_pair(a,b),std::make_pair(b,c),std::make_pair(c,a)}) { if(e.first>e.second) std::swap(e.first,e.second); ++edges[e]; }
        }
        for(auto e:edges) check(e.second==2,"Seam or pole crack");
        for(auto invalid:{-32768,32767,-18252,21547}) {
            auto bad=samples; bad[0]=std::int16_t(invalid);
            rejects([&] { moon::HeightField grid(1,std::move(bad)); });
        }
        check(moon::minimumDistance-(1+moon::maximumMeters*10/moon::radiusMeters)>mars::Camera::nearPlane,"Moon near plane clips relief");
        moon::HeightField flat(1,std::vector<std::int16_t>(360*180,200));
        auto slopes=moon::polarSlopes(flat);
        check(slopes.size()==std::size_t(moon::polarSize)*moon::polarSize*6,"Polar shading size");
        for(auto slope:slopes) check(std::isfinite(slope)&&std::abs(slope)<1e-7,"Flat data gained polar slopes");
        auto gradients=moon::polarSlopes(field);
        for(auto v:gradients) check(std::isfinite(v),"Invalid polar shading at seam");
        Scratch tmp;
        { std::ofstream f(tmp.path,std::ios::binary); f<<"MOONDEM1"; }
        rejects([&] { moon::HeightField::load(tmp.path); });
        // Correct-sized sparse fixture with an invalid header must fail before data allocation.
        { std::ofstream f(tmp.path,std::ios::binary|std::ios::trunc); f<<"BADMAGIC"; f.seekp(530841631); f.put(0); }
        rejects([&] { moon::HeightField::load(tmp.path); });
        // Valid header but wrong payload checksum: exercises streaming integrity check.
        { std::fstream f(tmp.path,std::ios::binary|std::ios::in|std::ios::out);
          f.write("MOONDEM1",8);
          for(std::uint32_t n:{1u,23040u,11520u,64u,123u,0u}) for(int i=0;i<4;++i) f.put(char(n>>(8*i)));
        }
        rejects([&] { moon::HeightField::load(tmp.path); });
        check(std::string(solar::body(solar::BodyId::Moon).dataset)=="lola64","Missing Moon catalog data");
        check(solar::moonDataset().referenceRadiusKm==1737.4,"Wrong catalog datum");
        solar::Navigation nav; nav.start({solar::ViewKind::Globe,solar::BodyId::Moon});
        check(nav.moon()&&nav.detailed()&&!nav.mars(),"Moon route");
        nav.up(); nav.update(.25); nav.update(.25); check(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Earth},"Moon return context");
        nav.open(); nav.update(.25); nav.update(.25); check(nav.moon(),"Moon reentry");
        if(argc==3) {
            auto real=moon::HeightField::load(argv[2]);
            // Independently decoded from the four raw IMG neighbours; no rendered heights.
            const float expected[]={-1925.604346f,-1427.554621f,-1062.166664f,-1924.298058f,8.699955f,-2624.996987f};
            for(int i=0;i<6;++i) {
                auto l=moon::landmarks[i]; float h=real.sample(l.lat,l.lon); near(h,expected[i]);
                std::cout<<l.name<<": "<<h<<" m\n";
            }
            near(real.sample(12,0),real.sample(12,360)); near(real.sample(90,0),real.sample(90,250));
        }
        std::cout<<"PASS Moon label, sample scaling, seam, poles, out-of-range data, mesh topology, file validation, navigation\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
