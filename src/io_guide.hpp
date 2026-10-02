#pragma once
#include "guide.hpp"
#include <stdexcept>
namespace io {
inline const solar::Article& guide() {
    static const solar::Article article={"IO",{
        "Io is a rocky moon of Jupiter and the Solar System's most volcanically active world. Gravitational tides heat its interior, feeding lava flows and volcanic plumes. Sulfur compounds and sulfur-dioxide frost contribute to its varied surface colors, while dark patches mark many volcanic regions.",
        "This explorer styles an archival Voyager/Galileo mosaic prepared by USGS as a dense globe of flat-colored triangles. Visit Pele, Loki Patera, Prometheus and Tvashtar with the region list. L switches stepped atlas lighting on or off. The surface is a reference sphere: its colors are not measured heights or present-day volcanic activity."
    },{
        {"PARENT","JUPITER / INNERMOST GALILEAN MOON"},
        {"MEAN RADIUS","1821.49 KM"},
        {"MEAN ORBITAL DISTANCE","421800 KM FROM JUPITER'S CENTRE"},
        {"ROTATION","SYNCHRONOUS / SAME FACE TOWARD JUPITER"},
        {"HEAT SOURCE","TIDAL DEFORMATION"},
        {"ATMOSPHERE","THIN / MAINLY SULFUR DIOXIDE"},
        {"SURFACE MAP","USGS / VOYAGER + GALILEO COLOR MERGE"},
        {"DISPLAY GRID","4096 X 2048 / ABOUT 2.8 KM AT EQUATOR"},
        {"RENDERING","1.31 MILLION TRIANGLES / 5-BIT COLOR"},
        {"ELEVATION MODEL","NONE / MEAN-RADIUS DISPLAY SPHERE"},
        {"JUPITER SYSTEM","TRUE BODY SIZES / STATIC ORBITAL PLACEMENTS"}
    },"Images combine different dates and resolutions; surface changes and seams remain. Polar color is interpolated in the source. Region coordinates are approximate viewing anchors. Data, coordinate conventions and references: docs/IO.md.",
    "https://science.nasa.gov/jupiter/jupiter-moons/io/facts/"};
    return article;
}
inline const solar::Article& locationGuide(int index) {
    static const solar::Article articles[]={
        {"PELE",{"Pele is surrounded by a conspicuous reddish deposit left by volcanic plumes. Sulfur-bearing material ejected from the vent settles across the surrounding terrain, creating a broad ring. The deposit records activity when the source images were taken; its appearance can change as new eruptions cover older material."},{},"PLUME DEPOSITS / ARCHIVAL VIEW","https://science.nasa.gov/photojournal/pele-plume-deposit-on-io/"},
        {"LOKI PATERA",{"Loki Patera is the large dark volcanic depression in this region. Spacecraft and telescopic observations associate it with a vast lava lake, whose changing thermal output helps scientists study volcanic processes. The dark surface in this mosaic is reflected-light imagery, not a temperature map; it does not show today's hot areas."},{},"VOLCANIC DEPRESSION / REFLECTED LIGHT","https://www.usgs.gov/observatories/hvo/news/volcano-watch-active-lava-lakes-are-found-beyond-earth"},
        {"PROMETHEUS",{"Prometheus contains dark lava flows bordered by brighter plains. Galileo's close views revealed flow textures and streaks associated with plume deposits. Fresh or warm lava can remain dark before sulfur-dioxide frost accumulates on it. This global mosaic provides regional context; the camera does not resolve individual moving flows."},{},"LAVA FLOWS + PLUME DEPOSITS","https://science.nasa.gov/photojournal/galileo-takes-a-close-up-look-at-prometheus/"},
        {"TVASHTAR PATERAE",{"Tvashtar is a group of volcanic depressions in Io's high northern latitudes. New Horizons photographed a towering plume here during its 2007 Jupiter encounter. That later observation helps explain the region's activity, but it is not part of the older Voyager/Galileo mosaic shown here. No plume animation is added to this archival surface."},{},"NORTHERN VOLCANIC REGION / ARCHIVAL VIEW","https://science.nasa.gov/photojournal/tvashtar-in-motion/"}
    };
    if(index<0||index>=4) throw std::runtime_error("Invalid Io guide location");
    return articles[index];
}
}
