#pragma once
#include "catalog.hpp"
#include <stdexcept>
namespace ice {
inline bool planet(solar::BodyId id) { return id==solar::BodyId::Uranus||id==solar::BodyId::Neptune; }
inline double equatorialKm(solar::BodyId id) { return id==solar::BodyId::Uranus?25559:24764; }
inline double polarKm(solar::BodyId id) { return id==solar::BodyId::Uranus?24973:24341; }
inline double tilt(solar::BodyId id) { return id==solar::BodyId::Uranus?97.77:28.32; }
inline double rotationSeconds(solar::BodyId id) { return (id==solar::BodyId::Uranus?17.24:16.11)*3600; }
inline mars::Vec3 rotate(mars::Vec3 p,double degrees) {
    auto c=float(std::cos(degrees*mars::radians)),s=float(std::sin(degrees*mars::radians));
    return {c*p.x-s*p.y,s*p.x+c*p.y,p.z};
}
struct Ring { double radiusKm,widthKm; float opacity; };
inline const std::vector<Ring>& rings(solar::BodyId id) {
    // Circular reference annuli, representative widths. Brightness is illustrative.
    static const std::vector<Ring> uranus{{41837,1.5,.30f},{42234,2,.35f},{42571,2,.30f},
        {44718,7,.35f},{45661,8,.30f},{47176,1.6,.30f},{47627,2.5,.30f},{48300,5,.35f},
        {50024,2,.20f},{51149,58,.50f},{67300,3800,.008f},{97700,17000,.005f}};
    static const std::vector<Ring> neptune{{41900,2000,.025f},{53200,80,.20f},
        {55200,4000,.012f},{57200,80,.16f},{62933,15,.25f}};
    return id==solar::BodyId::Uranus?uranus:neptune;
}
inline constexpr std::array<double,4> rates{{1,60,600,3600}};
struct Clock {
    double seconds=0;
    int speed=3;
    bool paused=false;
    void advance(double dt,solar::BodyId id) {
        if(!paused&&std::isfinite(dt)&&dt>0) seconds=std::fmod(seconds+std::min(dt,.25)*rates.at(speed),rotationSeconds(id)*10);
    }
    void cycle(int delta=1) { speed=(speed+delta+4)%4; }
    double rotation(solar::BodyId id) const { return std::fmod(seconds/rotationSeconds(id)*360,360.); }
    double phase(solar::BodyId id) const { return seconds/(rotationSeconds(id)*10)*2*mars::pi; }
};
}
