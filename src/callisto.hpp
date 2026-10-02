#pragma once
#include "faceted.hpp"
#include <array>
#include <filesystem>
#include <vector>

namespace callisto {
constexpr double radiusKm=2410.3;
struct Image {
    static constexpr int width=4096,height=2048;
    std::vector<unsigned char> rgb;
    static Image load(const std::filesystem::path& path);
};
using FacetVertex=faceted::FacetVertex;
// Uniform geodesic triangles: no longitude seam or sliver-shaped polar faces.
std::vector<FacetVertex> makeFacetedGlobe(const Image& image,int subdivisions=8);
struct Region { const char* name; double lat,lon; float distance; };
// Regional anchors from the USGS Gazetteer. West longitudes are converted to
// east for the atlas; the spherical viewing approximation is not a terrain DEM.
inline constexpr std::array<Region,4> regions{{
    {"VALHALLA",14.7,304.0,1.85f},
    {"ASGARD",32.2,220.1,1.65f},
    {"ADLINDA",-48.5,324.4,1.55f},
    {"LOFN",-56.5,337.7,1.38f}
}};
}
