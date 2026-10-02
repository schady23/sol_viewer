#pragma once
#include "guide.hpp"
#include <stdexcept>
namespace saturn {
inline const solar::Article& guide() {
    static const solar::Article article={"SATURN",{
        "Saturn is a hydrogen-and-helium gas giant with a strongly flattened shape. Its pale golden cloud bands and atmospheric haze give it a softer appearance than Jupiter. Winds, storms and a long-lived six-sided northern jet pattern create structure in its upper atmosphere. There is no solid surface to land on at the cloud tops.",
        "Explore the northern polar hexagon, equatorial clouds and northern or southern bands. This globe follows Jupiter's dense atmospheric rendering approach, but uses NASA VTAD's Saturn visualization texture, gentler cloud filaments and a modeled hexagon. The rings preserve a supplied radial appearance strip with authored transparency, physical representative radii and planet/ring shadows."
    },{
        {"EQUATOR / POLAR RADIUS","60268 / 54364 KM AT THE 1-BAR LEVEL"},
        {"REFERENCE ROTATION","10.656 HOURS / SYSTEM III PROXY"},
        {"MODEL LOOP","10 REFERENCE ROTATIONS / 106.56 HOURS"},
        {"ATMOSPHERE","HYDROGEN + HELIUM / HAZE + CLOUD BANDS"},
        {"MAIN RINGS","C / B / CASSINI DIVISION / A / F RING"},
        {"CASSINI DIVISION","117507 TO 122340 KM FROM CENTRE"},
        {"NORTH POLAR HEXAGON","ILLUSTRATIVE SIX-SIDED JET MODEL"},
        {"MOONS INCLUDED","7 / MIMAS TO IAPETUS / NOT A TOTAL COUNT"},
        {"SOURCE","NASA VTAD / SUPPLIED VISUALIZATION TEXTURES"},
        {"ELEVATION MODEL","NONE / CLOUD APPEARANCE IS NOT TERRAIN"},
        {"TIME CONTROLS","SPACE / PAUSE; S / SPEED; R / RESET"},
        {"CAMERA FRAME","V / FOLLOW OR WATCH REFERENCE ROTATION"}
    },"Cloud motion, filaments, hexagon, ring opacity and lighting are illustrative. The source is a visualization texture, not a calibrated Cassini global map. Readouts use model east longitude, without a claimed observation epoch. The Saturn local system uses physical sizes and mean distances for seven major moons, each with a NASA visualization explorer. Data and limits: docs/SATURN.md and docs/SATURN_MOONS.md.",
    "https://science.nasa.gov/saturn/facts/"};
    return article;
}
inline const solar::Article& locationGuide(int index) {
    static const solar::Article articles[]={
        {"NORTH POLAR HEXAGON",{"Saturn's northern polar hexagon is a six-sided pattern associated with a high-latitude atmospheric jet. Voyager and Cassini observed its unusual outline. This destination looks down at an illustrative hexagon and central vortex built in the shader; their colors, exact boundary and motion are not a reconstruction of a dated observation. Compare the polar view with the planet's broader cloud bands."},{},"MODELED POLAR JET / NO TERRAIN HEIGHT","https://science.nasa.gov/mission/cassini/science/saturn/hexagon-in-motion/"},
        {"EQUATORIAL CLOUDS",{"Saturn's equatorial atmosphere contains strong winds and layered haze. Cloud structures move at different rates with latitude; the planet does not have a single solid surface rotation visible in all its bands. This destination is an example viewing position in the equatorial region. The repeating motion here is a visual model, while the reference rotation controls the viewing frame."},{},"ILLUSTRATIVE CLOUD FLOW / EQUATOR","https://science.nasa.gov/saturn/facts/"},
        {"NORTHERN BANDS",{"The northern mid-latitudes show bands of pale cloud and atmospheric haze. Their color and contrast change with season and observing conditions. Use time speed and follow/watch rotation to compare the model's fine filaments and broad cloud structures. The NASA visualization texture supplies the base appearance, with procedural detail rather than a measured evolving wind field."},{},"CLOUD BANDS / NORTHERN HEMISPHERE","https://science.nasa.gov/saturn/facts/"},
        {"SOUTHERN BANDS",{"Saturn's southern cloud bands offer another view of its layered atmosphere and the equatorial ring plane. The rings can cast a broad shadow onto the opposite hemisphere depending on the light direction. This explorer uses camera-relative atlas lighting and analytic shadows, not a date-specific Sun direction. The southern hemisphere does not receive the shader's northern hexagon model."},{},"CLOUD BANDS + RING SHADOWS","https://science.nasa.gov/saturn/facts/"}
    };
    if(index<0||index>=4) throw std::runtime_error("Invalid Saturn region");
    return articles[index];
}
}
