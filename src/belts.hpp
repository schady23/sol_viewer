#pragma once
#include "catalog.hpp"
#include <cstdint>
#include <algorithm>
#include <stdexcept>
namespace solar::belts {
inline bool region(BodyId id) { return id==BodyId::AsteroidBelt||id==BodyId::KuiperBelt; }
inline bool isDwarf(BodyId id) { return id>=BodyId::Ceres&&id<=BodyId::Eris; }
// Regions organize navigation; every dwarf still orbits the Sun physically.
struct Dwarf { BodyId id; double radiusKm,boundingKm,a,e,i,node,peri,phase; };
inline const std::array<Dwarf,5>& dwarfs() {
    // Rounded JPL SBDB elements, retrieved 2026-09-30. Authored mean anomalies
    // spread the atlas anchors; these are NOT present-day positions.
    static const std::array<Dwarf,5> data{{
        {BodyId::Ceres,469.7,482.48026720504583,2.77,.0797,10.6,80.2,73.3,210},
        {BodyId::Pluto,1188.3,1188.6480561937703,39.6,.252,17.1,110,114,30},
        {BodyId::Haumea,798,1161,43.1,.194,28.2,122,241,190},
        {BodyId::Makemake,715,715,45.6,.159,29,79.3,297,310},
        {BodyId::Eris,1163,1163,67.9,.438,43.9,36,151,180}
    }}; return data;
}
inline const Dwarf& dwarf(BodyId id) {
    for(const auto& d:dwarfs()) if(d.id==id) return d;
    throw std::runtime_error("Not a dwarf planet");
}
inline double units(BodyId region) { return region==BodyId::AsteroidBelt?1.:.025; }
inline double distance(BodyId region) { return region==BodyId::AsteroidBelt?12.:8.; }
inline mars::Vec3 orbit(double a,double e,double inclination,double node,double peri,double mean,double scale) {
    double m=mean*mars::radians,E=m;
    for(int j=0;j<12;++j) E-=(E-e*std::sin(E)-m)/(1-e*std::cos(E));
    double x=a*(std::cos(E)-e),z=a*std::sqrt(1-e*e)*std::sin(E);
    double w=peri*mars::radians,inc=inclination*mars::radians,n=node*mars::radians;
    double px=x*std::cos(w)-z*std::sin(w),pz=x*std::sin(w)+z*std::cos(w);
    double y=pz*std::sin(inc); pz*=std::cos(inc);
    return {float((px*std::cos(n)-pz*std::sin(n))*scale),float(y*scale),float(-(px*std::sin(n)+pz*std::cos(n))*scale)};
}
inline mars::Vec3 position(const Dwarf& d,double scale) { return orbit(d.a,d.e,d.i,d.node,d.peri,d.phase,scale); }
struct Sample { mars::Vec3 point; float brightness; double a,e,i; int population; };
inline std::vector<Sample> samples(BodyId region) {
    // Fixed integer PRNG makes the atlas reproducible across platforms. Samples
    // explain orbital families, not a census, ephemeris or visible rock sizes.
    std::uint32_t seed=region==BodyId::AsteroidBelt?0x57ab13u:0x919abu;
    auto random=[&]() { seed=1664525u*seed+1013904223u; return double(seed>>8)/16777216.; };
    std::vector<Sample> out; int count=region==BodyId::AsteroidBelt?3600:2800;
    for(int j=0;j<count;++j) {
        double a,e,i; int family=0;
        if(region==BodyId::AsteroidBelt) {
            do { a=2.1+1.2*random(); } while(std::abs(a-2.50)<.025||std::abs(a-2.82)<.022||std::abs(a-2.96)<.012||std::abs(a-3.28)<.018);
            e=.015+.16*random(); i=20*std::pow(random(),2.0);
        } else {
            double choice=random();
            if(choice<.50) { a=42+5*random(); e=.01+.07*random(); i=5*random(); }
            else if(choice<.78) { family=1; a=40+10*random(); e=.04+.20*random(); i=30*random(); }
            else if(choice<.92) { family=2; a=39.4+.3*(random()-.5); e=.12+.16*random(); i=25*random(); }
            else { family=3; a=55+40*random(); double q=30+10*random(); e=1-q/a; i=40*random(); }
        }
        double node=360*random(),peri=360*random(),mean=360*random();
        auto p=orbit(a,e,i,node,peri,mean,1);
        out.push_back({p,float(.35+.65*random()),a,e,i,family});
    } return out;
}
inline const std::vector<Sample>& distribution(BodyId id) {
    static const auto main=samples(BodyId::AsteroidBelt),kuiper=samples(BodyId::KuiperBelt);
    return id==BodyId::AsteroidBelt?main:kuiper;
}
}
