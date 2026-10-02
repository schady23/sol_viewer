#include "earth.hpp"
#include "terrain.hpp"
#include "navigation.hpp"
#include "ui.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
bool close(mars::Vec3 a,mars::Vec3 b,float tolerance=1e-5f) {
    auto d=a-b; return mars::dot(d,d)<tolerance*tolerance;
}
void finish(solar::Navigation& nav) { for(int i=0;i<60;++i) nav.update(.01); }
int main(int argc,char** argv) {
    try {
        earth::HeightField flat; flat.meters.assign(2400*1200,100); flat.north=flat.south=100;
        auto mesh=earth::makeGlobe(flat,32);
        check(mesh.vertices.size()==514&&mesh.indices.size()==32*32*3,"Indexed Earth mesh dimensions");
        int northFaces=0,southFaces=0,seamFaces=0;
        for(std::size_t i=0;i<mesh.indices.size();i+=3) {
            const auto& a=mesh.vertices.at(mesh.indices[i]);
            const auto& b=mesh.vertices.at(mesh.indices[i+1]);
            const auto& c=mesh.vertices.at(mesh.indices[i+2]);
            auto normal=mars::cross(b.direction-a.direction,c.direction-a.direction);
            check(mars::dot(normal,a.direction)>0,"Earth triangle degenerates or faces inward");
            check(a.meters==100&&b.meters==100&&c.meters==100,"Mesh modifies source elevations");
            bool positive=false,negative=false;
            for(const auto& v:{a,b,c}) {
                if(v.direction.y==1) ++northFaces;
                if(v.direction.y==-1) ++southFaces;
                auto lon=mars::location(v.direction).lon;
                positive|=lon>350; negative|=lon<10;
            }
            seamFaces+=positive&&negative;
        }
        check(northFaces==32&&southFaces==32&&seamFaces>0,"Indexed Earth seam/pole coverage missing");
        bool rejectedMesh=false;
        try { earth::makeGlobe({},32); } catch(...) { rejectedMesh=true; }
        check(rejectedMesh,"Earth mesh accepted missing elevation data");
        // Regression: a coarse 32-column shell intersects Earth at true system scale.
        // The distance to every triangle plane bounds all points on that face.
        auto shell=mars::makeSampledGlobe(earth::cloudColumns,[](double,double) { return 0.f; });
        for(float relief:{1.f,10.f}) for(int layer:{0,1}) {
            float radius=earth::cloudShellRadius(relief,layer);
            for(std::size_t i=0;i<shell.indices.size();i+=3) {
                auto a=shell.vertices[shell.indices[i]].direction*radius;
                auto b=shell.vertices[shell.indices[i+1]].direction*radius;
                auto c=shell.vertices[shell.indices[i+2]].direction*radius;
                auto normal=mars::normalized(mars::cross(b-a,c-a));
                check(mars::dot(normal,a)>1.f+9000.f*relief/earth::radiusMeters,
                      "Cloud shell intersects maximum terrain at system/globe scale");
            }
        }
        earth::Clouds c;
        auto start=c.phase(); c.advance(30); check(!close(start,c.phase()),"Clouds do not move");
        c.advance(30); check(c.seconds==0&&close(start,c.phase()),"60-second loop closure");
        c.advance(3600*24+7.5); check(std::abs(c.seconds-7.5)<1e-9,"Long frame loses elapsed time");
        c.paused=true; auto held=c.phase(); c.advance(123); check(close(held,c.phase()),"Pause lost");
        c.paused=false; c.visible=false; c.advance(2); check(c.seconds==9.5,"Visibility must not reset or pause phase");
        for(double invalid:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) c.advance(invalid);
        check(c.seconds==9.5,"Invalid elapsed time accepted");
        c.reset(); for(int i=0;i<3600;++i) c.advance(1./60);
        check(close(c.phase(),start),"60 seconds at 60 FPS does not close");
        c.seconds=59.99; auto before=c.phase(); c.advance(.02); auto after=c.phase();
        auto left=(start-before)*100,right=(after-start)*100;
        check(close(left,right,.002f),"Velocity discontinuity at loop boundary");
        c.reset(); check(c.visible==false&&!c.paused&&c.seconds==0,"Reset changes cloud preferences");
        solar::Navigation nav; nav.start({}); nav.select(solar::BodyId::Earth); nav.open(); finish(nav);
        check(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Earth},"Earth system route");
        check(nav.earthControls(),"Selected Earth must show its controls");
        nav.drag(36,12); finish(nav); auto saved=nav.state().camera;
        nav.open(); finish(nav); check(nav.earth()&&nav.detailed(),"Earth detailed globe route");
        nav.up(); finish(nav);
        check(nav.state().selected==solar::BodyId::Earth&&std::abs(nav.state().camera.lon-saved.lon)<.01,"Earth return loses context");
        check(nav.earthControls(),"Returning from Earth lost its controls");
        nav.select(solar::BodyId::Moon);
        check(!nav.earthControls(),"Earth controls remain active when Moon is selected");
        mars::ViewLayout moonSystem; moonSystem.marsView=false; moonSystem.systemView=true;
        moonSystem.bodyRows=2; moonSystem.earthControls=nav.earthControls(); moonSystem.resize(1600,1000,1);
        for(auto r:{moonSystem.earthToggle(),moonSystem.earthMap(),moonSystem.earthPause(),moonSystem.earthReset(),moonSystem.earthRelief()})
            check(!moonSystem.overHud(r.x+1,r.y+1),"Hidden Earth controls still capture Moon-system input");
        for(auto r:{moonSystem.earthWhole(),moonSystem.earthApproach()})
            check(moonSystem.overHud(r.x+1,r.y+1),"Moon selection lost system framing controls");
        nav.open(); finish(nav); check(nav.moon(),"Moon route regressed");
        check(!nav.earthControls(),"Earth controls leaked into Moon globe");
        nav.up(); finish(nav); check(nav.state().selected==solar::BodyId::Moon,"Moon return selection lost");
        check(!nav.earthControls(),"Returning from Moon reopened Earth controls");
        nav.select(solar::BodyId::Earth); check(nav.earthControls(),"Reselecting Earth did not restore controls");
        nav.open(); finish(nav); check(nav.earthControls(),"Earth globe controls missing");
        for(float scale:{.75f,1.f,2.f}) for(auto size:{std::pair<int,int>{800,500},{1600,1000},{1000,1500}}) {
            mars::ViewLayout l; l.marsView=false; l.earthView=l.earthControls=true; l.resize(size.first,size.second,scale);
            for(auto r:{l.earthToggle(),l.earthPause(),l.earthReset(),l.earthMap(),l.earthRelief(),l.landmarkButton(3)}) {
                check(l.overHud(r.x+1,r.y+1),"Earth control leaks input to globe");
                check(r.x>=0&&r.x+r.w<=l.width()&&r.y+r.h<l.readout().y,"Earth UI clipped/overlaps readout");
            }
            check(l.navigationUp().x==210,"Earth uses wrong breadcrumbs");
            l.earthView=false; l.bodyRows=2;
            for(auto r:{l.earthWhole(),l.earthApproach()}) {
                check(l.overHud(r.x+1,r.y+1),"Earth system framing leaks input");
                check(r.y>l.openBody().y+l.openBody().h&&r.y+r.h<l.readout().y,"Earth system framing overlaps UI");
            }
        }
        if(argc>1) {
            auto image=earth::Image::load(argv[1]);
            check(image.width==4096&&image.rgb.size()==4096*2048*3,"Prepared Earth map");
            auto bad=std::filesystem::temp_directory_path()/"solar-earth-invalid.rgb";
            { std::ofstream out(bad,std::ios::binary); out<<"EARTH_RGB 1 99999999 1024 0000000000000000\n"; }
            bool rejected=false; try { earth::Image::load(bad); } catch(...) { rejected=true; }
            std::filesystem::remove(bad); check(rejected,"Unbounded malformed asset dimensions accepted");
        }
        if(argc>2) {
            auto field=earth::HeightField::load(argv[2]);
            check(field.meters.size()==2400*1200,"DEM dimensions");
            check(field.sample(28,87)>3500,"Himalayas missing / grid orientation wrong");
            check(field.sample(0,-140)<-3000,"Pacific bathymetry missing");
            check(field.sample(-85,0)>1000,"Antarctic ice surface missing");
            check(field.sample(23,180)==field.sample(23,-180),"Elevation longitude seam");
            check(field.sample(90,0)==field.sample(90,160)&&field.sample(-90,0)==field.sample(-90,160),"Pole is longitude dependent");
            check(std::abs(field.sample(0,-180.000001)-field.sample(0,179.999999))<.01,"Bilinear seam continuity");
            check(std::isnan(field.sample(std::numeric_limits<double>::quiet_NaN(),0)),"Invalid probe accepted");
            for(auto index:{0,740032,1452457,2879999}) {
                int y=index/2400,x=index%2400;
                check(std::abs(field.sample(90-(y+.5)*.15,-180+(x+.5)*.15)-field.meters[index])<.01,"Pixel-centre sample shifted");
            }
        }
        if(argc>3) {
            auto cloud=earth::Image::loadClouds(argv[3]);
            check(cloud.width==2048&&cloud.rgb.size()==2048*1024,"Cloud reference integrity");
        }
        std::cout<<"PASS: Earth loop duration/velocity, pause, toggle, map/DEM integrity and coordinates, scaled controls and Earth/Moon navigation\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
