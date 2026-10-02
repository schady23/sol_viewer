#pragma once
#include "math.hpp"
#include <array>
#include <string>
#include <vector>

namespace solar {
enum class BodyId { Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune,
                    Moon, Phobos, Deimos, Io, Europa, Ganymede, Callisto, Titan, Titania, Triton, Mimas, Enceladus, Tethys, Dione, Rhea, Iapetus, Miranda, Ariel, Umbriel, Oberon, Proteus, AsteroidBelt, KuiperBelt, Ceres, Pluto, Haumea, Makemake, Eris, None };
enum class BodyKind { Star, Planet, Moon, Region, DwarfPlanet };
struct Body {
    BodyId id,parent;
    const char* name;
    BodyKind kind;
    mars::Vec3 color;
    float displayRadius; // Schematic only; never a physical radius.
    int style; // 0 solid, 1 bands, 2 rings, 3 unlit star.
    const char* dataset; // Empty for placeholders.
};
struct SurfaceDataset {
    const char* id; BodyId body;
    const char* path; const char* format; const char* coordinates; const char* datum;
    double referenceRadiusKm; int pixelsPerDegree;
    const char* coverage; const char* provenance;
};
constexpr int bodyCount=37;
const std::array<Body,bodyCount>& bodies();
const Body& body(BodyId id);
BodyId bodyNamed(std::string name);
std::vector<BodyId> children(BodyId parent);
const SurfaceDataset& molaDataset();
const SurfaceDataset& mercuryDataset();
const SurfaceDataset& moonDataset();
// Solar overview: one scene unit is ten astronomical units for both radii and
// orbital distances. See docs/SOLAR_SCALE.md for sources and static-layout limits.
constexpr double astronomicalUnitKm=149597870.7,solarUnitsPerAu=.1;
struct SolarBody { BodyId id; double radiusKm,orbitAu; };
const std::array<SolarBody,9>& solarBodies();
const SolarBody& solarBody(BodyId id);
float solarBodyRadius(BodyId id);
// One common length scale in the Jovian system (714,920 km per scene unit).
// Body radii and representative orbital distances are never scaled separately.
constexpr double jovianRadiusKm=71492.0;
constexpr float jovianSceneRadius=.1f;
struct JovianMoon { BodyId id; double radiusKm,orbitKm; };
const std::array<JovianMoon,4>& jovianMoons();
// Earth local system: 100,000 km per scene unit, for sizes AND distances.
// NASA mean spherical radii and JPL mean lunar semimajor axis: docs/EARTH.md.
constexpr double earthRadiusKm=6371.0,moonRadiusKm=1737.4,moonOrbitKm=384400.0;
constexpr double earthSystemKmPerUnit=100000.0;
// Mars local system: a single 10,000 km length scale for sizes and distances.
constexpr double marsRadiusKm=3396.19,marsSystemKmPerUnit=10000.0;
struct MartianMoon { BodyId id; double radiusKm,orbitKm,boundingRatio; };
const std::array<MartianMoon,2>& martianMoons();
// Saturn: one scene unit is 1,000,000 km, including the rings and all moons.
constexpr double saturnRadiusKm=60268.,saturnSystemKmPerUnit=1000000.;
struct SaturnianMoon { BodyId id; double radiusKm,orbitKm,boundingRatio; };
const std::array<SaturnianMoon,7>& saturnianMoons();
constexpr double outerSystemKmPerUnit=1000000.;
struct OuterMoon { BodyId id; double radiusKm,orbitKm,boundingRatio,inclination; };
const std::vector<OuterMoon>& outerMoons(BodyId parent);
float systemBodyRadius(BodyId parent,BodyId id);
float laneRadius(BodyId parent,std::size_t index);
mars::Vec3 mapPosition(BodyId parent,BodyId child);
} // namespace solar
