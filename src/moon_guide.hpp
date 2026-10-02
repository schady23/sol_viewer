#pragma once
#include "guide.hpp"
#include "moon.hpp"
#include <stdexcept>
namespace moon {
inline const solar::Article& guide() {
    static const solar::Article article={"MOON",{
        "Earth's Moon preserves ancient impact basins, bright highlands and dark plains made by volcanic lava. Its synchronous rotation keeps approximately the same face toward Earth. The far side receives sunlight too; it is not a permanently dark hemisphere.",
        "This globe uses Lunar Reconnaissance Orbiter laser altimetry for relief and NASA's LROC-derived appearance map for maria and crater rays. Visit all six Apollo landing sites, compare crater floors with their rims, or cross to the far side. Landing markers locate history; this global grid does not resolve spacecraft or footprints."
    },{
        {"ORBIT / SIDEREAL","27.322 EARTH DAYS AROUND EARTH"},
        {"ROTATION / SIDEREAL","27.322 EARTH DAYS / SYNCHRONOUS"},
        {"SOLAR DAY / PHASE CYCLE","29.531 EARTH DAYS"},
        {"MEAN DISTANCE FROM EARTH","384400 KM / CENTRE TO CENTRE"},
        {"DAY TEMPERATURE","UP TO ABOUT +127 C"},
        {"NIGHT TEMPERATURE","DOWN TO ABOUT -173 C"},
        {"WEATHER WINDS","N/A / TENUOUS EXOSPHERE"},
        {"COMPOSITION","ROCKY CRUST AND MANTLE; SMALL IRON-RICH CORE"},
        {"SURFACE GRAVITY","1.62 M/S2 / ABOUT 1/6 EARTH"},
        {"MAGNETIC FIELD","NO PRESENT GLOBAL FIELD; LOCAL CRUSTAL FIELDS"},
        {"ORBIT INCLINATION","5.145 DEG TO THE ECLIPTIC"},
        {"AXIAL TILT","6.68 DEG TO THE LUNAR ORBIT NORMAL"},
        {"MOONS OF ITS OWN","0"},
        {"ELEVATION DATUM","1737.4 KM SPHERE / LOLA"},
        {"TERRAIN / DISPLAY","64 PPD SOURCE; 16 PPD DEFAULT MESH"}
    },"Temperatures depend on illumination and location; permanently shadowed polar terrain can be colder. The view uses a camera-relative study light, not present-day sunlight. Relief is 10X by default; elevation readouts always remain in real metres.",
    "https://science.nasa.gov/moon/facts/"};
    return article;
}
inline const solar::Article& locationGuide(int i) {
    static const solar::Article articles[]={
        {"APOLLO 11 / TRANQUILITY BASE",{
            "On 20 July 1969, Eagle carried Neil Armstrong and Buzz Aldrin to the Sea of Tranquility for the first human landing on another world. Michael Collins remained in lunar orbit. This marker follows the lunar module position measured in LROC images; the surrounding plain is measured terrain, but the spacecraft and footprints are much smaller than this grid."
        },{},"LANDING SITE / LROC 2016 COORDINATES","https://www.nasa.gov/mission/apollo-11/"},
        {"APOLLO 12 / OCEAN OF STORMS",{
            "Pete Conrad and Alan Bean landed Intrepid in November 1969, close enough to the earlier Surveyor 3 spacecraft to visit it on foot. Their precise landing demonstrated that a crew could reach a chosen scientific site. Explore the surrounding Ocean of Storms; the marker identifies the lunar module site, not a resolved model of the vehicle."
        },{},"LANDING SITE / LROC 2016 COORDINATES","https://www.nasa.gov/mission/apollo-12/"},
        {"APOLLO 14 / FRA MAURO",{
            "Alan Shepard and Edgar Mitchell explored the Fra Mauro region in February 1971. Their mission collected rocks from hilly terrain associated with material thrown out by the Imbrium impact. The landing marker lies near Cone crater; small traverse features are below the reliable detail of this global elevation mesh."
        },{},"LANDING SITE / LROC 2016 COORDINATES","https://www.nasa.gov/mission/apollo-14/"},
        {"APOLLO 15 / HADLEY-APENNINE",{
            "David Scott and James Irwin landed beside the Apennine mountains in July 1971. Apollo 15 introduced the lunar rover, extending exploration around Hadley Rille and the mountain front. This destination frames the contrast between broad mare plains and rugged uplands. The rover route is not reconstructed in this atlas."
        },{},"LANDING SITE / LROC 2016 COORDINATES","https://www.nasa.gov/mission/apollo-15/"},
        {"APOLLO 16 / DESCARTES",{
            "John Young and Charles Duke reached the Descartes highlands in April 1972. Their samples helped establish the importance of impacts in forming this terrain, challenging expectations of a mainly volcanic origin. Compare the rough, brighter highlands with the darker maria visited by other crews. The marker denotes Orion's landing position."
        },{},"LANDING SITE / LROC 2016 COORDINATES","https://www.nasa.gov/mission/apollo-16/"},
        {"APOLLO 17 / TAURUS-LITTROW",{
            "Eugene Cernan and geologist Harrison Schmitt explored Taurus-Littrow in December 1972, the final Apollo lunar landing. Mountains border a valley containing volcanic deposits, including the orange soil sampled at Shorty crater. This view shows the regional setting; the famous soil deposit and equipment cannot be resolved by this grid."
        },{},"LANDING SITE / LROC 2016 COORDINATES","https://www.nasa.gov/mission/apollo-17/"},
        {"TYCHO",{
            "Tycho is a prominent impact crater in the southern highlands. Its central mountain rose as compressed rock rebounded after the impact, and its bright rays spread across the near side. Compare the measured bowl and central uplift with the rays in the appearance map: brightness and elevation are different kinds of information."
        },{},"RAY CRATER / REGIONAL RELIEF","https://science.nasa.gov/resource/tycho-crater-on-the-moon-labeled/"},
        {"COPERNICUS",{
            "Copernicus is a large impact crater west of the near-side centre. Its central peaks expose material brought up from depth, while the surrounding bright ejecta record the impact's reach. Move between the image palette and elevation colors to inspect its raised rim and depressed floor without confusing bright material with high ground."
        },{},"COMPLEX CRATER / REGIONAL RELIEF","https://science.nasa.gov/resource/copernicus-central-peak-another-layered-target/"},
        {"ARISTARCHUS",{
            "Aristarchus is a bright crater on a geologically varied plateau. Its roughly 40-kilometre span is small compared with the great lunar basins, but its interior stands out in the appearance map. The surrounding region includes volcanic landforms; the elevation mesh shows regional shape, not individual boulders or thin rock layers."
        },{},"CRATER / ARISTARCHUS PLATEAU","https://science.nasa.gov/asset/hubble/aristarchus-plateau-on-the-moon/"},
        {"PLATO",{
            "Plato is a roughly 101-kilometre crater along the northern margin of Mare Imbrium. Its dark, comparatively smooth interior contrasts with the rugged rim. Compare the broad floor with nearby highlands. This marker is the Gazetteer feature centre; small craterlets on the floor are not dependable detail at the default mesh spacing."
        },{},"CRATER / GAZETTEER CENTRE","https://planetarynames.wr.usgs.gov/Feature/4757"},
        {"MARE IMBRIUM",{
            "Mare Imbrium is a vast volcanic plain occupying an ancient impact basin. Repeated lava flows resurfaced its interior, leaving dark terrain bordered by mountainous country. Its roughly 1,146-kilometre named extent makes it useful for stepping back from individual craters to basin-scale geology. The marker is a regional centre, not a landing site."
        },{},"VOLCANIC PLAIN / REGIONAL ANCHOR","https://www.usgs.gov/publications/volcanic-studies-part-e-eratosthenian-volcanism-mare-imbrium-source-youngest-lava"},
        {"REINER GAMMA",{
            "Reiner Gamma is a bright, winding lunar swirl amid the dark Oceanus Procellarum. It is associated with a local magnetic anomaly and has little corresponding relief. Compare the appearance map with elevation colors: a striking surface marking does not necessarily form a mountain or crater."
        },{},"LUNAR SWIRL / ALBEDO FEATURE","https://svs.gsfc.nasa.gov/4468"},
        {"ORIENTALE",{
            "The Orientale impact structure lies near the Moon's western limb. Concentric mountain rings surround the smaller dark Mare Orientale at its centre. This destination uses the named mare's centre as an anchor and pulls back to include the wider basin. Compare the rings and central depression in the measured lunar relief."
        },{},"MULTI-RING BASIN / MARE CENTRE ANCHOR","https://science.nasa.gov/moon/lunar-craters/the-explosive-history-of-orientale-basin/"},
        {"TSIOLKOVSKIY",{
            "Tsiolkovskiy is a large crater on the lunar far side. A central mountain rises above its dark floor, making a strong contrast with surrounding highlands. The uplift formed during the impact as the compressed target rock rebounded. Inspect that central relief and the crater's regional setting in the laser-altimeter grid."
        },{},"FARSIDE CRATER / REGIONAL RELIEF","https://science.nasa.gov/image-detail/tsiolkovsky/"},
        {"SOUTH POLE-AITKEN",{
            "South Pole-Aitken is an immense ancient basin on the far side. Its depressed interior extends across a large part of the southern hemisphere and contains many later craters. The marker at approximately 53 degrees south, 191 degrees east is a regional viewing anchor. Zoom out or use elevation colors to see the basin in context."
        },{},"ANCIENT BASIN / APPROXIMATE REGIONAL ANCHOR","https://www.hou.usra.edu/meetings/pgm2021/pdf/7032.pdf"},
        {"SHACKLETON",{
            "Shackleton is a roughly 21-kilometre crater beside the lunar south pole. Its interior includes permanently shadowed terrain of scientific interest for studies of cold traps and volatiles. The atlas uses a study light to reveal shape; illumination here is not a prediction of sunlight or a map of ice. The cylindrical grid also limits very fine polar detail."
        },{},"POLAR CRATER / STUDY LIGHT","https://science.nasa.gov/resource/shackleton-craters-illuminated-rim-shadowed-interior/"}
    };
    static_assert(std::size(articles)==std::size(landmarks));
    if(i<0||i>=landmarkCount) throw std::runtime_error("Invalid Moon location");
    return articles[i];
}
}
