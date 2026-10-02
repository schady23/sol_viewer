#include "callisto_view.hpp"
#include "guide.hpp"
#include "navigation.hpp"
#include "placeholders.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* message) { if(!good) throw std::runtime_error(message); }
template<class F> void rejects(F fn) { bool failed=false; try { fn(); } catch(const std::exception&) { failed=true; } check(failed,"Invalid Callisto asset or guide accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("callisto-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Expected Callisto map path");
        auto map=callisto::Image::load(argv[1]);
        check(map.rgb.size()==4096*2048*3,"Callisto map dimensions");
        auto pixel=[&](double lat,double lon,int channel) {
            int x=int(mars::wrapLongitude(lon+180)*callisto::Image::width/360);
            int y=std::clamp(int((90-lat)*callisto::Image::height/180),0,callisto::Image::height-1);
            return map.rgb[(y*callisto::Image::width+x)*3+channel];
        };
        check(pixel(14.7,304,0)==108&&pixel(32.2,220.1,0)==143,"Valhalla / Asgard registration");
        check(pixel(-56.5,337.7,0)==171&&pixel(-48.5,324.4,0)==126,"Lofn / Adlinda registration");
        check(pixel(14.7,304,0)==pixel(14.7,664,0),"Longitude wrapping");
        check(pixel(90,60,0)==35&&pixel(90,60,1)==36&&pixel(-85,180,0)==35,"Missing coverage must stay flat charcoal");
        check(pixel(-56.5,337.7,0)>pixel(-56.5,337.7,2),"Callisto display tint lost");
        auto facets=callisto::makeFacetedGlobe(map,3);
        check(facets.size()==20*64*3,"Callisto geodesic triangle count");
        for(std::size_t i=0;i<facets.size();i+=3) {
            auto a=facets[i].direction,b=facets[i+1].direction,c=facets[i+2].direction;
            check(mars::dot(mars::cross(b-a,c-a),a)>0,"Callisto triangle winding or degenerate facet");
            for(int j=0;j<3;++j) {
                auto& v=facets[i+j];
                check(std::abs(mars::dot(v.direction,v.direction)-1)<.00001,"Callisto sphere radius changed");
                check(v.color==facets[i].color&&v.color[3]==255,"Callisto colors are not flat per triangle");
                for(int channel=0;channel<3;++channel) {
                    auto quantized=std::lround(std::round(v.color[channel]/255.*31)/31*255);
                    check(v.color[channel]==quantized,"Callisto palette is not 5-bit color");
                }
            }
        }
        rejects([&] { callisto::makeFacetedGlobe(map,9); });
        rejects([&] { callisto::makeFacetedGlobe(map,-1); });
        rejects([] { callisto::makeFacetedGlobe(callisto::Image{}); });
        Scratch corrupt;
        rejects([&] { callisto::Image::load(corrupt.path); });
        { std::ifstream in(argv[1],std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary); out<<in.rdbuf(); }
        { std::fstream f(corrupt.path,std::ios::binary|std::ios::in|std::ios::out); f.seekg(-1,std::ios::end); char b=0; f.get(b); f.seekp(-1,std::ios::end); f.put(char(b^1)); }
        rejects([&] { callisto::Image::load(corrupt.path); });
        { std::ifstream in(argv[1],std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<in.rdbuf(); out.put('x'); }
        rejects([&] { callisto::Image::load(corrupt.path); });
        for(auto header:{"IO_RGB 1 4096 2048 0000000000000000\n","CALLISTO_RGB 2 4096 2048 0000000000000000\n","CALLISTO_RGB 1 2048 4096 0000000000000000\n",
                         "CALLISTO_RGB 1 4096 2048 zzzzzzzzzzzzzzzz\n","CALLISTO_RGB 1 4096 2048 0000000000000000\n"}) {
            { std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<header; }
            rejects([&] { callisto::Image::load(corrupt.path); });
        }
        callisto::View view;
        for(int i=0;i<4;++i) {
            view.key(SDLK_1+i); const auto& r=callisto::regions[i];
            check(view.selected==i&&view.target.lat==r.lat&&view.target.lon==r.lon,"Callisto region target");
        }
        view.camera=view.target; auto pose=view.camera; auto revision=view.revision;
        view.key(SDLK_L); view.key(SDLK_R);
        check(!view.lighting&&view.revision==revision+1,"Lighting or retry control");
        check(view.camera.lat==pose.lat&&view.camera.lon==pose.lon&&view.selected==3,"Controls moved the camera");
        view.zoom(1000); check(view.target.distance==1.18f,"Callisto close zoom limit");
        view.zoom(-1000); check(view.target.distance==8.f,"Callisto far zoom limit");
        view.drag(5,5,0); check(view.selected==-1&&std::isfinite(view.target.lat),"Callisto drag/selection");
        view.home(); check(view.target.lon==304&&view.target.lat==12&&view.target.distance==2.8f,"Callisto home");
        using namespace solar;
        Navigation nav; nav.start({ViewKind::Globe,BodyId::Callisto});
        check(nav.callisto()&&nav.detailed(),"Callisto not routed as a detailed globe");
        auto crumbs=nav.breadcrumbs(); check(crumbs.size()==3&&crumbs[1].view.focus==BodyId::Jupiter,"Callisto breadcrumbs");
        nav.up(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.view()==ViewKey{ViewKind::System,BodyId::Jupiter}&&nav.state().selected==BodyId::Callisto,"Callisto parent selection");
        nav.frameJovian(false); for(int i=0;i<200;++i) nav.update(.01);
        auto distance=nav.camera().distance; nav.open(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.callisto(),"Jupiter system did not open Callisto");
        nav.up(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.state().selected==BodyId::Callisto&&std::abs(nav.camera().distance-distance)<.0001,"Jupiter framing lost after Callisto visit");
        check(hasGuide(BodyId::Callisto)&&locationCount(BodyId::Callisto)==4,"Callisto guide routing");
        check(planetArticle(BodyId::Callisto).facts.size()>=8,"Callisto reference facts");
        rejects([] { locationArticle(BodyId::Callisto,4); });
        rejects([] { locationArticle(BodyId::Callisto,-1); });
        for(int w:{800,1242,2560}) for(int h:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout l; l.resize(w,h,scale); l.marsView=false; l.callistoView=true;
            l.guideAvailable=l.locationAvailable=l.locationOpen=true;
            check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Callisto notes cover coordinates");
            check(l.callistoPanel().y+l.callistoPanel().h<l.legend().y,"Callisto controls cover legend");
            for(auto r:{l.callistoLighting(),l.callistoRetry(),l.legend(),l.navigationUp()})
                check(l.overHud(float(r.x+2),float(r.y+2)),"Callisto HUD click not captured");
            auto r=l.callistoLighting(); bool before=view.lighting;
            view.click(l,float(r.x+2),float(r.y+2)); check(view.lighting!=before,"Callisto lighting button");
            for(int i=0;i<4;++i) {
                auto button=l.landmarkButton(i); view.click(l,float(button.x+2),float(button.y+2));
                check(view.selected==i,"Callisto region button");
                const auto& article=locationArticle(BodyId::Callisto,i);
                check(article.paragraphs.size()==1&&std::string(article.paragraphs[0]).size()>200,"Callisto region notes");
                for(const auto& line:articleLines(article,l.locationColumns())) check(int(line.size())<=l.locationColumns(),"Callisto text overflow");
            }
        }
        std::cout<<"Callisto source orientation, validation, controls, guide and Jupiter navigation passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
