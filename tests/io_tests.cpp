#include "io_view.hpp"
#include "guide.hpp"
#include "navigation.hpp"
#include "placeholders.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* message) { if(!good) throw std::runtime_error(message); }
template<class F> void rejects(F fn) { bool failed=false; try { fn(); } catch(const std::exception&) { failed=true; } check(failed,"Invalid Io asset or guide accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("io-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"Expected Io map path");
        auto map=io::Image::load(argv[1]);
        check(map.rgb.size()==4096*2048*3,"Io map dimensions");
        auto pixel=[&](double lat,double lon,int channel) {
            int x=int(mars::wrapLongitude(lon+180)*io::Image::width/360);
            int y=std::clamp(int((90-lat)*io::Image::height/180),0,io::Image::height-1);
            return map.rgb[(y*io::Image::width+x)*3+channel];
        };
        check(pixel(-20,117,0)>pixel(-20,117,1)+25,"Pele red deposits misregistered");
        check(pixel(13,49,0)<100&&pixel(13,51,0)>130,"Loki Patera dark floor / bright island misregistered");
        check(pixel(13.01,51.21,0)==pixel(13.01,411.21,0),"Longitude wrapping");
        auto facets=io::makeFacetedGlobe(map,3);
        check(facets.size()==20*64*3,"Io geodesic triangle count");
        for(std::size_t i=0;i<facets.size();i+=3) {
            auto a=facets[i].direction,b=facets[i+1].direction,c=facets[i+2].direction;
            check(mars::dot(mars::cross(b-a,c-a),a)>0,"Io triangle winding or degenerate facet");
            for(int j=0;j<3;++j) {
                auto& v=facets[i+j];
                check(std::abs(mars::dot(v.direction,v.direction)-1)<.00001,"Io sphere radius changed");
                check(v.color==facets[i].color&&v.color[3]==255,"Io colors are not flat per triangle");
                for(int channel=0;channel<3;++channel) {
                    auto quantized=std::lround(std::round(v.color[channel]/255.*31)/31*255);
                    check(v.color[channel]==quantized,"Io palette is not 5-bit color");
                }
            }
        }
        rejects([&] { io::makeFacetedGlobe(map,9); });
        rejects([] { io::makeFacetedGlobe(io::Image{}); });
        Scratch corrupt;
        rejects([&] { io::Image::load(corrupt.path); });
        { std::ifstream in(argv[1],std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary); out<<in.rdbuf(); }
        { std::fstream f(corrupt.path,std::ios::binary|std::ios::in|std::ios::out); f.seekg(-1,std::ios::end); char b=0; f.get(b); f.seekp(-1,std::ios::end); f.put(char(b^1)); }
        rejects([&] { io::Image::load(corrupt.path); });
        for(auto header:{"IO_RGB 2 4096 2048 0000000000000000\n","IO_RGB 1 2048 4096 0000000000000000\n",
                         "IO_RGB 1 4096 2048 zzzzzzzzzzzzzzzz\n","IO_RGB 1 4096 2048 0000000000000000\n"}) {
            { std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<header; }
            rejects([&] { io::Image::load(corrupt.path); });
        }
        io::View view;
        for(int i=0;i<4;++i) {
            view.key(SDLK_1+i); const auto& r=io::regions[i];
            check(view.selected==i&&view.target.lat==r.lat&&view.target.lon==r.lon,"Io region target");
        }
        view.camera=view.target; auto pose=view.camera; auto revision=view.revision;
        view.key(SDLK_L); view.key(SDLK_R);
        check(!view.lighting&&view.revision==revision+1,"Lighting or retry control");
        check(view.camera.lat==pose.lat&&view.camera.lon==pose.lon&&view.selected==3,"Controls moved the camera");
        view.zoom(1000); check(view.target.distance==1.18f,"Io close zoom limit");
        view.zoom(-1000); check(view.target.distance==8.f,"Io far zoom limit");
        view.drag(5,5,0); check(view.selected==-1&&std::isfinite(view.target.lat),"Io drag/selection");
        view.home(); check(view.target.lon==100&&view.target.lat==-8&&view.target.distance==2.8f,"Io home");
        using namespace solar;
        Navigation nav; nav.start({ViewKind::Globe,BodyId::Io});
        check(nav.io()&&nav.detailed(),"Io not routed as a detailed globe");
        auto crumbs=nav.breadcrumbs(); check(crumbs.size()==3&&crumbs[1].view.focus==BodyId::Jupiter,"Io breadcrumbs");
        nav.up(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.view()==ViewKey{ViewKind::System,BodyId::Jupiter}&&nav.state().selected==BodyId::Io,"Io parent selection");
        nav.frameJovian(false); for(int i=0;i<200;++i) nav.update(.01);
        auto distance=nav.camera().distance; nav.open(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.io(),"Jupiter system did not open Io");
        nav.up(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.state().selected==BodyId::Io&&std::abs(nav.camera().distance-distance)<.0001,"Jupiter framing lost after Io visit");
        check(hasGuide(BodyId::Io)&&locationCount(BodyId::Io)==4,"Io guide routing");
        check(planetArticle(BodyId::Io).facts.size()>=8,"Io reference facts");
        rejects([] { locationArticle(BodyId::Io,4); });
        for(int w:{800,1242,2560}) for(int h:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout l; l.resize(w,h,scale); l.marsView=false; l.ioView=true;
            l.guideAvailable=l.locationAvailable=l.locationOpen=true;
            check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Io notes cover coordinates");
            check(l.ioPanel().y+l.ioPanel().h<l.legend().y,"Io controls cover legend");
            for(auto r:{l.ioLighting(),l.ioRetry(),l.legend(),l.navigationUp()})
                check(l.overHud(float(r.x+2),float(r.y+2)),"Io HUD click not captured");
            auto r=l.ioLighting(); bool before=view.lighting;
            view.click(l,float(r.x+2),float(r.y+2)); check(view.lighting!=before,"Io lighting button");
            for(int i=0;i<4;++i) {
                auto button=l.landmarkButton(i); view.click(l,float(button.x+2),float(button.y+2));
                check(view.selected==i,"Io region button");
                const auto& article=locationArticle(BodyId::Io,i);
                check(article.paragraphs.size()==1&&std::string(article.paragraphs[0]).size()>200,"Io region notes");
                for(const auto& line:articleLines(article,l.locationColumns())) check(int(line.size())<=l.locationColumns(),"Io text overflow");
            }
        }
        std::cout<<"Io source orientation, validation, controls, guide and Jupiter navigation passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
