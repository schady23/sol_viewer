#pragma once
#include "math.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace saturn {
constexpr double equatorialKm=60268,polarKm=54364,axisRatio=polarKm/equatorialKm;
constexpr double rotationSeconds=10.656*3600,loopSeconds=rotationSeconds*10;
constexpr double ringInnerKm=74658,ringOuterKm=139876;
constexpr double cassiniInnerKm=117507,cassiniOuterKm=122340,enckeKm=133410;
// NASA GLB radial texture span, in equatorial radii (preserved UV mapping).
constexpr double sourceRingInner=1.2323,sourceRingOuter=2.3309;
constexpr std::array<double,4> rates{{1,60,600,3600}};
double centricLatitude(double graphic);
double graphicLatitude(double centric);
double jetSpeed(double graphicLatitude); // Illustrative jets, not measured time-varying weather.
struct Clock {
    double seconds=0;
    int speed=3;
    bool paused=false;
    unsigned revision=0;
    void advance(double realSeconds);
    void setRate(double rate);
    void cycleRate(int delta=1);
    void reset();
    double rotation() const;
    double loopFraction() const;
};
struct Image {
    int width{},height{};
    std::vector<unsigned char> rgb;
    static Image load(const std::filesystem::path& path);
};
struct Feature { const char* name; double latitude,longitude; float distance; };
inline const std::array<Feature,4> features{{
    {"NORTH POLAR HEXAGON",90,130,4.1f},
    {"EQUATORIAL CLOUDS",0,130,3.5f},
    {"NORTHERN BANDS",35,130,3.2f},
    {"SOUTHERN BANDS",-35,130,3.2f}
}};
struct RingImage {
    std::vector<unsigned char> rgba;
    static RingImage load(const std::filesystem::path& path);
};
std::vector<mars::Vec3> makeRings(int segments=2048);
}
