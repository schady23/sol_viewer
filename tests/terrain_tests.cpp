#include "terrain.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
void near(double a,double b,double eps=0.01) { check(std::abs(a-b)<eps,"Numeric check failed"); }
template<class F> void rejects(F f) {
    bool rejected=false;
    try { f(); } catch(const std::exception&) { rejected=true; }
    check(rejected,"Invalid input was accepted");
}
std::string replace(std::string text,const std::string& from,const std::string& to) {
    auto p=text.find(from); check(p!=std::string::npos,"Test label fixture changed");
    text.replace(p,from.size(),to); return text;
}
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/
        ("mars-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mola");
    ~Scratch() { std::error_code ec; std::filesystem::remove(path,ec); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc==2||argc==3,"Usage: terrain_tests label.lbl [real.mola]");
        auto label=mars::readText(argv[1]); mars::validateMolaLabel(label);
        rejects([&] { mars::validateMolaLabel(replace(label,"MSB_INTEGER","LSB_INTEGER")); });
        rejects([&] { mars::validateMolaLabel(replace(label,"LINE_SAMPLES                 = 5760","LINE_SAMPLES                 = 11520")); });
        rejects([&] { mars::validateMolaLabel(replace(label,"\"EAST\"","\"WEST\"")); });
        rejects([&] { mars::validateMolaLabel(label+"\nMISSING_CONSTANT = -32768\n"); });
        rejects([&] { mars::validateMolaLabel(label+"\nSCALING_FACTOR = 2\n"); });
        near(mars::decodeSigned16(0xff,0xff),-1);
        near(mars::decodeSigned16(0x80,0),-32768);
        near(mars::decodeSigned16(0x7f,0xff),32767);
        // Closest camera clears even the highest 10x terrain and its near plane.
        check(mars::Camera::minDistance-(1+21171.0f*10/mars::radiusMeters)>mars::Camera::nearPlane,
              "Closest zoom can clip exaggerated terrain");
        for(float distance:{mars::Camera::minDistance,2.9f,mars::Camera::maxDistance}) {
            mars::Camera camera{18.65,226.2,distance};
            for(float aspect:{0.8f,1.6f,2.4f}) {
                auto world=camera.outward()*1.04f+camera.right()*0.012f+camera.up()*0.007f;
                float z=distance-mars::dot(world,camera.outward());
                float nx=mars::dot(world,camera.right())/(z*mars::Camera::tanHalfFov*aspect);
                float ny=mars::dot(world,camera.up())/(z*mars::Camera::tanHalfFov);
                float n=mars::Camera::nearPlane,f=mars::Camera::farPlane;
                float depth=((f+n)/(f-n)-2*f*n/((f-n)*z)+1)*0.5f;
                auto restored=camera.unproject(nx,ny,depth,aspect);
                check(mars::dot(mars::normalized(world),mars::normalized(restored))>0.99999f,
                      "Depth picking changes direction with aspect ratio/zoom");
            }
        }
        using H=mars::HeightField;
        std::vector<std::int16_t> samples(std::size_t(H::width)*H::height);
        for(std::uint32_t y=0;y<H::height;++y) for(std::uint32_t x=0;x<H::width;++x)
            samples[std::size_t(y)*H::width+x]=std::int16_t(int(y)*2+int(x)-6000);
        H field(std::move(samples));
        near(field.sample(90-10.5/16,20.5/16),-5960); // exact pixel centre
        near(field.sample(90-11.0/16,21.0/16),-5958.5); // four-cell interpolation
        near(field.sample(90-10.5/16,0),-3100.5); // columns 5759 and 0
        near(field.sample(17,0),field.sample(17,360));
        near(field.sample(17,-1),field.sample(17,359));
        near(field.sample(90,0),field.sample(90,147));
        near(field.sample(-90,0),field.sample(-90,227));
        near(field.sample(90,90),-3120.5);
        near(field.sample(-90,90),2637.5);
        for(double lat:{-90.0,-70.0,0.0,30.0,90.0}) for(double lon:{0.0,90.0,180.0,359.0}) {
            auto d=mars::direction(lat,lon);
            auto p=mars::location(d);
            check(mars::dot(d,mars::direction(p.lat,p.lon))>0.99999f,"Coordinate round trip failed");
        }
        // A closed orientable sphere: every indexed edge appears exactly twice, including seam/poles.
        auto mesh=mars::makeGlobe(field,72);
        std::map<std::pair<std::uint32_t,std::uint32_t>,int> edges;
        for(std::size_t i=0;i<mesh.indices.size();i+=3) {
            auto a=mesh.indices[i],b=mesh.indices[i+1],c=mesh.indices[i+2];
            auto p=mesh.vertices.at(a).direction,q=mesh.vertices.at(b).direction,r=mesh.vertices.at(c).direction;
            check(mars::dot(mars::cross(q-p,r-p),p)>0,"Degenerate or inward terrain triangle");
            for(auto e:{std::pair{a,b},std::pair{b,c},std::pair{c,a}}) { if(e.first>e.second) std::swap(e.first,e.second); ++edges[e]; }
        }
        for(auto [edge,uses]:edges) { (void)edge; check(uses==2,"Open terrain edge / visible crack"); }
        check(int(mesh.vertices.size())-int(edges.size())+int(mesh.indices.size()/3)==2,"Mesh is not spherical");
        // Draw batches must partition all indices and conservatively contain every
        // referenced vertex at both relief settings, including seam and pole fans.
        std::uint32_t next=0;
        for(const auto& b:mesh.batches) {
            check(b.first==next&&b.count%3==0,"Terrain batch index partition"); next+=b.count;
            for(std::size_t k=b.first;k<b.first+b.count;++k) {
                const auto& v=mesh.vertices[mesh.indices[k]];
                check(v.meters>=b.minimum&&v.meters<=b.maximum,"Batch height bound excludes terrain");
                check(mars::dot(v.direction,b.centre)+0.00001f>=std::cos(b.halfAngle),"Batch angular bound excludes terrain");
                for(float exaggeration:{1.0f,10.0f}) {
                    float low=1+b.minimum*exaggeration/mars::radiusMeters,high=1+b.maximum*exaggeration/mars::radiusMeters;
                    float radius=2*high*std::sin(b.halfAngle*0.5f)+(high-low)*0.5f+0.00001f;
                    auto delta=v.direction*(1+v.meters*exaggeration/mars::radiusMeters)-b.centre*((low+high)*0.5f);
                    check(mars::dot(delta,delta)<=radius*radius,"Batch bounding sphere excludes terrain");
                }
            }
        }
        check(next==mesh.indices.size(),"Terrain batches omit triangles");
        mars::Camera front{0,0,2.68f};
        mars::TerrainBatch nearBatch{0,3,mars::direction(0,0),0.01f,0,0};
        mars::TerrainBatch farBatch{0,3,mars::direction(0,180),0.01f,0,0};
        check(mars::batchVisible(nearBatch,front,1,1.6f,1),"Culling rejected visible terrain");
        check(!mars::batchVisible(farBatch,front,1,1.6f,1),"Culling retained opposite hemisphere");
        Scratch scratch; field.save(scratch.path);
        auto loaded=H::load(scratch.path);
        check(loaded.samples()==field.samples(),"Signed elevations changed in round trip");
        {
            std::fstream f(scratch.path,std::ios::binary|std::ios::in|std::ios::out);
            f.seekp(34); f.put(0);
        }
        rejects([&] { H::load(scratch.path); });
        { std::ofstream f(scratch.path,std::ios::binary|std::ios::trunc); f<<"bad"; }
        rejects([&] { H::load(scratch.path); });
        if(argc==3) {
            auto real=H::load(argv[2]);
            auto bounds=std::minmax_element(real.samples().begin(),real.samples().end());
            near(*bounds.first,-8177); near(*bounds.second,21171);
            near(real.sample(18.65,226.20),20037.84,0.1);
            near(real.sample(-42.43,70.50),-6041.13,0.1);
            std::cout<<"Real MOLA range and independent reference samples passed.\n";
        }
        std::cout<<"PASS: label validation, signed bytes, pixel centres, interpolation, seam, poles, closed mesh, checksum, truncation.\n";
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
