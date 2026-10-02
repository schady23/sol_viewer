#pragma once
#include "math.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace mars {
// Intentionally accepts only the verified 16 ppd topography product, not arbitrary PDS.
void validateMolaLabel(const std::string& text);
std::string readText(const std::filesystem::path& path);
std::int16_t decodeSigned16(std::uint8_t high, std::uint8_t low);

class HeightField {
public:
    static constexpr std::uint32_t width=5760, height=2880, ppd=16;
    explicit HeightField(std::vector<std::int16_t> samples);
    static HeightField importMola(const std::filesystem::path& label, const std::filesystem::path& image);
    static HeightField load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;
    float sample(double lat, double lon) const;
    const std::vector<std::int16_t>& samples() const { return samples_; }
private:
    std::vector<std::int16_t> samples_;
    float north_{}, south_{};
};
struct TerrainVertex { Vec3 direction; float meters; };
struct TerrainBatch {
    std::uint32_t first{},count{};
    Vec3 centre;
    float halfAngle{},minimum{},maximum{};
};
struct Mesh {
    std::vector<TerrainVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<TerrainBatch> batches;
    float chordFactor=1;
};
Mesh makeGlobe(const HeightField& field, int columns=5760); // 0.0625 degree, native MOLA grid
Mesh makeSampledGlobe(int columns,const std::function<float(double,double)>& sample);
bool batchVisible(const TerrainBatch& batch,const Camera& camera,float exaggeration,float aspect,float chordFactor,
                  float radius=radiusMeters,float minimum=-8177);

struct Landmark { const char* name; const char* description; double lat,lon; float distance; };
inline constexpr Landmark landmarks[] = {
    {"OLYMPUS MONS", "SHIELD VOLCANO", 18.65,226.20,1.32f},
    {"VALLES MARINERIS", "CANYON SYSTEM", -14.01,301.41,1.65f},
    {"HELLAS PLANITIA", "IMPACT BASIN", -42.43,70.50,1.8f},
    {"ELYSIUM PLANITIA", "VOLCANIC PLAINS", 2.98,154.74,1.8f}
};
} // namespace mars
