#include "orbit.hpp"
#include "ui.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void near(double a,double b,double tolerance=0.001) { check(std::abs(a-b)<tolerance,"Orbital numeric check failed"); }
double length(mars::Vec3 v) { return std::sqrt(mars::dot(v,v)); }
}
int main() {
    try {
        using O=mars::SatelliteOrbit;
        O orbit; orbit.enabled=true; orbit.timeScale=1;
        for(auto e:{mars::OrbitElements{250,1500,45},{100,20000,90},{100,100,0},{20000,20000,180}}) {
            orbit.configure(e); orbit.reset(0); auto peri=orbit.state();
            near(peri.altitudeKm,e.periapsisKm);
            near(length(peri.positionKm),O::radiusKm+e.periapsisKm,0.003);
            double energy=0.5*mars::dot(peri.velocityKmS,peri.velocityKmS)-O::mu/(O::radiusKm+peri.altitudeKm);
            near(energy,-O::mu/(2*orbit.semiMajorKm()));
            double momentum=length(mars::cross(peri.positionKm,peri.velocityKmS));
            orbit.advance(orbit.periodSeconds()/2); auto apo=orbit.state();
            near(apo.altitudeKm,e.apoapsisKm);
            check(peri.speedKmS+1e-9>=apo.speedKmS,"Periapsis should be the fastest part of the ellipse");
            near(length(mars::cross(apo.positionKm,apo.velocityKmS)),momentum,0.02);
            orbit.advance(orbit.periodSeconds()/2);
            near(orbit.state().altitudeKm,e.periapsisKm);
            // Return in inertial space, but Mars has rotated beneath the satellite.
            auto expected=mars::direction(0,-360*orbit.periodSeconds()/O::rotationSeconds);
            check(mars::dot(mars::normalized(orbit.state().positionKm),expected)>0.99999f,"Ground track ignored Mars rotation");
        }
        orbit.configure({500,500,90}); orbit.reset(0); orbit.advance(orbit.periodSeconds()/4);
        near(mars::location(orbit.state().positionKm).lat,90,0.01);
        orbit.configure({500,500,0}); orbit.reset(0); auto prograde=orbit.state();
        orbit.configure({500,500,180}); auto retrograde=orbit.state();
        check(mars::dot(prograde.velocityKmS,retrograde.velocityKmS)<0,"Retrograde orbit must reverse motion");
        double mean=orbit.meanAnomaly(); orbit.paused=true; orbit.advance(100);
        near(orbit.meanAnomaly(),mean); orbit.paused=false;
        orbit.enabled=false; orbit.advance(100); near(orbit.meanAnomaly(),mean); orbit.enabled=true;
        for(auto bad:{mars::OrbitElements{50,1000,0},{1000,500,0},{100,20001,0},{100,500,181},{100,500,-1},
                      {NAN,500,0},{100,INFINITY,90}}) {
            bool rejected=false; try { orbit.configure(bad); } catch(const std::exception&) { rejected=true; }
            check(rejected,"Invalid orbital elements accepted");
        }
        // Generalized view matrices must stay orthonormal and pick accurately across
        // both poles, circular/high-e orbits, horizon/nadir views and optical zoom.
        for(double inc:{0.0,90.0,180.0}) for(double phase:{0.0,90.0,180.0,270.0}) for(bool limb:{false,true}) {
            orbit.configure({100,20000,inc}); orbit.reset(359.99,phase); orbit.limbView=limb;
            for(double fov:{25.0,50.0,90.0}) {
                orbit.fovDegrees=fov; auto c=orbit.camera();
                near(length(c.right()),1); near(length(c.up()),1); near(length(c.backward()),1);
                near(mars::dot(c.right(),c.up()),0); near(mars::dot(c.up(),c.backward()),0);
                check(mars::dot(mars::cross(c.right(),c.up()),c.backward())>0.9999f,"Flight camera handedness");
                for(float aspect:{0.8f,1.6f,2.4f}) {
                    auto world=c.position()-c.backward()*0.25f+c.right()*0.02f+c.up()*0.01f;
                    auto rel=world-c.position(); float z=-mars::dot(rel,c.backward());
                    float nx=mars::dot(rel,c.right())/(z*c.lens*aspect),ny=mars::dot(rel,c.up())/(z*c.lens);
                    float n=mars::Camera::nearPlane,f=mars::Camera::farPlane;
                    float depth=f/(f-n)-f*n/((f-n)*z);
                    check(length(c.unproject(nx,ny,depth,aspect)-world)<0.00002,"Flight depth unprojection");
                }
            }
        }
        check(1+O::minAltitudeKm/O::radiusKm-(1+21171.0/mars::radiusMeters+0.010)>mars::Camera::nearPlane,
              "Lowest orbit clips terrain/weather shells");
        for(auto size:{mars::Rect{0,0,800,500},{0,0,1242,740},{0,0,1242,1508},{0,0,2560,1600}})
            for(float scale:{0.75f,1.0f,1.25f,1.5f,2.0f}) {
                mars::ViewLayout l; l.satellite=true; l.resize(size.w,size.h,scale);
                check(l.orbitPanel().y+l.orbitPanel().h<l.readout().y,"Orbit panel overlaps probe");
                check(l.orbitPanel().x+l.orbitPanel().w<l.environmentButton().x,"Orbit/environment panels overlap");
                for(auto r:{l.orbitButton(),l.orbitValue(2),l.orbitMinus(0),l.orbitPlus(1),l.orbitPause(),l.orbitSpeed(),l.orbitView()})
                    check(l.overHud(r.x+r.w/2.0f,r.y+r.h/2.0f),"Orbit control not covered by input layout");
            }
        std::cout<<"PASS: orbital apsides, speed, energy, momentum, rotation, inclination, pause, limits, camera and UI\n";
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
