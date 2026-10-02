#pragma once
#include "terrain.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace earth {
constexpr double loopSeconds=60;
constexpr float radiusMeters=6371000.f;
// At 1x scale the chords must stay above 9-km terrain under the 10-km shell.
constexpr int cloudColumns=512;
inline float cloudShellRadius(float relief,int layer) {
    return 1.f+(layer==0?10000.f:16000.f)*relief/radiusMeters;
}
struct Clouds {
    double seconds=0;
    bool visible=true,paused=false;
    unsigned revision=0;
    void advance(double dt);
    void reset() { seconds=0; ++revision; }
    double fraction() const;
    // Closed circular path. These coordinates, not unbounded time, enter the shader.
    mars::Vec3 phase() const;
};
// Periodic gradient-noise volume, generated once to gently deform the cloud map.
std::vector<unsigned char> cloudNoise();
struct Image {
    int width{},height{};
    std::vector<unsigned char> rgb;
    static Image load(const std::filesystem::path& path);
    static Image loadClouds(const std::filesystem::path& path);
};
struct HeightField {
    static constexpr int width=2400,height=1200;
    std::vector<std::int16_t> meters;
    float north=0,south=0;
    static HeightField load(const std::filesystem::path& path);
    float sample(double lat,double lon) const;
};
// Indexed terrain shared by rendering and regression tests.
mars::Mesh makeGlobe(const HeightField& field,int columns=HeightField::width);
struct Landmark { const char* name; double lat,lon; float distance; bool seafloor; };
// Regional viewing anchors, not individual DEM summit/depth measurements.
inline constexpr std::array<Landmark,4> landmarks{{
    {"MOUNT EVEREST",27.99,86.93,1.30f,false},
    {"MARIANA TRENCH",11.37,142.59,1.35f,true},
    {"GRAND CANYON",36.10,-112.10,1.20f,false},
    {"MAUNA LOA",19.475,-155.608,1.18f,false}
}};
}
