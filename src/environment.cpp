#include "environment.hpp"

namespace mars {
Environment::Environment(std::uint32_t seed):random_(seed?seed:1) { untilNext_=random(240,420); }
double Environment::random(double lo,double hi) {
    random_^=random_<<13; random_^=random_>>17; random_^=random_<<5;
    return lo+(hi-lo)*(double(random_)/4294967296.0);
}
void Environment::beginStorm(Location origin) {
    origin_=origin; age_=0; duration_=random(60,120); active_=true;
}
void Environment::previewStorm(Location near) {
    dust=true; topography=false;
    beginStorm({std::clamp(near.lat,-90.0,90.0),wrapLongitude(near.lon)});
    age_=duration_*0.35; // Immediately visible, including when the clock is paused.
}
void Environment::update(double seconds) {
    if(paused||!std::isfinite(seconds)||seconds<=0) return;
    // Process event boundaries so the seeded sequence is independent of frame rate.
    while(seconds>0) {
        double remaining=active_?duration_-age_:untilNext_;
        double step=std::min(seconds,remaining);
        time_+=step; seconds-=step;
        if(active_) age_+=step; else untilNext_-=step;
        if(step>=remaining) {
            if(active_) { active_=false; untilNext_=random(240,420); }
            else {
                // Explicit order matters for a repeatable sequence across compilers.
                double lat=random(-60,45),lon=random(0,360);
                beginStorm({lat,lon});
            }
        }
    }
}
float Environment::stormStrength() const {
    if(!active_) return 0;
    float t=float(age_/duration_);
    auto smooth=[](float x) { x=std::clamp(x,0.0f,1.0f); return x*x*(3-2*x); };
    return smooth(t/0.22f)*smooth((1-t)/0.38f);
}
float Environment::stormRadius() const { return active_?float(0.08+0.23*age_/duration_):0.08f; }
Vec3 Environment::stormCentre() const {
    return direction(origin_.lat+2.0*std::sin(age_*0.025),origin_.lon+age_*0.065);
}
} // namespace mars
