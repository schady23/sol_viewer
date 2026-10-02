#include "orbit.hpp"
#include <stdexcept>

namespace mars {
void SatelliteOrbit::configure(OrbitElements e) {
    if(!std::isfinite(e.periapsisKm)||!std::isfinite(e.apoapsisKm)||!std::isfinite(e.inclinationDeg))
        throw std::runtime_error("VALUES MUST BE FINITE");
    if(e.periapsisKm<minAltitudeKm||e.apoapsisKm>maxAltitudeKm||e.apoapsisKm<minAltitudeKm||e.periapsisKm>maxAltitudeKm)
        throw std::runtime_error("ALTITUDE: 100 TO 20000 KM");
    if(e.periapsisKm>e.apoapsisKm) throw std::runtime_error("PERIAPSIS EXCEEDS APOAPSIS");
    if(e.inclinationDeg<0||e.inclinationDeg>180) throw std::runtime_error("INCLINATION: 0 TO 180 DEG");
    elements_=e;
}
void SatelliteOrbit::reset(double node,double phase) {
    if(!std::isfinite(node)||!std::isfinite(phase)) throw std::runtime_error("Invalid orbit orientation");
    nodeLongitude_=wrapLongitude(node); meanAnomaly_=wrapLongitude(phase)*radians; spinAngle_=0;
    resetLook();
}
double SatelliteOrbit::semiMajorKm() const { return radiusKm+(elements_.periapsisKm+elements_.apoapsisKm)*0.5; }
double SatelliteOrbit::eccentricity() const { return (elements_.apoapsisKm-elements_.periapsisKm)/(2*semiMajorKm()); }
double SatelliteOrbit::periodSeconds() const { return 2*pi*std::sqrt(std::pow(semiMajorKm(),3)/mu); }
void SatelliteOrbit::advance(double seconds) {
    if(!enabled||paused||!std::isfinite(seconds)||seconds<=0) return;
    double dt=seconds*timeScale;
    meanAnomaly_=std::fmod(meanAnomaly_+2*pi*std::fmod(dt,periodSeconds())/periodSeconds(),2*pi);
    spinAngle_=std::fmod(spinAngle_+2*pi*std::fmod(dt,rotationSeconds)/rotationSeconds,2*pi);
}
void SatelliteOrbit::cycleSpeed() { timeScale=timeScale<10?10:timeScale<60?60:timeScale<300?300:1; }
OrbitState SatelliteOrbit::state() const {
    double a=semiMajorKm(),e=eccentricity();
    // Bracketed solve of E - e sin E = M; bounded and stable for circular/high-e orbits.
    double lo=0,hi=2*pi;
    for(int i=0;i<48;++i) {
        double mid=(lo+hi)*0.5;
        if(mid-e*std::sin(mid)<meanAnomaly_) lo=mid; else hi=mid;
    }
    double E=(lo+hi)*0.5,c=std::cos(E),s=std::sin(E),minor=std::sqrt(1-e*e);
    double radius=a*(1-e*c),rate=std::sqrt(mu/(a*a*a))/(1-e*c);
    double node=nodeLongitude_-spinAngle_/radians,inc=elements_.inclinationDeg*radians;
    Vec3 p=direction(0,node),q=direction(0,node+90)*float(std::cos(inc))+Vec3{0,float(std::sin(inc)),0};
    return {p*float(a*(c-e))+q*float(a*minor*s),
            p*float(-a*s*rate)+q*float(a*minor*c*rate),
            radius-radiusKm,std::sqrt(mu*(2/radius-1/a)),E};
}
Camera SatelliteOrbit::camera() const {
    auto s=state();
    Vec3 radial=normalized(s.positionKm);
    Vec3 tangent=normalized(s.velocityKmS-radial*dot(s.velocityKmS,radial));
    Vec3 side=cross(tangent,radial);
    double yaw=lookYaw*radians;
    tangent=normalized(tangent*float(std::cos(yaw))+side*float(std::sin(yaw)));
    double horizon=std::asin(radiusKm/(radiusKm+s.altitudeKm))/radians;
    double tilt=std::clamp((limbView?std::max(0.0,horizon-12):0.0)+lookPitch,0.0,88.0)*radians;
    auto l=location(radial);
    Camera c{l.lat,l.lon,float(1+s.altitudeKm/radiusKm)};
    c.flight=true;
    c.flightPosition=s.positionKm*float(1/radiusKm);
    c.flightBack=radial*float(std::cos(tilt))-tangent*float(std::sin(tilt));
    c.flightUp=radial*float(std::sin(tilt))+tangent*float(std::cos(tilt));
    c.flightRight=normalized(cross(c.flightUp,c.flightBack));
    c.lens=float(std::tan(std::clamp(fovDegrees,25.0,90.0)*radians*0.5));
    return c;
}
} // namespace mars
