#pragma once
#include "guide.hpp"
#include <stdexcept>
namespace callisto {
inline const solar::Article& guide() {
    static const solar::Article article={"CALLISTO",{
        "Callisto is the outermost Galilean moon and Jupiter's second-largest satellite. Its dark, icy surface preserves a long impact record: bright craters, overlapping scars and enormous ringed basins. Much of this ancient landscape has escaped the extensive resurfacing seen on some other moons.",
        "Explore Valhalla, Asgard, Adlinda and Lofn in this Voyager/Galileo atlas. The monochrome USGS mosaic is styled with a stone-and-ice tint, flat triangular faces and stepped lighting. The palette is artistic; brightness is not measured elevation or an ice-composition map."
    },{
        {"PARENT","JUPITER / OUTERMOST GALILEAN MOON"},
        {"MEAN RADIUS","2410.30 KM"},
        {"MEAN ORBITAL DISTANCE","1882700 KM FROM JUPITER'S CENTRE"},
        {"ORBIT / ROTATION","ABOUT 16.69 EARTH DAYS / SYNCHRONOUS"},
        {"SURFACE","ANCIENT, HEAVILY CRATERED ICE AND ROCK"},
        {"INTERIOR","POSSIBLE DEEP OCEAN / NOT CONFIRMED"},
        {"ATMOSPHERE","VERY THIN EXOSPHERE"},
        {"SURFACE MAP","USGS / VOYAGER + GALILEO MONOCHROME"},
        {"DISPLAY GRID","4096 X 2048 / ABOUT 3.7 KM AT EQUATOR"},
        {"RENDERING","1.31 MILLION TRIANGLES / 5-BIT COLOR"},
        {"ELEVATION MODEL","NONE / MEAN-RADIUS DISPLAY SPHERE"},
        {"LIGHTING","L / ATLAS LIGHTING OR UNLIT FACETS"}
    },"Image resolution and illumination vary across the mosaic. Flat dark polar patches mark missing source coverage. The tint adds no observed color or terrain heights. Regional anchors use east longitude; data and references: docs/CALLISTO.md.",
    "https://science.nasa.gov/jupiter/jupiter-moons/callisto/facts/"};
    return article;
}
inline const solar::Article& locationGuide(int index) {
    static const solar::Article articles[]={
        {"VALHALLA",{"Valhalla's bright central plains are surrounded by a vast pattern of concentric rings. This impact structure records deformation of Callisto's icy crust after a large collision. Many outer rings are troughs rather than raised ridges. The atlas preserves their image contrast on a spherical display surface; it does not reconstruct their depth."},{},"MULTI-RING IMPACT STRUCTURE","https://science.nasa.gov/photojournal/the-valhalla-multi-ring-structure-on-callisto/"},
        {"ASGARD",{"Asgard has a bright interior surrounded by incomplete rings. Galileo's closer views revealed degraded ridges and dark material in the outer troughs. Look across the surrounding cratered plains to compare the basin with later impacts. This global mosaic mixes image resolutions, so fine features are not equally resolved throughout the region."},{},"BRIGHT PLAINS + DISCONTINUOUS RINGS","https://science.nasa.gov/photojournal/asgard-multi-ring-structure-on-callisto/"},
        {"ADLINDA",{"Adlinda is a large ringed impact feature in Callisto's southern hemisphere, northwest of Lofn. The surrounding landscape records overlapping events: the older ringed structure, nearby crater terrain and material spread by later impacts. Compare this wider regional view with the Lofn destination to see how the bright impact deposits sit within the ancient plains."},{},"SOUTHERN RINGED IMPACT TERRAIN","https://ntrs.nasa.gov/citations/20030018897"},
        {"LOFN",{"Lofn is a bright impact structure in the southern cratered plains. Galileo images show an asymmetric arrangement of its rim and ejecta, interpreted as evidence for an oblique impact. Its shallow form has prompted several formation explanations. Here the bright deposits come from the archival images; the display tint does not measure their composition."},{},"BRIGHT CRATER + IMPACT DEPOSITS","https://ntrs.nasa.gov/citations/20030018897"}
    };
    if(index<0||index>=4) throw std::runtime_error("Invalid Callisto guide location");
    return articles[index];
}
}
