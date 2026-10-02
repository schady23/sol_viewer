#pragma once
#include "math.hpp"

namespace mars {
struct OrbitElements { double periapsisKm=250,apoapsisKm=1500,inclinationDeg=45; };
struct OrbitState {
    Vec3 positionKm,velocityKmS; // Inertial velocity expressed in body-fixed axes.
    double altitudeKm{},speedKmS{},eccentricAnomaly{};
};
// Ideal two-body ellipse, constant Mars spin, arbitrary epoch. No SPICE dependency.
class SatelliteOrbit {
public:
    static constexpr double mu=42828.37362069909; // NAIF gm_de440.tpc, BODY499_GM, km^3/s^2.
    static constexpr double radiusKm=3396.0; // Same reference sphere as the MOLA display.
    static constexpr double rotationSeconds=24.6229*3600; // NASA Mars Fact Sheet, sidereal.
    static constexpr double minAltitudeKm=100,maxAltitudeKm=20000;
    bool enabled=false,paused=false,limbView=true;
    double timeScale=60,lookYaw=0,lookPitch=0,fovDegrees=50;
    const OrbitElements& elements() const { return elements_; }
    void configure(OrbitElements elements); // Throws on non-finite/unordered/unsupported values.
    void reset(double nodeLongitude,double phaseDegrees=0);
    void advance(double realSeconds);
    void cycleSpeed();
    void resetLook() { lookYaw=lookPitch=0; fovDegrees=50; }
    double eccentricity() const;
    double semiMajorKm() const;
    double periodSeconds() const;
    double meanAnomaly() const { return meanAnomaly_; }
    OrbitState state() const;
    Camera camera() const;
private:
    OrbitElements elements_;
    double meanAnomaly_=0,spinAngle_=0,nodeLongitude_=253;
};
} // namespace mars
