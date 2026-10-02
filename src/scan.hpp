#pragma once
#include <algorithm>
#include <cmath>
namespace solar {
// Visual archive sweep, deliberately not a fictitious download percentage.
struct SurfaceScan {
    double seconds=0;
    float remaining=0;
    bool wasLoading=false;
    void replay() { seconds=0; remaining=3; }
    void update(double dt,bool loading) {
        dt=std::clamp(dt,0.0,.25);
        if(loading&&!wasLoading) seconds=0;
        if(!loading&&wasLoading) remaining=.7f;
        if(loading||remaining>0) seconds+=dt;
        if(!loading) remaining=std::max(0.f,remaining-float(dt));
        wasLoading=loading;
    }
    float phase() const { return float(std::fmod(seconds/2.2,1.0)); }
    float strength() const { return wasLoading?1:std::min(remaining/.7f,1.f); }
};
}
