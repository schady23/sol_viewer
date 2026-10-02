#pragma once
#include <algorithm>
#include <cmath>
#include <string>

namespace mars {
struct Rect {
    int x,y,w,h;
    bool contains(float px,float py) const { return px>=x && py>=y && px<x+w && py<y+h; }
};
struct NumberEdit { int field=-1; std::string value,error; bool replace=true; };
// Logical instrument coordinates. The globe always uses the full native drawable.
struct ViewLayout {
    int pixelWidth=1,pixelHeight=1,bodyRows=9;
    float scale=1,requestedScale=1;
    bool environmentOpen=false,satellite=false,marsView=true,mercuryView=false,guideAvailable=false,guideOpen=false;
    bool locationAvailable=false,locationOpen=false;
    bool jupiterView=false,jupiterControls=false,saturnView=false,saturnControls=false;
    bool ioView=false,callistoView=false,satelliteView=false;
    bool venusView=false,venusControls=false;
    bool earthView=false,earthControls=false;
    bool sunView=false,systemView=false,solarView=false,moonView=false,marsSystem=false,saturnSystem=false,titanView=false,iceView=false,outerSystem=false;
    void resize(int w,int h,float requested) {
        pixelWidth=w; pixelHeight=h;
        requestedScale=std::clamp(requested,0.75f,2.0f);
        float fit=std::min(w/760.0f,h/(solarView?528.0f:480.0f));
        scale=std::max(std::min(1.0f,fit),std::min(baseScale()*requestedScale,fit));
    }
    float baseScale() const { return float(std::max(1,std::min(pixelWidth/1000,pixelHeight/650))); }
    int width() const { return int(pixelWidth/scale); }
    int height() const { return int(pixelHeight/scale); }
    Rect title() const { return {20,20,226,58}; }
    int sidebarTop() const { return guideAvailable?132:94; }
    Rect destinations() const { return {20,sidebarTop(),322,moonView?136:108}; }
    Rect landmarkButton(int i) const { return {20,sidebarTop()+20+i*22,322,22}; }
    Rect locationPrev() const { return {20,sidebarTop()+110,32,24}; }
    Rect locationNext() const { return {310,sidebarTop()+110,32,24}; }
    Rect orbitButton() const {
        if(satellite) return {guideAvailable?274:20,94,246,26};
        int y=locationAvailable?locationToggle().y+24+(locationOpen?locationPanel().h:0)+8:destinations().y+destinations().h+12;
        return {20,y,246,26};
    }
    Rect orbitPanel() const { return {20,126,246,230}; }
    Rect orbitValue(int i) const { return {28,146+i*46,166,24}; }
    Rect orbitMinus(int i) const { return {200,146+i*46,28,24}; }
    Rect orbitPlus(int i) const { return {230,146+i*46,28,24}; }
    Rect orbitPause() const { return {28,270,110,24}; }
    Rect orbitSpeed() const { return {142,270,116,24}; }
    Rect orbitView() const { return {28,300,230,24}; }
    Rect fullscreenButton() const { return {width()-226,20,206,26}; }
    Rect reliefButton() const { return {width()-226,50,100,26}; }
    Rect homeButton() const { return {width()-122,50,102,26}; }
    Rect scalePanel() const { return {width()-226,80,206,30}; }
    Rect scaleDown() const { return {width()-78,80,28,30}; }
    Rect scaleUp() const { return {width()-48,80,28,30}; }
    Rect environmentButton() const { return {width()-226,120,206,28}; }
    Rect environmentPanel() const { return {width()-226,148,206,178}; }
    Rect layerButton(int i) const { return {width()-220,166+i*25,194,24}; }
    Rect pauseButton() const { return {width()-220,273,94,24}; }
    Rect stormButton() const { return {width()-122,273,96,24}; }
    Rect readout() const { return {20,height()-112,322,82}; }
    Rect legend() const { return {width()-226,height()-112,206,82}; }
    Rect navigationUp() const { return (marsView||mercuryView||moonView||jupiterView||saturnView||earthView||venusView||ioView||callistoView||satelliteView)?Rect{210,24,32,24}:reliefButton(); }
    Rect callistoLighting() const { return ioLighting(); }
    Rect callistoRetry() const { return ioRetry(); }
    Rect callistoPanel() const { return ioPanel(); }
    Rect ioLighting() const { return {width()-226,120,206,24}; }
    Rect icePause() const { return {width()-226,150,206,24}; }
    Rect iceSpeed() const { return {width()-226,180,206,24}; }
    Rect iceReset() const { return {width()-226,210,206,24}; }
    Rect ioRetry() const { return {width()-226,titanView?240:210,206,24}; }
    Rect ioPanel() const { return {width()-226,120,206,iceView?144:titanView?144:114}; }
    Rect venusMap() const { return {width()-226,120,206,24}; }
    Rect venusRelief() const { return {width()-226,150,206,24}; }
    Rect venusPause() const { return {width()-226,180,206,24}; }
    Rect venusReset() const { return {width()-226,titanView?240:210,206,24}; }
    Rect venusPanel() const { return {width()-226,120,206,208}; }
    Rect sunPause() const { return {width()-226,120,206,24}; }
    Rect sunSpeed() const { return {width()-226,150,206,24}; }
    Rect sunReset() const { return {width()-226,180,206,24}; }
    Rect sunClose() const { return {20,132,206,24}; }
    Rect sunCrown() const { return {20,162,206,24}; }
    Rect sunScale() const { return {20,192,206,24}; }
    Rect sunScalePanel() const { return {20,228,206,96}; }
    Rect earthToggle() const { return {width()-226,120,206,24}; }
    Rect earthPause() const { return {width()-226,150,206,24}; }
    Rect earthReset() const { return {width()-226,180,206,24}; }
    Rect earthPanel() const { return {width()-226,120,206,208}; }
    Rect earthMap() const { return {width()-226,titanView?240:210,206,24}; }
    Rect earthWhole() const { return {20,openBody().y+34,182,24}; }
    Rect earthApproach() const { return {20,openBody().y+64,182,24}; }
    Rect earthRelief() const { return {width()-226,240,206,24}; }
    Rect saturnWhole() const { return {20,openBody().y+34,182,24}; }
    Rect saturnInner() const { return {20,openBody().y+64,182,24}; }
    Rect saturnApproach() const { return {20,openBody().y+94,182,24}; }
    Rect titanHaze() const { return {width()-226,150,206,24}; }
    Rect saturnPause() const { return jupiterPause(); }
    Rect saturnSlower() const { return jupiterSlower(); }
    Rect saturnFaster() const { return jupiterFaster(); }
    Rect saturnFrame() const { return jupiterFrame(); }
    Rect saturnReset() const { return {width()-226,saturnView?234:204,206,24}; }
    Rect saturnPanel() const { return {width()-226,120,206,saturnView?176:146}; }
    Rect jupiterPause() const { return {width()-226,120,206,24}; }
    Rect marsWhole() const { return {width()-226,120,206,24}; }
    Rect marsApproach() const { return {width()-226,150,206,24}; }
    Rect marsSystemPanel() const { return {width()-226,120,206,100}; }
    Rect solarWhole() const { return {width()-226,120,206,24}; }
    Rect solarInner() const { return {width()-226,150,206,24}; }
    Rect solarPanel() const { return {width()-226,120,206,100}; }
    Rect asteroidBelt() const { return bodyButton(bodyRows); }
    Rect kuiperBelt() const { return bodyButton(bodyRows+1); }
    Rect jupiterSlower() const { return {width()-226,150,26,24}; }
    Rect jupiterFaster() const { return {width()-46,150,26,24}; }
    Rect jupiterFrame() const { return {width()-226,204,206,24}; }
    Rect jupiterApproach() const { return {width()-226,234,206,24}; }
    Rect jupiterReset() const { return {width()-226,jupiterView?234:264,206,24}; }
    Rect jupiterPanel() const { return {width()-226,120,206,jupiterView?176:206}; }
    // Reserve the guide row even for unsupported planets so selecting a body
    // cannot move the list between the two clicks of a double-click.
    Rect bodyList() const { return {20,132,182,28+18*(bodyRows+(solarView?2:0))}; }
    Rect bodyButton(int i) const { return {24,bodyList().y+22+i*18,174,17}; }
    Rect openBody() const { return {20,bodyList().y+bodyList().h+8,182,26}; }
    Rect guideToggle() const { return {20,94,246,26}; }
    Rect scanButton() const { return {(width()-96)/2,20,96,26}; }
    Rect guidePanel() const { return {20,126,440,height()-158}; }
    Rect guideClose() const { auto r=guidePanel(); return {r.x+r.w-36,r.y+8,24,24}; }
    Rect guideSource() const { auto r=guidePanel(); return {r.x+12,r.y+r.h-62,174,24}; }
    Rect guideUp() const { auto r=guidePanel(); return {r.x+r.w-70,r.y+r.h-62,26,24}; }
    Rect guideDown() const { auto r=guidePanel(); return {r.x+r.w-38,r.y+r.h-62,26,24}; }
    int guideColumns() const { return (guidePanel().w-36)/9; }
    int guideRows() const { return std::max(1,(guidePanel().h-132)/18); }
    Rect locationToggle() const { return {20,destinations().y+destinations().h+8,322,24}; }
    Rect locationPanel() const {
        int y=locationToggle().y+24;
        return {20,y,322,std::clamp(readout().y-8-(marsView?34:0)-y,50,218)};
    }
    Rect locationSource() const { auto r=locationPanel(); return {r.x+8,r.y+r.h-22,144,18}; }
    Rect locationUp() const { auto r=locationPanel(); return {r.x+r.w-66,r.y+r.h-22,26,18}; }
    Rect locationDown() const { auto r=locationPanel(); return {r.x+r.w-34,r.y+r.h-22,26,18}; }
    int locationColumns() const { return int((locationPanel().w-28)/7.5f); }
    int locationRows() const { return std::max(1,(locationPanel().h-28)/13); }
    bool beltView=false;
    bool overHud(float x,float y) const {
        if(beltView&&Rect{width()-226,120,206,194}.contains(x,y)) return true;
        if(sunView) for(auto r:{sunPause(),sunSpeed(),sunReset(),sunClose(),sunCrown(),sunScale(),sunScalePanel()}) if(r.contains(x,y)) return true;
        if(systemView&&(earthWhole().contains(x,y)||earthApproach().contains(x,y))) return true;
        if((saturnSystem||outerSystem)&&(saturnWhole().contains(x,y)||saturnInner().contains(x,y)||saturnApproach().contains(x,y))) return true;
        if(outerSystem&&ioPanel().contains(x,y)) return true;
        if(marsSystem&&marsSystemPanel().contains(x,y)) return true;
        if(solarView&&solarPanel().contains(x,y)) return true;
        if(guideAvailable&&(guideToggle().contains(x,y)||scanButton().contains(x,y)||(guideOpen&&guidePanel().contains(x,y)))) return true;
        if(locationAvailable&&(locationToggle().contains(x,y)||(locationOpen&&locationPanel().contains(x,y)))) return true;
        if(venusControls&&venusPanel().contains(x,y)) return true;
        if((ioView||callistoView||satelliteView)&&ioPanel().contains(x,y)) return true;
        if(earthControls&&earthPanel().contains(x,y)) return true;
        if(earthControls&&!earthView&&(earthWhole().contains(x,y)||earthApproach().contains(x,y))) return true;
        if(saturnControls&&saturnPanel().contains(x,y)) return true;
        if(jupiterControls&&jupiterPanel().contains(x,y)) return true;
        if((earthView||venusView||ioView||callistoView||satelliteView)&&legend().contains(x,y)) return true;
        if(jupiterView||saturnView||earthView||venusView||ioView||callistoView||satelliteView) {
            for(Rect r:{title(),destinations(),fullscreenButton(),homeButton(),scalePanel(),readout()})
                if(r.contains(x,y)) return true;
            return false;
        }
        if(mercuryView||moonView) {
            for(Rect r:{title(),destinations(),fullscreenButton(),reliefButton(),homeButton(),scalePanel(),
                        environmentButton(),readout(),legend()}) if(r.contains(x,y)) return true;
            return false;
        }
        if(!marsView) {
            for(Rect r:{Rect{20,20,300,74},bodyList(),openBody(),fullscreenButton(),navigationUp(),homeButton(),
                        scalePanel(),readout(),legend()}) if(r.contains(x,y)) return true;
            return false;
        }
        if((satellite?orbitPanel():destinations()).contains(x,y)||orbitButton().contains(x,y)) return true;
        for(Rect r:{title(),fullscreenButton(),reliefButton(),homeButton(),scalePanel(),
                    environmentButton(),readout(),legend()}) if(r.contains(x,y)) return true;
        return environmentOpen&&environmentPanel().contains(x,y);
    }
};
} // namespace mars
