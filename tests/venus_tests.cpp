#include "venus_view.hpp"
#include "navigation.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("venus-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
int main(int argc,char** argv) {
    try {
        check(argc==3,"Expected Venus terrain and surface-color asset paths");
        auto field=venus::HeightField::load(argv[1]);
        auto surface=venus::SurfaceMap::load(argv[2]);
        check(surface.rgb.size()==1440*720*3,"Surface map dimensions");
        auto pixel=[&](double lat,double lon,int channel) {
            int x=int(mars::wrapLongitude(lon+180)*4),y=int((90-lat)*4);
            return surface.rgb[(y*1440+x)*3+channel];
        };
        check(pixel(65.2,3.3,2)>200&&pixel(20,30,2)<60,"Maxwell Montes / surface map orientation");
        Scratch corrupt;
        { std::ifstream in(argv[2],std::ios::binary); std::ofstream out(corrupt.path,std::ios::binary); out<<in.rdbuf(); }
        { std::fstream file(corrupt.path,std::ios::in|std::ios::out|std::ios::binary); file.seekg(-1,std::ios::end); char byte=0; file.get(byte); file.seekp(-1,std::ios::end); file.put(char(byte^1)); }
        bool rejected=false;
        try { venus::SurfaceMap::load(corrupt.path); } catch(const std::exception&) { rejected=true; }
        check(rejected,"Corrupt surface map accepted");
        // Keep the full payload so this exercises the checksum diagnostic,
        // rather than failing earlier on a truncated file.
        { std::fstream file(corrupt.path,std::ios::in|std::ios::out|std::ios::binary);
          file.seekp(std::streamoff(std::string("VENUS_RGB 1 1440 720 ").size())); file<<"zzzzzzzzzzzzzzzz"; }
        rejected=false;
        try { venus::SurfaceMap::load(corrupt.path); } catch(const std::exception& e) {
            rejected=std::string(e.what()).find(corrupt.path.string())!=std::string::npos;
        }
        check(rejected,"Malformed checksum error omitted its path");
        auto rejects=[&](const char* header,bool terrain,const char* why) {
            { std::ofstream out(corrupt.path,std::ios::binary|std::ios::trunc); out<<header; }
            bool failed=false;
            try {
                if(terrain) venus::HeightField::load(corrupt.path);
                else venus::SurfaceMap::load(corrupt.path);
            } catch(const std::exception& e) {
                failed=true;
                check(std::string(e.what()).find(corrupt.path.string())!=std::string::npos,"Asset error omitted the failing path");
            }
            check(failed,why);
        };
        rejects("VENUS_RGB 1 1440 720 0000000000000000\n",false,"Truncated surface map accepted");
        rejects("VENUS_DEM 1 4096 2048 0000000000000000\n",true,"Truncated terrain accepted");
        rejects("VENUS_DEM 1 2048 4096 0000000000000000\n",true,"Wrong terrain dimensions accepted");
        Scratch absent;
        rejected=false;
        try { venus::SurfaceMap::load(absent.path); } catch(const std::exception& e) {
            rejected=std::string(e.what()).find(absent.path.string())!=std::string::npos;
        }
        check(rejected,"Missing asset error omitted its path");
        // Ishtar Terra's Maxwell Montes must be high, with the correct map orientation.
        check(field.sample(65.2,3.3)>8000,"Maxwell Montes orientation/datum");
        check(std::abs(field.sample(20,30)-field.sample(20,390))<.01,"Longitude seam wrap");
        auto mesh=venus::makeFacetedGlobe(field,64);
        bool missing=false;
        for(auto& v:mesh) {
            check(std::isfinite(v.meters),"Nonfinite terrain geometry");
            check(v.meters>-4000&&v.meters<12000,"Missing sample displaced geometry");
            missing=missing||v.faceMeters==-32768;
        }
        check(missing,"Lost no-data mask");
        // Missing data with zero interpolation weight must not hide a measured
        // sample or make an exact pole depend on the arbitrary longitude.
        venus::HeightField sparse;
        sparse.meters.assign(venus::HeightField::width*venus::HeightField::height,-32768);
        sparse.north=120; sparse.south=-80;
        constexpr int row=512,column=2048;
        sparse.meters[row*venus::HeightField::width+column]=640;
        double lat=90-(row+.5)*180/venus::HeightField::height;
        double lon=(column+.5)*360/venus::HeightField::width-180;
        check(sparse.sample(lat,lon)==640,"Valid cell centre lost to zero-weight no-data neighbour");
        check(std::isnan(sparse.sample(lat,lon+.01)),"Interpolated missing data was filled");
        check(std::isnan(sparse.sample(lat-.01,lon)),"Missing row was filled");
        for(double longitude:{0.,90.,180.,359.}) {
            check(sparse.sample(90,longitude)==120,"North pole depends on missing longitude");
            check(sparse.sample(-90,longitude)==-80,"South pole depends on missing longitude");
        }
        check(std::isnan(sparse.sample(89.99,0)),"Missing polar neighbour was filled");
        check(std::isnan(sparse.sample(std::numeric_limits<double>::quiet_NaN(),0)),"Invalid coordinate accepted");
        venus::View view;
        view.select(2); view.camera=view.target;
        auto before=view.camera; view.clouds.advance(17);
        view.key(SDLK_T); check(view.topography,"Topography toggle");
        view.key(SDLK_C); check(!view.topography,"Cloud toggle alias");
        check(view.camera.lat==before.lat&&view.camera.lon==before.lon&&view.selected==2,"Map switch moved camera");
        check(view.clouds.seconds==17,"Map switch reset animation");
        view.key(SDLK_SPACE); view.clouds.advance(2);
        check(view.clouds.seconds==17,"Paused clouds advanced");
        view.key(SDLK_SPACE); view.clouds.advance(43);
        check(view.clouds.seconds==0,"Cloud loop did not wrap");
        auto phase=view.clouds.phase();
        check(phase.x==1&&phase.y==0,"Cloud loop did not close");
        view.clouds.advance(9); auto revision=view.clouds.revision;
        view.key(SDLK_R);
        check(view.clouds.seconds==0&&view.clouds.revision==revision+1,"Cloud restart/retry");
        mars::ViewLayout layout; layout.resize(1000,650,1); layout.marsView=false; layout.venusView=true; layout.venusControls=true;
        auto r=layout.venusMap(); check(layout.overHud(r.x+2,r.y+2),"Map click not captured by HUD");
        view.click(layout,float(r.x+2),float(r.y+2)); check(view.topography,"Map button");
        auto click=[&](mars::Rect button) { view.click(layout,float(button.x+2),float(button.y+2)); };
        view.clouds.paused=true;
        click(layout.venusPause()); check(view.clouds.paused,"Disabled cloud button changed the pause state in surface mode");
        view.key(SDLK_SPACE); check(view.clouds.paused,"Disabled cloud key changed the pause state in surface mode");
        view.key(SDLK_E); check(view.exaggeration==1,"Surface relief keyboard control");
        click(layout.venusRelief()); check(view.exaggeration==10,"Surface relief button");
        click(layout.venusMap());
        view.key(SDLK_E); check(view.exaggeration==10,"Disabled relief key changed the hidden surface setting");
        click(layout.venusRelief()); check(view.exaggeration==10,"Disabled relief button changed the hidden surface setting");
        click(layout.venusPause()); check(!view.clouds.paused,"Cloud pause button did not resume");
        click(layout.venusMap());
        layout.venusView=false;
        click(layout.venusRelief()); check(view.exaggeration==10,"System relief changed the detailed globe setting");
        view.clouds.advance(5); revision=view.clouds.revision;
        click(layout.venusReset()); check(view.clouds.seconds==0&&view.clouds.revision==revision+1,"System restart/retry button");
        solar::Navigation nav; nav.start({solar::ViewKind::Globe,solar::BodyId::Venus});
        check(nav.venus()&&nav.detailed(),"Venus detailed routing");
        nav.up(); for(int i=0;i<60;++i) nav.update(.01);
        check(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Venus},"Venus parent routing");
        std::cout<<"Venus data, geometry, controls and navigation passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
