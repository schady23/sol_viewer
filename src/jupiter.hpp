#pragma once
#include "math.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace jupiter {
constexpr double equatorialKm=71492,polarKm=66854,axisRatio=polarKm/equatorialKm;
constexpr double rotationSeconds=9.925*3600,loopSeconds=rotationSeconds*10;
constexpr double spotLatitude=-22.4,spotLongitude=233.2; // 2015 map anchor, planetographic / east.
constexpr std::array<double,4> rates{{1,60,600,3600}};
double centricLatitude(double graphic);
double graphicLatitude(double centric);
double jetSpeed(double graphicLatitude); // Authored approximation, m/s eastward, not retrieved winds.
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
    {"GREAT RED SPOT",spotLatitude,spotLongitude,1.48f},
    {"EQUATORIAL ZONE",0,210,1.8f},
    {"NORTH EQUATORIAL BELT",15,210,1.65f},
    {"SOUTH EQUATORIAL BELT",-15,245,1.65f}
}};
}
