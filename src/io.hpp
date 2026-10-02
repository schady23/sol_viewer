#pragma once
#include "faceted.hpp"
#include <array>
#include <filesystem>
#include <vector>

namespace io {
constexpr double radiusKm=1821.49;
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
    {"PELE",-18.71,104.72,1.65f},
    {"LOKI PATERA",13.01,51.21,1.45f},
    {"PROMETHEUS",-1.52,206.06,1.55f},
    {"TVASHTAR PATERAE",62.76,236.47,1.65f}
}};
}
