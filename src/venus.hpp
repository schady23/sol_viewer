#pragma once
#include "earth.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace venus {
constexpr float radiusMeters=6051000.f;
using Clouds=earth::Clouds;
struct SurfaceMap {
    static constexpr int width=1440,height=720;
    std::vector<unsigned char> rgb;
    static SurfaceMap load(const std::filesystem::path& path);
};
struct HeightField {
    static constexpr int width=4096,height=2048;
    std::vector<std::int16_t> meters;
    float north=0,south=0;
    static HeightField load(const std::filesystem::path& path);
    float sample(double lat,double lon) const;
};
struct FacetVertex { mars::Vec3 direction; float meters; float faceMeters; };
std::vector<FacetVertex> makeFacetedGlobe(const HeightField& field,int columns=2048);
struct Region { const char* name; double lat,lon; };
inline constexpr std::array<Region,4> regions{{
    {"ISHTAR TERRA",65,15}, {"APHRODITE TERRA",-10,105},
    {"BETA REGIO",25,282}, {"ATLA REGIO",9,200}
}};
}
