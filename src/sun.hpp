#pragma once
#include "math.hpp"
namespace sun {
inline constexpr double radiusKm=695700.,loopSeconds=90.;
struct Clock {
    double seconds=0;
    bool paused=false;
    int speed=1;
    void advance(double dt) { if(!paused&&std::isfinite(dt)&&dt>0) seconds=std::fmod(seconds+std::min(dt,.25)*speed,loopSeconds); }
    double phase() const { return seconds/loopSeconds*2*mars::pi; }
    void cycle() { speed=speed==1?3:speed==3?10:1; }
};
}
