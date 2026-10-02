#pragma once
#include "math.hpp"
#include <cstdint>

namespace mars {
// Illustrative animation, not reconstructed Martian weather or a climate model.
class Environment {
public:
    bool ice=true,clouds=true,dust=true,paused=false,topography=false;
    explicit Environment(std::uint32_t seed=240926);
    void update(double seconds);
    void previewStorm(Location near);
    double time() const { return time_; }
    bool stormActive() const { return active_; }
    float stormStrength() const;
    float stormRadius() const; // Angular display radius, radians.
    Vec3 stormCentre() const;
    bool showIce() const { return ice&&!topography; }
    bool showClouds() const { return clouds&&!topography; }
    bool showDust() const { return dust&&!topography&&active_; }
private:
    std::uint32_t random_;
    double time_=0,untilNext_=0,age_=0,duration_=90;
    Location origin_{};
    bool active_=false;
    double random(double lo,double hi);
    void beginStorm(Location origin);
};
} // namespace mars
