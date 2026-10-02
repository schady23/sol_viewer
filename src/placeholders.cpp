#include "placeholders.hpp"
#include "ice_giants.hpp"
#include "belts.hpp"
#include <limits>
namespace solar {
namespace {
bool visibleBody(const SceneFrame& scene,const DisplayBody& target) {
    // Test the ray through the exact body centre, not a rounded screen pixel.
    // At solar scale a terrestrial planet can occupy far less than one pixel.
    auto eye=scene.camera.position();
    double x=double(target.centre.x)-eye.x,y=double(target.centre.y)-eye.y,z=double(target.centre.z)-eye.z;
    double distance=std::sqrt(x*x+y*y+z*z);
    if(distance<=target.radius) return true;
    x/=distance; y/=distance; z/=distance;
    for(const auto& b:scene.bodies) if(b.id!=target.id) {
        double bx=double(b.centre.x)-eye.x,by=double(b.centre.y)-eye.y,bz=double(b.centre.z)-eye.z;
        double along=bx*x+by*y+bz*z;
        if(along<=0||along>=distance) continue;
        bx-=x*along; by-=y*along; bz-=z*along;
        if(bx*bx+by*by+bz*bz<double(b.radius)*b.radius) return false;
    }
    return true;
}
BodyId rayPick(const SceneFrame& scene,const mars::ViewLayout& layout,float x,float y,bool generous) {
    auto& c=scene.camera;
    float nx=2*x/layout.width()-1,ny=1-2*y/layout.height();
    auto ray=mars::normalized(c.right()*(nx*c.lens*float(layout.pixelWidth)/layout.pixelHeight)+c.up()*(ny*c.lens)-c.backward());
    float closest=std::numeric_limits<float>::max(); BodyId id=BodyId::None;
    for(const auto& b:scene.bodies) {
        auto centre=b.centre-c.position(); float along=mars::dot(centre,ray);
        float radius=b.radius;
        if(generous) radius=std::max(radius,18*along*c.lens/layout.height());
        // Closest-approach form avoids catastrophic cancellation for the small
        // correctly scaled Galilean moons viewed from far across the system.
        auto perpendicular=centre-ray*along;
        float disc=radius*radius-mars::dot(perpendicular,perpendicular);
        if(along<=0||disc<0) continue;
        float near=along-std::sqrt(disc);
        if(near>0&&near<closest) { closest=near; id=b.id; }
    }
    return id;
}
}
PlaceholderGeometry makePlaceholderGeometry() {
    PlaceholderGeometry g;
    for(int j=0;j<16;++j) for(int i=0;i<32;++i) {
        auto a=mars::direction(-90+j*180.0/16,i*360.0/32);
        auto b=mars::direction(-90+j*180.0/16,(i+1)*360.0/32);
        auto c=mars::direction(-90+(j+1)*180.0/16,i*360.0/32);
        auto d=mars::direction(-90+(j+1)*180.0/16,(i+1)*360.0/32);
        if(j>0) g.sphere.insert(g.sphere.end(),{a,b,c});
        if(j<15) g.sphere.insert(g.sphere.end(),{b,d,c});
    }
    auto ringPoint=[](double angle,float radius) {
        auto p=mars::direction(0,angle)*radius;
        return mars::Vec3{p.x,p.z*.40f,p.z*.9165f};
    };
    for(int i=0;i<96;++i) {
        double a=i*360.0/96,b=(i+1)*360.0/96;
        auto p=ringPoint(a,1.35f),q=ringPoint(a,1.9f),r=ringPoint(b,1.35f),s=ringPoint(b,1.9f);
        g.ring.insert(g.ring.end(),{p,q,r,q,s,r});
    }
    for(int i=0;i<512;++i)
        g.circle.insert(g.circle.end(),{mars::direction(0,i*360.0/512),mars::direction(0,(i+1)*360.0/512)});
    return g;
}
std::vector<mars::Rect> crumbRects(const Navigation& nav,const mars::ViewLayout& layout) {
    bool detailed=layout.marsView||layout.mercuryView||layout.moonView||layout.jupiterView||layout.saturnView||layout.earthView||layout.venusView||layout.ioView||layout.callistoView||layout.satelliteView;
    int x=detailed?32:20,y=detailed?54:72;
    std::vector<mars::Rect> result;
    for(const auto& c:nav.breadcrumbs()) {
        int w=int(c.label.size())*6+12; result.push_back({x,y,w,20}); x+=w+6;
    }
    return result;
}
SceneFrame makeScene(const Navigation& nav,const mars::ViewLayout& layout) {
    SceneFrame f;
    if((nav.view().kind==ViewKind::Solar||nav.belt())&&nav.state().approached!=BodyId::None)
        f.renderOrigin=mapPosition(nav.view().focus,nav.state().approached);
    f.orbitCentre=f.renderOrigin*-1.f;
    f.camera=nav.camera(f.renderOrigin);
    // Portrait windows retain the same horizontal fit without extending the depth range.
    f.camera.lens*=std::max(1.0f,float(layout.pixelHeight)/layout.pixelWidth);
    auto view=nav.view(); f.surfacePreviews=view.kind!=ViewKind::Solar||nav.state().approached!=BodyId::None;
    f.trueSolarScale=view.kind==ViewKind::Solar;
    f.trueEarthScale=view==ViewKey{ViewKind::System,BodyId::Earth};
    f.trueMarsScale=view==ViewKey{ViewKind::System,BodyId::Mars};
    f.trueSaturnScale=view==ViewKey{ViewKind::System,BodyId::Saturn};
    f.trueOuterScale=view.kind==ViewKind::System&&ice::planet(view.focus);
    if(view.kind==ViewKind::Globe) f.bodies.push_back({view.focus,{0,0,0},1});
    else if(nav.belt()) {
        f.belt=view.focus;
        f.bodies.push_back({BodyId::Sun,{},systemBodyRadius(view.focus,BodyId::Sun)});
        for(auto id:children(view.focus)) f.bodies.push_back({id,mapPosition(view.focus,id),systemBodyRadius(view.focus,id)});
    } else {
        f.bodies.push_back({view.focus,{0,0,0},f.trueSolarScale?solarBodyRadius(view.focus):systemBodyRadius(view.focus,view.focus)});
        auto ids=children(view.focus);
        // Authored display coordinates are shared with transition framing.
        for(std::size_t i=0;i<ids.size();++i) {
            f.lanes.push_back(laneRadius(view.focus,i));
            f.laneTilts.push_back(f.trueOuterScale?float(ice::tilt(view.focus)+outerMoons(view.focus)[i].inclination):0.f);
            f.bodies.push_back({ids[i],mapPosition(view.focus,ids[i]),f.trueSolarScale?solarBodyRadius(ids[i]):systemBodyRadius(view.focus,ids[i])});
        }
    }
    for(auto& b:f.bodies) b.centre=b.centre-f.renderOrigin;
    float aspect=float(layout.pixelWidth)/layout.pixelHeight;
    for(const auto& b:f.bodies) {
        auto relative=b.centre-f.camera.position();
        float z=-mars::dot(relative,f.camera.backward());
        if(z<=f.camera.clipNear) continue;
        float x=(1+mars::dot(relative,f.camera.right())/(z*f.camera.lens*aspect))*layout.width()*.5f;
        float y=(1-mars::dot(relative,f.camera.up())/(z*f.camera.lens))*layout.height()*.5f;
        float r=b.radius/(z*f.camera.lens)*layout.height()*.5f;
        BodyLabel label{b.id,x,y,z,r,{0,0,int(std::string(body(b.id).name).size()*6+12),20},false};
        label.visible=visibleBody(f,b)&&x>=0&&x<layout.width()&&y>=0&&y<layout.height();
        f.labels.push_back(label);
    }
    // Place labels after projecting every body so a label cannot hide another
    // planet's locator in the tightly packed inner system.
    for(auto& label:f.labels) {
        if(label.visible&&view.kind!=ViewKind::Globe) {
            for(int attempt=0;attempt<((f.trueSolarScale||f.trueMarsScale||f.trueSaturnScale||f.trueOuterScale||belts::region(f.belt))?18:6)&&!label.shown;++attempt) {
                int side=attempt%2?-1:1,step=attempt/2;
                int offset=(f.trueSolarScale||f.trueMarsScale||f.trueSaturnScale||f.trueOuterScale||belts::region(f.belt))?(step%2?(step+1)/2:-step/2):step;
                label.rect.x=side>0?int(label.x+label.radius+9):int(label.x-label.radius-9)-label.rect.w;
                label.rect.y=int(label.y-10)+offset*24;
                auto a=label.rect;
                bool clear=a.x>=8&&a.y>=100&&a.x+a.w<layout.width()-8&&a.y+a.h<layout.height()-120;
                for(float px:{float(a.x),float(a.x+a.w)}) for(float py:{float(a.y),float(a.y+a.h)})
                    if(layout.overHud(px,py)) clear=false;
                for(const auto& previous:f.labels) if(previous.shown) {
                    auto bRect=previous.rect;
                    if(a.x<bRect.x+bRect.w+4&&a.x+a.w+4>bRect.x&&a.y<bRect.y+bRect.h+4&&a.y+a.h+4>bRect.y) clear=false;
                }
                if(f.trueSolarScale||f.trueMarsScale||f.trueSaturnScale||f.trueOuterScale||belts::region(f.belt)) for(const auto& other:f.labels) if(other.visible&&other.id!=label.id)
                    if(other.x>=a.x-5&&other.x<=a.x+a.w+5&&other.y>=a.y-5&&other.y<=a.y+a.h+5) clear=false;
                label.shown=clear;
            }
        }
    }
    return f;
}
BodyId pickBody(const SceneFrame& f,const mars::ViewLayout& layout,float x,float y,bool labels) {
    if(labels&&layout.overHud(x,y)) return BodyId::None;
    if(labels) for(const auto& l:f.labels) if(l.shown&&l.rect.contains(x,y)) return l.id;
    if(f.trueSolarScale||f.trueMarsScale||f.trueSaturnScale||f.trueOuterScale||belts::region(f.belt)) {
        auto exact=rayPick(f,layout,x,y,false);
        if(exact!=BodyId::None) return exact;
        // Choose the nearest locator on screen, rather than the nearest enlarged
        // sphere along the ray. Planet geometry always retains its physical size.
        float closest=9*9; BodyId id=BodyId::None;
        for(const auto& l:f.labels) if(l.visible) {
            float dx=x-l.x,dy=y-l.y,d=dx*dx+dy*dy;
            if(d<closest) { closest=d; id=l.id; }
        }
        return id;
    }
    return rayPick(f,layout,x,y,true);
}
}
