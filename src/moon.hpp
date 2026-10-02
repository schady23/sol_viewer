#pragma once
#include "terrain.hpp"
#include <limits>
namespace moon {
constexpr float radiusMeters=1737400, minimumMeters=-9125.5f, maximumMeters=10773;
constexpr float minimumDistance=1.085f;
constexpr int sourcePpd=64;
// V3.1 declares no missing constant; reject samples outside the labelled DN range.
// Only LOLA LDEM_64 V3.1 is accepted by the importer.
void validateLabel(const std::string& label);
class HeightField {
public:
    // Smaller global grids are useful for deterministic sampler tests. Disk files are always 64 ppd.
    HeightField(int ppd,std::vector<std::int16_t> samples);
    static HeightField importPds(const std::filesystem::path& label,const std::filesystem::path& image);
    static HeightField load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;
    float sample(double lat,double lon) const;
private:
    int ppd_,width_,height_;
    std::vector<std::int16_t> samples_;
    float north_,south_;
};
struct Image {
    static constexpr int width=8192,height=4096;
    std::vector<unsigned char> rgb;
    static Image load(const std::filesystem::path& path);
};
inline constexpr mars::Landmark landmarks[]={
    {"APOLLO 11 / TRANQUILITY BASE","FIRST CREWED LANDING",0.67416,23.47314,1.12f},
    {"APOLLO 12 / OCEAN OF STORMS","CREWED LANDING",-3.0128,336.5781,1.12f},
    {"APOLLO 14 / FRA MAURO","CREWED LANDING",-3.64589,342.52806,1.12f},
    {"APOLLO 15 / HADLEY-APENNINE","CREWED LANDING",26.13239,3.63330,1.16f},
    {"APOLLO 16 / DESCARTES","CREWED LANDING",-8.9734,15.5011,1.12f},
    {"APOLLO 17 / TAURUS-LITTROW","CREWED LANDING",20.1911,30.7723,1.14f},
    {"TYCHO","RAY CRATER",-43.30,348.78,1.16f},
    {"COPERNICUS","COMPLEX CRATER",9.62,339.92,1.16f},
    {"ARISTARCHUS","BRIGHT CRATER",23.73,312.51,1.10f},
    {"PLATO","LAVA-FLOODED CRATER",51.62,350.62,1.18f},
    {"MARE IMBRIUM","VOLCANIC PLAIN",34.72,345.09,1.60f},
    {"REINER GAMMA","LUNAR SWIRL",7.39,301.04,1.14f},
    {"ORIENTALE","MULTI-RING BASIN",-19.87,265.33,1.55f},
    {"TSIOLKOVSKIY","FARSIDE CRATER",-20.38,128.97,1.26f},
    {"SOUTH POLE-AITKEN","ANCIENT IMPACT BASIN",-53,191,1.95f},
    {"SHACKLETON","SOUTH POLAR CRATER",-89.63,132.32,1.10f}
};
constexpr int landmarkCount=int(std::size(landmarks)),pageSize=4,pageCount=(landmarkCount+pageSize-1)/pageSize;
// Two orthographic polar slope patches, derived from LOLA (not invented relief).
constexpr int polarSize=512;
constexpr float polarExtent=.2588190451f; // sin(15 degrees): covers 75N..90N and 75S..90S.
std::vector<float> polarSlopes(const HeightField& field);
}
