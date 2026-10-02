#pragma once
#include "math.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace faceted {
inline constexpr int width=4096,height=2048;
struct FacetVertex { mars::Vec3 direction; std::array<unsigned char,4> color; };
std::vector<unsigned char> loadRgb(const std::filesystem::path& path,const std::string& magic,const std::string& label,int mapWidth=width,int mapHeight=height);
// Uniform geodesic faces with a deterministic 5-bit palette; spherical geometry.
std::vector<FacetVertex> makeFacetedGlobe(const std::vector<unsigned char>& rgb,int subdivisions=8);
}
