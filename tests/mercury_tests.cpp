#include "mercury.hpp"
#include "catalog.hpp"
#include "navigation.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
namespace {
void check(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
void near(float a,float b) { check(std::abs(a-b)<.002f,"Mercury sample mismatch"); }
template<class F> void rejects(F f) { bool caught=false; try { f(); } catch(const std::exception&) { caught=true; } check(caught,"Invalid data accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("mercury-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dem");
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc>=2,"Label argument required");
        auto label=mars::readText(argv[1]); mercury::validateLabel(label);
        for(auto [from,to]:std::vector<std::pair<std::string,std::string>>{
            {"LSB_INTEGER","MSB_INTEGER"},{"SCALING_FACTOR             = 0.5","SCALING_FACTOR             = 1.0"},
            {"MISSING_CONSTANT           = -32768","MISSING_CONSTANT           = 0"},{"A_AXIS_RADIUS                = 2439.4","A_AXIS_RADIUS                = 2440.0"},
            {"EAST\r","WEST\r"},{"11519.5","11520.5"},{"64.0 <pix/deg>","16.0 <pix/deg>"},{"V02","V01"}}) {
            auto bad=label; auto offset=bad.find(from); check(offset!=std::string::npos,"Test label fixture changed");
            bad.replace(offset,from.size(),to); rejects([&] { mercury::validateLabel(bad); });
        }
        rejects([&] { mercury::validateLabel(label+"\nLINE_PREFIX_BYTES = 2\n"); });
        rejects([&] { mercury::validateLabel(label+"\nOFFSET = 1\n"); });
        // One degree analytic field in half-metre DN, exercising signed values and cell centres.
        std::vector<std::int16_t> samples(360*180);
        for(int y=0;y<180;++y) for(int x=0;x<360;++x) samples[y*360+x]=std::int16_t(2*y+x-1000);
        mercury::HeightField field(1,samples);
        near(field.sample(79.5,20.5),-480);
        near(field.sample(79,21),-479.25f);
        near(field.sample(79.5,0),-400.25f);
        near(field.sample(10,0),field.sample(10,360)); near(field.sample(10,-1),field.sample(10,359));
        near(field.sample(90,0),-410.25f); near(field.sample(90,143),field.sample(90,0));
        near(field.sample(-90,0),-231.25f); near(field.sample(-90,250),field.sample(-90,0));
        near(field.sample(120,20),field.sample(90,20));
        rejects([&] { field.sample(std::numeric_limits<double>::infinity(),0); });
        rejects([&] { mercury::HeightField invalid(1,{0}); });
        auto mesh=mars::makeSampledGlobe(72,[&](double lat,double lon) { return field.sample(lat,lon); });
        std::map<std::pair<std::uint32_t,std::uint32_t>,int> edges;
        for(std::size_t i=0;i<mesh.indices.size();i+=3) {
            auto a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];
            auto p=mesh.vertices[a].direction,q=mesh.vertices[b].direction,r=mesh.vertices[c].direction;
            check(mars::dot(mars::cross(q-p,r-p),p)>0,"Bad Mercury winding");
            for(auto e:{std::make_pair(a,b),std::make_pair(b,c),std::make_pair(c,a)}) { if(e.first>e.second) std::swap(e.first,e.second); ++edges[e]; }
        }
        for(auto e:edges) check(e.second==2,"Seam or pole crack");
        samples[10*360+20]=mercury::missing;
        mercury::HeightField gap(1,std::move(samples));
        check(gap.missingCount()==1,"Missing sample count");
        check(std::isnan(gap.sample(79.5,20.5)),"Missing value became elevation");
        near(gap.sample(79.5,21.5),-479.5f); // Exact valid sample adjacent to missing value.
        check(std::isnan(gap.sample(79.5,21)),"Interpolated across unobserved sample");
        auto missingMesh=mars::makeSampledGlobe(72,[&](double lat,double lon) { return lon<10?std::numeric_limits<float>::quiet_NaN():field.sample(lat,lon); });
        check(missingMesh.indices.size()<mesh.indices.size(),"Missing region was not omitted");
        for(auto i:missingMesh.indices) check(std::isfinite(missingMesh.vertices[i].meters),"Missing triangle submitted");
        check(mercury::minimumDistance-(1+mercury::maximumMeters*10/mercury::radiusMeters)>mars::Camera::nearPlane,"Mercury near plane clips relief");
        Scratch tmp;
        { std::ofstream f(tmp.path,std::ios::binary); f<<"MERCDEM2"; }
        rejects([&] { mercury::HeightField::load(tmp.path); });
        // Correct-sized sparse fixture with an invalid header must fail before data allocation.
        { std::ofstream f(tmp.path,std::ios::binary|std::ios::trunc); f<<"BADMAGIC"; f.seekp(530841631); f.put(0); }
        rejects([&] { mercury::HeightField::load(tmp.path); });
        // Valid header but wrong payload checksum: exercises streaming integrity check.
        { std::fstream f(tmp.path,std::ios::binary|std::ios::in|std::ios::out);
          f.write("MERCDEM2",8);
          for(std::uint32_t n:{1u,23040u,11520u,64u,123u,0u}) for(int i=0;i<4;++i) f.put(char(n>>(8*i)));
        }
        rejects([&] { mercury::HeightField::load(tmp.path); });
        check(std::string(solar::body(solar::BodyId::Mercury).dataset)=="mdis64","Missing Mercury catalog data");
        check(solar::mercuryDataset().referenceRadiusKm==2439.4,"Wrong catalog datum");
        solar::Navigation nav; nav.start({solar::ViewKind::Globe,solar::BodyId::Mercury});
        check(nav.mercury()&&nav.detailed()&&!nav.mars(),"Mercury route");
        nav.up(); nav.update(.25); nav.update(.25); check(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Mercury},"Mercury return context");
        nav.open(); nav.update(.25); nav.update(.25); check(nav.mercury(),"Mercury reentry");
        if(argc==3) {
            auto real=mercury::HeightField::load(argv[2]);
            check(real.missingCount()==0,"Unexpected V02 missing samples");
            // Independently decoded from four raw little-endian IMG neighbours per location.
            const float expected[]={-441.591f,-2047.700f,-5276.3486f,-1060.5806f};
            for(int i=0;i<4;++i) {
                auto l=mercury::landmarks[i]; float h=real.sample(l.lat,l.lon); near(h,expected[i]);
                std::cout<<l.name<<": "<<h<<" m\n";
            }
            near(real.sample(12,0),real.sample(12,360)); near(real.sample(90,0),real.sample(90,250));
        }
        std::cout<<"PASS Mercury label, sample scaling, seam, poles, missing data, mesh topology, file validation, navigation\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
