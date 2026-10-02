#pragma once
#include <algorithm>
#include <cmath>

namespace mars {
constexpr double pi = 3.14159265358979323846;
constexpr double radians = pi / 180.0;
constexpr float radiusMeters = 3396000.0f; // Display sphere: label's map radius, not the areoid.
struct Vec3 {
    float x{}, y{}, z{};
    Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec3 operator*(float b) const { return {x*b,y*b,z*b}; }
};
inline float dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
inline Vec3 normalized(Vec3 a) { return a*(1.0f/std::sqrt(dot(a,a))); }
inline double wrapLongitude(double lon) { return lon-360.0*std::floor(lon/360.0); }
inline Vec3 direction(double lat, double lon) {
    double p=lat*radians, l=wrapLongitude(lon)*radians;
    return {float(std::cos(p)*std::cos(l)),float(std::sin(p)),float(-std::cos(p)*std::sin(l))};
}
struct Location { double lat{}, lon{}; };
inline Location location(Vec3 p) {
    p=normalized(p);
    return {std::asin(std::clamp(double(p.y),-1.0,1.0))/radians,
            wrapLongitude(std::atan2(-double(p.z),double(p.x))/radians)};
}
struct Camera {
    double lat=12.0, lon=253.0;
    float distance=2.68f;
    static constexpr float minDistance=1.085f, maxDistance=8.0f;
    static constexpr float nearPlane=0.005f, farPlane=20.0f, tanHalfFov=0.41421356237f;
    bool flight=false;
    Vec3 flightRight{},flightUp{},flightBack{},flightPosition{};
    float lens=tanHalfFov,clipNear=nearPlane;
    Vec3 outward() const { return flight?normalized(flightPosition):direction(lat,lon); }
    Vec3 position() const { return flight?flightPosition:outward()*distance; }
    Vec3 backward() const { return flight?flightBack:outward(); }
    Vec3 right() const { return flight?flightRight:direction(0,lon+90); }
    Vec3 up() const { return flight?flightUp:cross(outward(),right()); }
    void clamp() { lat=std::clamp(lat,-90.0,90.0); lon=wrapLongitude(lon); distance=std::clamp(distance,minDistance,maxDistance); }
    Vec3 unproject(float nx, float ny, float depth, float aspect) const {
        float z=2*clipNear*farPlane/(farPlane+clipNear-(2*depth-1)*(farPlane-clipNear));
        return position() + (right()*(nx*lens*aspect)+up()*(ny*lens)-backward())*z;
    }
};
} // namespace mars
