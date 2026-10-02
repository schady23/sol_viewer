#pragma once
#include "terrain.hpp"
#include <limits>
namespace mercury {
constexpr float radiusMeters=2439400, minimumMeters=-5382, maximumMeters=4497;
constexpr float minimumDistance=1.035f;
constexpr int sourcePpd=64;
constexpr std::int16_t missing=-32768;
// Only MSGR_DEM_USG_SC_I_V02 is accepted by the importer.
void validateLabel(const std::string& label);
class HeightField {
public:
    // Smaller global grids are useful for deterministic sampler tests. Disk files are always 64 ppd.
    HeightField(int ppd,std::vector<std::int16_t> samples);
    static HeightField importPds(const std::filesystem::path& label,const std::filesystem::path& image);
    static HeightField load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;
    float sample(double lat,double lon) const;
    std::size_t missingCount() const { return missingCount_; }
private:
    int ppd_,width_,height_;
    std::vector<std::int16_t> samples_;
    float north_,south_;
    std::size_t missingCount_=0;
};
inline constexpr mars::Landmark landmarks[]={
    {"CALORIS PLANITIA","IMPACT BASIN",31.65,161.98,1.40f},
    {"REMBRANDT","IMPACT BASIN",-32.83,87.54,1.22f},
    {"RACHMANINOFF","PEAK-RING BASIN",27.79,57.58,1.12f},
    {"BEETHOVEN","IMPACT BASIN",-20.92,236.34,1.22f}
};
}
