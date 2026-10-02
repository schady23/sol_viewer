#include "satellites_view.hpp"
#include "navigation.hpp"
#include "placeholders.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* why) { if(!good) throw std::runtime_error(why); }
template<class F> void rejects(F fn) { bool failed=false; try { fn(); } catch(const std::exception&) { failed=true; } check(failed,"Invalid satellite input accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("satellite-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Expected asset directory");
        using namespace solar;
        check(!satellites::supports(BodyId::Io)&&!satellites::supports(BodyId::None),"Existing moon routing changed");
        rejects([] { satellites::definition(BodyId::Io); });
        Scratch corrupt;
        for(auto id:{BodyId::Phobos,BodyId::Deimos,BodyId::Europa,BodyId::Ganymede}) {
            const auto& d=satellites::definition(id);
            auto path=std::filesystem::path(argv[1])/d.asset;
            auto mesh=satellites::loadSurface(id,path,3);
            check(mesh.size()==(id==BodyId::Phobos?1537920:id==BodyId::Deimos?1560576:3840),"Satellite triangle count changed");
            float minRadius=2,maxRadius=0;
            for(std::size_t i=0;i<mesh.size();i+=3) {
                auto a=mesh[i].direction,b=mesh[i+1].direction,c=mesh[i+2].direction;
                check(mars::dot(mars::cross(b-a,c-a),a)>0,"Degenerate or inward satellite triangle");
                for(int j=0;j<3;++j) {
                    auto& v=mesh[i+j]; float r=std::sqrt(mars::dot(v.direction,v.direction));
                    minRadius=std::min(minRadius,r); maxRadius=std::max(maxRadius,r);
                    check(v.color==mesh[i].color&&v.color[3]==255,"Facet colors are not flat");
                    for(int k=0;k<3;++k) check(v.color[k]==std::lround(std::round(v.color[k]/255.*31)/31*255),"Palette not quantized");
                }
            }
            check(maxRadius>.9999f&&maxRadius<1.0001f,"Satellite normalization");
            check(d.model?minRadius<.85f:minRadius>.9999f,"Irregular model was replaced by a sphere");
            if(d.model) {
                double volume=0;
                for(std::size_t i=0;i<mesh.size();i+=3) {
                    auto a=mesh[i].direction,b=mesh[i+1].direction,c=mesh[i+2].direction;
                    volume+=double(a.x)*(double(b.y)*c.z-double(b.z)*c.y)/6+double(a.y)*(double(b.z)*c.x-double(b.x)*c.z)/6+double(a.z)*(double(b.x)*c.y-double(b.y)*c.x)/6;
                }
                double equivalent=std::cbrt(volume*3/(4*mars::pi));
                auto physical=martianMoons().at(id==BodyId::Phobos?0:1);
                double radius=equivalent*systemBodyRadius(BodyId::Mars,id)*marsSystemKmPerUnit;
                check(std::abs(radius-physical.radiusKm)<.0001,"Mars moon mesh volume is not calibrated to its mean radius");
            }
            if(!d.model) {
                auto rgb=faceted::loadRgb(path,d.magic,body(id).name);
                auto pixel=[&](double lat,double lon) {
                    int x=int(mars::wrapLongitude(lon+180)*4096/360);
                    int y=std::clamp(int((90-lat)*2048/180),0,2047);
                    return rgb[(y*4096+x)*3];
                };
                // Pinned image samples protect the actual raster registration,
                // not merely the feature list's west-to-east arithmetic.
                check(id==BodyId::Europa?pixel(-25.2,88.6)==148&&pixel(9.7,87.3)==126:
                      pixel(45,233)==80&&pixel(3.4,200)==95,"USGS mosaic registration changed");
                check(pixel(3.4,200)==pixel(3.4,560),"Atlas longitude seam");
                check(id==BodyId::Europa?pixel(-89.9,0)==28:pixel(90,0)==28,"Missing source coverage not preserved");
                rejects([&] { satellites::loadSurface(id,path,-1); });
                rejects([&] { satellites::loadSurface(id,path,9); });
            }
            rejects([&] { satellites::loadSurface(id,corrupt.path); });
            { std::ifstream in(path,std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary); out<<in.rdbuf(); }
            { std::fstream f(corrupt.path,std::ios::binary|std::ios::in|std::ios::out); f.seekg(-1,std::ios::end); char b=0; f.get(b); f.seekp(-1,std::ios::end); f.put(char(b^1)); }
            rejects([&] { satellites::loadSurface(id,corrupt.path); });
            { std::ifstream in(path,std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<in.rdbuf(); out.put('x'); }
            rejects([&] { satellites::loadSurface(id,corrupt.path); });
            // Header validation must reject count overflow and wrong identity
            // before allocating potentially huge buffers.
            for(auto header:{"PHOBOS_MESH 1 2000001 0000000000000000\n","DEIMOS_MESH 2 3 0000000000000000\n","PHOBOS_MESH 1 4 0000000000000000\n"}) {
                { std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<header; }
                rejects([&] { satellites::loadSurface(id,corrupt.path); });
            }
            satellites::View view(id);
            for(int i=0;i<4;++i) {
                view.key(SDLK_1+i); const auto& region=d.regions[i];
                check(view.selected==i&&view.target.lat==region.lat&&view.target.lon==region.lon,"Satellite preset routing");
                check(std::string(locationArticle(id,i).paragraphs.at(0)).size()>200,"Satellite regional guide missing");
            }
            auto selected=view.selected; auto target=view.target;
            view.key(SDLK_L); view.key(SDLK_R);
            check(!view.lighting&&view.revision==1&&view.selected==selected&&view.target.lon==target.lon,"Lighting/retry changed camera");
            view.zoom(1000); check(view.target.distance==1.18f,"Near camera limit");
            view.zoom(-1000); check(view.target.distance==8.f,"Far camera limit");
            view.drag(12,8,0); check(view.selected==-1&&std::isfinite(view.target.lat),"Drag selection or zero height");
            view.home(); check(view.selected==-1&&view.target.distance==2.8f,"Satellite reset");
            check(hasGuide(id)&&locationCount(id)==4&&planetArticle(id).facts.size()>=8,"Satellite body guide");
            rejects([&] { locationArticle(id,-1); }); rejects([&] { locationArticle(id,4); });
            Navigation nav; nav.start({ViewKind::Globe,id});
            check(nav.satellite()&&nav.detailed(),"Satellite detailed navigation");
            auto crumbs=nav.breadcrumbs(); check(crumbs.size()==3&&crumbs[1].view.focus==body(id).parent,"Satellite parent breadcrumb");
            nav.up(); for(int i=0;i<60;++i) nav.update(.01);
            check(nav.view()==ViewKey{ViewKind::System,body(id).parent}&&nav.state().selected==id,"Satellite return selection");
            auto sceneCamera=nav.camera();
            nav.open(); for(int i=0;i<60;++i) nav.update(.01); check(nav.satellite()&&nav.view().focus==id,"System moon launch");
            nav.up(); for(int i=0;i<60;++i) nav.update(.01);
            check(std::abs(nav.camera().distance-sceneCamera.distance)<.0001,"System camera not restored");
            for(int w:{800,1242,2560}) for(int h:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
                mars::ViewLayout l; l.resize(w,h,scale); l.marsView=false; l.satelliteView=true;
                l.guideAvailable=l.locationAvailable=l.locationOpen=true;
                check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Location notes cover coordinates");
                check(l.ioPanel().y+l.ioPanel().h<l.legend().y,"Satellite controls overlap legend");
                for(auto r:{l.ioLighting(),l.ioRetry(),l.legend(),l.navigationUp()})
                    check(l.overHud(float(r.x+2),float(r.y+2)),"Satellite HUD click leaked into globe");
                for(int i=0;i<4;++i) {
                    auto r=l.landmarkButton(i); view.click(l,float(r.x+2),float(r.y+2)); check(view.selected==i,"Preset button");
                    for(const auto& line:articleLines(locationArticle(id,i),l.locationColumns())) check(int(line.size())<=l.locationColumns(),"Regional text overflow");
                }
            }
            std::error_code e; std::filesystem::remove(corrupt.path,e);
        }
        // Independent USGS west-to-east feature anchors.
        check(satellites::definition(BodyId::Europa).regions[0].lon==88.6&&satellites::definition(BodyId::Europa).regions[1].lon==87.3,"Europa east-longitude registration");
        check(satellites::definition(BodyId::Ganymede).regions[0].lon==233&&satellites::definition(BodyId::Ganymede).regions[1].lon==200,"Ganymede east-longitude registration");
        std::cout<<"Four satellite assets, shapes, controls, guides, layout and navigation passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
