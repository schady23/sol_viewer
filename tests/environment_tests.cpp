#include "environment.hpp"
#include "ui.hpp"
#include <iostream>
#include <stdexcept>

void require(bool ok,const char* what) { if(!ok) throw std::runtime_error(what); }
int main() {
    try {
        mars::Environment a,b;
        a.update(239);
        require(!a.stormActive(),"Storm should not begin before the four-minute interval");
        a.update(181);
        require(a.stormActive()||a.stormStrength()==0,"Storm strength must be zero when inactive");
        // Large time steps and 60 Hz updates must consume the same seeded events.
        a.update(1800-420);
        for(int i=0;i<1800*60;++i) b.update(1.0/60);
        require(a.stormActive()==b.stormActive(),"Frame-rate-dependent event schedule");
        require(std::abs(a.stormStrength()-b.stormStrength())<0.0001f,"Frame-rate-dependent storm envelope");
        require(mars::dot(a.stormCentre(),b.stormCentre())>0.99999f,"Frame-rate-dependent storm drift");

        mars::Environment preview;
        preview.previewStorm({15,359.9});
        require(preview.showDust()&&preview.stormStrength()>0.9f,"Preview must show an established storm");
        float strength=preview.stormStrength(); auto centre=preview.stormCentre();
        preview.paused=true; preview.update(100);
        require(preview.time()==0&&preview.stormStrength()==strength,"Pause must freeze all animation");
        require(mars::dot(centre,preview.stormCentre())>0.99999f,"Pause must freeze storm location");
        preview.topography=true;
        require(!preview.showIce()&&!preview.showClouds()&&!preview.showDust(),"Clean topography must suppress all overlays");
        preview.topography=false;
        require(preview.showIce()&&preview.showClouds()&&preview.showDust(),"Layer selections must survive clean mode");
        preview.paused=false; preview.update(121);
        require(!preview.stormActive()&&preview.stormStrength()==0,"Storm must dissipate");
        mars::Environment lifecycle;
        bool started=false,finished=false;
        for(int i=0;i<540;++i) {
            bool before=lifecycle.stormActive(); lifecycle.update(1);
            if(lifecycle.stormActive()) started=true;
            if(before&&!lifecycle.stormActive()) finished=true;
            require(lifecycle.stormStrength()>=0&&lifecycle.stormStrength()<=1,"Invalid opacity envelope");
        }
        require(started&&finished,"Automatic storm must start and end within the documented ranges");

        for(auto size:{mars::Rect{0,0,800,500},{0,0,1242,740},{0,0,1242,1508},{0,0,2560,1600},{0,0,3840,2160}}) {
            for(float requested:{0.75f,1.0f,1.25f,1.5f,1.75f,2.0f}) {
                mars::ViewLayout layout; layout.environmentOpen=true;
                layout.resize(size.w,size.h,requested);
                require(layout.width()>=760&&layout.height()>=480,"Scaled UI must fit");
                require(layout.scale>=1,"Glyphs must retain at least one native pixel per stroke");
                require(layout.readout().x+layout.readout().w<layout.legend().x,"Bottom panels overlap");
                require(layout.environmentPanel().y+layout.environmentPanel().h<layout.legend().y,"Environment overlaps legend");
                for(auto r:{layout.scaleDown(),layout.scaleUp(),layout.landmarkButton(3),layout.pauseButton(),layout.stormButton(),layout.layerButton(3)}) {
                    require(layout.overHud(r.x+r.w/2.0f,r.y+r.h/2.0f),"Control must intercept globe input");
                    float nativeX=(r.x+r.w/2.0f)*layout.scale,nativeY=(r.y+r.h/2.0f)*layout.scale;
                    require(r.contains(nativeX/layout.scale,nativeY/layout.scale),"Scaled button coordinates must round trip");
                }
                layout.environmentOpen=false;
                require(!layout.overHud(float(layout.stormButton().x),float(layout.stormButton().y)),"Collapsed controls must not intercept globe input");
            }
        }
        std::cout<<"Environment timing, pause, clean mode and scaled layout checks passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
