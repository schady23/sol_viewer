#include "guide.hpp"
#include "belts.hpp"
#include "moon_guide.hpp"
#include "earth_guide.hpp"
#include "io_guide.hpp"
#include "callisto_guide.hpp"
#include "satellites.hpp"
#include "saturn_guide.hpp"
#include <sstream>
#include <stdexcept>
namespace solar {
bool hasGuide(BodyId b) { return belts::region(b)||b==BodyId::Earth||b==BodyId::Sun||b==BodyId::Mars||b==BodyId::Mercury||b==BodyId::Jupiter||b==BodyId::Moon||b==BodyId::Io||b==BodyId::Callisto||b==BodyId::Saturn||satellites::supports(b); }
int locationCount(BodyId b) { return b==BodyId::Sun||belts::region(b)?0:b==BodyId::Moon?moon::landmarkCount:hasGuide(b)?4:0; }
const Article& planetArticle(BodyId b) {
    static const Article star={"SUN",{
        "The Sun is a self-luminous star: its visible photosphere is plasma, not a solid surface. A radius of 695,700 km makes its diameter about 109 times Earth's.",
        "Granulation suggests the Sun's convecting plasma. Sunspots are cooler, darker parts of magnetically active regions. Here, irregular groups grow, drift and fade, with dark cores, striated borders and neighboring pores. Bright flares release energy; coronal mass ejections expel clouds of solar plasma and magnetic field. They are different phenomena, although they can occur together.",
        "This explorer combines a glowing photosphere, bright active regions, prominences and an expanding corona in an authored emission palette. The repeating activity lasts 90 seconds; neither its colors nor its rotation or eruption timing represent a live observation or prediction."
    },{{"MEAN RADIUS","695,700 KM"},{"DIAMETER","1,391,400 KM"},{"EARTH COMPARISON","ABOUT 109 EARTH DIAMETERS"},{"SURFACE","PHOTOSPHERE / PLASMA"},{"SUNSPOTS","COOLER MAGNETIC ACTIVE REGIONS"},{"FLARES","ENERGY RELEASE"},{"CORONAL MASS EJECTIONS","EJECTED PLASMA AND MAGNETIC FIELD"}},
    "Corona visibility and activity are emphasized for explanation. The 90-second loop is not the Sun's physical rotation period.","https://science.nasa.gov/sun/facts/"};
    static const Article mainBelt={"MAIN ASTEROID BELT",{
        "The main asteroid belt lies between Mars and Jupiter. Its principal concentration spans roughly 2.1 to 3.3 astronomical units from the Sun. Ceres is its largest object and only recognized dwarf planet. Most of this enormous volume is empty: NASA estimates that kilometre-scale asteroids are typically separated by millions of kilometres.",
        "This atlas shows an idealized selection of inclined, eccentric orbits. Dots are enlarged location markers, not visible boulders or a count of real objects. Jupiter's resonances create Kirkwood gaps in the distribution of orbital semimajor axes; eccentric orbits cross radial distances within those gaps, so they are not empty circular trenches. Sizes of the Sun and Ceres use the same length conversion as orbital distances. Select Ceres to approach it automatically; Enter opens its NASA visualization."
    },{{"MAIN REGION","ABOUT 2.1-3.3 AU"},{"REFERENCE DISTANCE","1 AU = 149,597,870.7 KM"},{"DWARF PLANET","CERES"},{"KIRKWOOD GAPS","3:1 / 5:2 / 7:3 / 2:1 WITH JUPITER"}},"Static, idealized samples explain structure. They are not a measured census, current ephemeris or naked-eye view. See docs/BELTS.md for references and assumptions.","https://science.nasa.gov/mission/dawn/faq/"};
    static const Article kuiper={"KUIPER BELT AND SCATTERED DISK",{
        "Beyond Neptune, the main Kuiper belt stretches from about 30 to 50 AU. It contains icy bodies left from planet formation. Cold classical objects occupy relatively low-inclination orbits, while other objects follow more inclined or resonant paths. The sparse scattered population extends much farther out; Eris belongs to that population, not to a dense ring at 30-50 AU.",
        "Dots are enlarged sample locators. The visualization separates illustrative cold, hot, 3:2 resonant and scattered orbital families; their sampled proportions are authored, not measured population fractions. Dwarf planet paths use rounded JPL eccentricities and inclinations with authored orbital phases. Pluto, Haumea, Makemake and Eris can be selected to approach automatically, then opened with Enter. Eris's orbit reaches roughly 98 AU. Reference shapes for unvisited worlds do not claim known surface geography."
    },{{"MAIN REGION","ABOUT 30-50 AU"},{"COLD CLASSICAL SAMPLE","42-47 AU / LOW INCLINATION"},{"DWARF PLANETS","PLUTO / HAUMEA / MAKEMAKE / ERIS"},{"ERIS","SCATTERED POPULATION / ~38-98 AU"}},"Static orbital atlas; enhanced markers are not physical object diameters. No live ephemeris or complete trans-Neptunian census is implied. See docs/BELTS.md.","https://science.nasa.gov/solar-system/kuiper-belt/facts/"};
    if(b==BodyId::AsteroidBelt) return mainBelt;
    if(b==BodyId::KuiperBelt) return kuiper;
    if(b==BodyId::Sun) return star;
    if(b==BodyId::Earth) return earth::guide();
    if(b==BodyId::Saturn) return saturn::guide();
    if(satellites::supports(b)) return satellites::guide(b);
    if(b==BodyId::Moon) return moon::guide();
    if(b==BodyId::Callisto) return callisto::guide();
    if(b==BodyId::Io) return io::guide();
    static const Article mercury={"MERCURY",{
        "Mercury is the smallest planet and the closest to the Sun. Its battered crust preserves impact basins, lava-covered plains and cliffs formed as its interior cooled and contracted. A large iron-rich core lies beneath a rocky mantle and crust.",
        "Sunrise is rare here: Mercury turns three times for every two trips around the Sun. With only a tenuous exosphere to redistribute heat, sunlit ground becomes intensely hot while the long night turns bitterly cold. Permanently shadowed polar craters can shelter water ice."
    },{
        {"YEAR","87.97 EARTH DAYS"},{"ROTATION / SIDEREAL","58.65 EARTH DAYS"},{"SOLAR DAY / DAY-NIGHT CYCLE","175.94 EARTH DAYS"},
        {"DAY TEMPERATURE","UP TO +430 C"},{"NIGHT TEMPERATURE","DOWN TO -180 C"},
        {"WEATHER WINDS","N/A / NO BULK WEATHER ATMOSPHERE"},
        {"INTERIOR","IRON-RICH CORE; ROCKY MANTLE AND CRUST"},
        {"EXOSPHERE","OXYGEN, SODIUM, HYDROGEN, HELIUM, POTASSIUM"},
        {"MEAN SURFACE GRAVITY","3.70 M/S2 / ABOUT 0.38 EARTH"},
        {"MAGNETIC FIELD","WEAK GLOBAL FIELD / ABOUT 1% OF EARTH"},
        {"ORBIT INCLINATION","7.005 DEG TO THE ECLIPTIC / J2000"},
        {"AXIAL TILT","0.034 DEG FROM THE ORBIT NORMAL"},
        {"MOONS","0"}
    },"Temperatures are approximate extremes, not forecasts. Solar wind is a stream of charged particles from the Sun, not a weather wind. The ecliptic is the reference plane of Earth's orbit.",
    "https://nssdc.gsfc.nasa.gov/planetary/factsheet/mercuryfact.html"};
    static const Article mars={"MARS",{
        "Mars is a cold desert planet with a thin, mostly carbon-dioxide atmosphere. Rust-bearing dust gives it a reddish appearance. Volcanoes, impact basins and vast canyons record a long geological history, while ancient valleys and lake deposits show that liquid water once shaped parts of its surface.",
        "Its day is close to ours, but a year lasts almost twice as long. An inclined rotation axis produces seasons. Polar ice, moving dust and changing winds make Mars an active world today; its weak atmospheric insulation allows large temperature swings. Phobos and Deimos are its two small moons."
    },{
        {"YEAR","686.98 EARTH DAYS"},{"ROTATION / SIDEREAL","24 H 37 MIN"},{"SOLAR DAY / SOL","24 H 39 MIN 35 S"},
        {"DAY / WARM EXAMPLE","UP TO +27 C / EQUATOR NEAR PERIHELION"},
        {"NIGHT / COLD EXAMPLE","DOWN TO -128 C / POLAR NIGHT"},
        {"DAY-NIGHT RANGE / VIKING 1","-89 TO -31 C / ONE LANDING SITE"},
        {"WINDS / VIKING SITES","2-7 M/S SUMMER; 5-10 M/S AUTUMN"},
        {"DUST-STORM WINDS / VIKING","17-30 M/S"},
        {"INTERIOR","IRON-RICH CORE; ROCKY MANTLE AND CRUST"},
        {"ATMOSPHERE","ABOUT 95% CO2; NITROGEN AND ARGON"},
        {"MEAN SURFACE GRAVITY","3.73 M/S2 / ABOUT 0.38 EARTH"},
        {"MAGNETIC FIELD","NO GLOBAL DYNAMO; LOCAL CRUSTAL FIELDS"},
        {"ORBIT INCLINATION","1.851 DEG TO THE ECLIPTIC / J2000"},
        {"AXIAL TILT","25.19 DEG FROM THE ORBIT NORMAL"},
        {"MOONS","2 / PHOBOS AND DEIMOS"}
    },"Day/night examples vary with latitude, season and surface conditions. Viking wind ranges are local observations, not a global average. These reference facts do not describe live weather in the simulation.",
    "https://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html"};
    static const Article jupiter={"JUPITER",{
        "Jupiter is a giant world dominated by hydrogen and helium. It has no solid ground at the visible cloud tops. Rapid rotation flattens its poles, while powerful east-west jets divide the atmosphere into bright zones and darker belts.",
        "The Great Red Spot is a long-lived storm in the southern hemisphere. This explorer combines a January 2015 Hubble cloud map with fine folded ribbons and blue-gray cloud gaps inspired by Juno images. The atmosphere moves through a seamless artistic loop; its motion and added small details are not observed or predicted weather."
    },{
        {"YEAR","4332.59 EARTH DAYS / ABOUT 11.86 YEARS"},
        {"ROTATION / SYSTEM III","9 H 55 MIN 30 S / CLOUDS ALSO DRIFT"},
        {"DAY-NIGHT TEMPERATURE","NO SINGLE SURFACE DAY/NIGHT RANGE"},
        {"ATMOSPHERE / 1 BAR","ABOUT -108 C"},{"ATMOSPHERE / 0.1 BAR","ABOUT -161 C"},
        {"WINDS","LATITUDE-DEPENDENT JETS; SOME ABOUT 150 M/S"},
        {"COMPOSITION","ABOUT 90% HYDROGEN / 10% HELIUM BY NUMBER"},
        {"MEAN GRAVITY / 1 BAR","25.92 M/S2 / REFERENCE ATMOSPHERIC LEVEL"},
        {"MAGNETIC FIELD","POWERFUL GLOBAL DYNAMO IN METALLIC HYDROGEN"},
        {"ORBIT INCLINATION","1.305 DEG TO THE ECLIPTIC / J2000"},
        {"AXIAL TILT","3.13 DEG FROM THE ORBIT NORMAL"},
        {"MOONS / NASA COUNT","115 / NASA PAGE CHECKED 27 SEP 2026"},
        {"EQUATOR / POLAR RADIUS","71492 / 66854 KM AT THE 1-BAR LEVEL"},
        {"CLOUD MAP","HUBBLE OPAL / 19 JAN 2015 / ROTATION 1"},
        {"DETAIL INSPIRATION","JUNO / PIA21974 AND PIA26595"}
    },"The procedural loop lasts 10 reference rotations (99.25 Earth hours). Polar appearance is illustrative. Cloud coordinates are planetographic, with east-positive System III longitude; no terrain elevation is claimed. Source details and fact-sheet links: docs/JUPITER.md.",
    "https://science.nasa.gov/jupiter/jupiter-facts/"};
    if(b==BodyId::Jupiter) return jupiter;
    if(b==BodyId::Mercury) return mercury;
    if(b==BodyId::Mars) return mars;
    throw std::runtime_error("No field guide for this body");
}
const Article& locationArticle(BodyId body,int i) {
    if(body==BodyId::Earth) return earth::locationGuide(i);
    if(body==BodyId::Saturn) return saturn::locationGuide(i);
    if(satellites::supports(body)) return satellites::locationGuide(body,i);
    if(body==BodyId::Moon) return moon::locationGuide(i);
    if(body==BodyId::Callisto) return callisto::locationGuide(i);
    if(body==BodyId::Io) return io::locationGuide(i);
    static const Article mars[]={
        {"OLYMPUS MONS",{"Olympus Mons is a giant shield volcano built by repeated lava flows. Its broad, gently sloping flanks surround a complex summit caldera, where the roof above a magma reservoir collapsed. A steep scarp borders much of the volcano. In the globe, compare the wide volcanic rise with the surrounding plains; its great footprint matters as much as its summit height."},{},"SHIELD VOLCANO / MARS","https://www.jpl.nasa.gov/images/pia00300-olympus-mons/"},
        {"VALLES MARINERIS",{"Valles Marineris is a vast network of troughs stretching across the Martian equatorial region. Crustal stretching helped open the system, and landslides, collapse and erosion later changed its walls and floor. Layered deposits preserve further chapters of its history. Look for the connected depressions east of the Tharsis volcanic region, rather than a single river-cut valley."},{},"CANYON SYSTEM / MARS","https://ntrs.nasa.gov/search.jsp?R=19780027498"},
        {"HELLAS PLANITIA",{"Hellas is a huge impact basin in the southern highlands, roughly 2,300 kilometres across. The ancient collision excavated a deep depression that was later modified by sediment, erosion and other geological processes. Its low floor contrasts strongly with the surrounding cratered terrain. Negative MOLA readings here mean elevation below the Martian reference areoid, not below an ocean."},{},"IMPACT BASIN / MARS","https://www.esa.int/Science_Exploration/Space_Science/Mars_Express/Frost-covered_chaos_on_Mars"},
        {"ELYSIUM PLANITIA",{"Elysium Planitia is a broad region of volcanic plains near the Martian equator. Lava flows helped build its relatively low, smooth landscape. NASA chose part of western Elysium for InSight because a flat, relatively rock-free surface offered a safe place to study the planet's interior. This atlas marker identifies the wider plains, not the exact InSight landing site."},{},"VOLCANIC PLAINS / MARS","https://www.jpl.nasa.gov/news/press_kits/insight/launch/mission/landing-site/"}
    };
    static const Article mercury[]={
        {"CALORIS PLANITIA",{"Caloris is an enormous impact basin, about 1,500 kilometres across, encircled by mountainous terrain. Lava later flooded much of its interior, creating smoother plains inside the older impact structure. Younger craters cut through these deposits and expose material beneath them. The broad basin and its rough rim make a useful contrast to Mercury's densely cratered surroundings."},{},"IMPACT BASIN / MERCURY","https://science.nasa.gov/resource/mercurys-caloris-basin/"},
        {"REMBRANDT",{"Rembrandt is a large impact basin in Mercury's southern hemisphere, about 716 kilometres wide. Its interior preserves both impact-related terrain and smoother plains, together with ridges and troughs that record later deformation. This mixture lets scientists study how impacts, volcanism and crustal stresses reshaped the same region. Compare the basin floor with its rim and the older terrain outside it."},{},"IMPACT BASIN / MERCURY","https://science.nasa.gov/photojournal/a-closer-look-into-rembrandt-basin/"},
        {"RACHMANINOFF",{"Rachmaninoff is a double-ring impact basin whose inner mountain ring surrounds smoother interior plains. Volcanic deposits are thought to have resurfaced parts of the basin after the impact. The MESSENGER global elevation model places Mercury's lowest mapped terrain on its floor. The atlas marker is a basin-centre anchor; its readout is not necessarily the deepest sample in the region."},{},"PEAK-RING BASIN / MERCURY","https://www.nasa.gov/missions/first-global-topographic-model-of-mercury/"},
        {"BEETHOVEN",{"Beethoven is a broad impact basin in Mercury's southern hemisphere. MESSENGER mapping of the surrounding region distinguishes volcanic plains, rock melted by impacts, and smooth deposits whose origin remains uncertain. That combination records more than the original collision: later resurfacing also changed the landscape. In the globe, look for the wide, subdued depression among neighbouring craters."},{},"IMPACT BASIN / MERCURY","https://science.nasa.gov/photojournal/mapping-beethoven/"}
    };
    static const Article jupiter[]={
        {"GREAT RED SPOT",{"The Great Red Spot is an anticyclonic storm south of Jupiter's equator. Clouds circulate counterclockwise around it. Select this region to keep the model storm in view while increasing time speed. Its centre is anchored to the 2015 image here; real storms change shape, size and longitude."},{},"ARCHIVAL ANCHOR / MODEL CIRCULATION","https://science.nasa.gov/photojournal/red-spot-movie/"},
        {"EQUATORIAL ZONE",{"The bright band around Jupiter's equator is the Equatorial Zone. It lies between darker belts to the north and south. Clouds at different latitudes move at different speeds, stretching patterns along the planet. This selection is an example viewing position in a broad atmospheric region, not a fixed surface landmark."},{},"BRIGHT CLOUD BAND / NO SOLID SURFACE","https://science.nasa.gov/jupiter/jupiter-facts/"},
        {"NORTH EQUATORIAL BELT",{"North of the bright equatorial band, the North Equatorial Belt contains darker cloud patterns bounded by strong jets. Use Follow Rotation and accelerated time to compare motion across its edges. The region moves and changes in nature; this atlas uses a periodic procedural model to suggest atmospheric motion."},{},"DARK CLOUD BELT / STYLIZED LOOP","https://science.nasa.gov/photojournal/red-spot-movie/"},
        {"SOUTH EQUATORIAL BELT",{"The South Equatorial Belt is the darker band south of the equator, close to the Great Red Spot. Its cloud patterns interact with neighbouring flows and storms. In this model, opposing cloud patterns and a looping vortex suggest motion around the Red Spot. This is a visual explanation of motion, not a forecast."},{},"DARK CLOUD BELT / MODEL VORTEX NEARBY","https://science.nasa.gov/resource/great-red-spot-rotation-animation/"}
    };
    if(i<0||i>=locationCount(body)||!hasGuide(body)) throw std::runtime_error("Invalid field-guide location");
    return body==BodyId::Mars?mars[i]:body==BodyId::Jupiter?jupiter[i]:mercury[i];
}
std::vector<std::string> wrapText(const std::string& text,int columns) {
    columns=std::max(1,columns); std::istringstream input(text);
    std::vector<std::string> lines; std::string line,word;
    while(input>>word) {
        if(!line.empty()&&int(line.size()+1+word.size())>columns) { lines.push_back(line); line.clear(); }
        while(int(word.size())>columns) { lines.push_back(word.substr(0,columns)); word.erase(0,columns); }
        if(!word.empty()) { if(!line.empty()) line+=' '; line+=word; }
    }
    if(!line.empty()) lines.push_back(line);
    return lines;
}
std::vector<GuideLine> styledArticleLines(const Article& a,int columns,float bodyScale) {
    std::vector<GuideLine> result;
    auto append=[&](const std::string& t,GuideStyle style,float scale) {
        for(auto& line:wrapText(t,std::max(1,int(columns*bodyScale/scale))))
            result.push_back({line,style,scale});
    };
    auto gap=[&] { result.push_back({"",GuideStyle::Space,1}); };
    for(auto paragraph:a.paragraphs) { append(paragraph,GuideStyle::Body,bodyScale); gap(); }
    if(!a.facts.empty()) { append(a.factHeading,GuideStyle::Heading,2); gap(); }
    for(auto fact:a.facts) {
        append(fact.label,GuideStyle::Label,1);
        append(fact.value,GuideStyle::Value,bodyScale); gap();
    }
    if(a.note&&*a.note) { append("CONTEXT / SOURCES",GuideStyle::Label,1); append(a.note,GuideStyle::Note,1); }
    return result;
}
std::vector<std::string> articleLines(const Article& a,int columns) {
    // Plain-text export keeps its original column contract.
    std::vector<std::string> result;
    for(const auto& line:styledArticleLines(a,columns,1)) result.push_back(line.text);
    return result;
}
void Guide::sync(BodyId current,int selected) {
    if(!hasGuide(current)) current=BodyId::None;
    if(body!=current) { body=current; scroll=0; location=-1; locationOpen=false; if(body==BodyId::None) open=false; }
    if(!hasGuide(body)||selected<0||selected>=locationCount(body)) selected=-1;
    if(selected!=location) { location=selected; locationScroll=0; locationOpen=selected>=0; }
}
void Guide::toggle() { if(hasGuide(body)) open=!open; }
const Article& Guide::article() const { return planetArticle(body); }
int Guide::maximumScroll(const mars::ViewLayout& l) const {
    if(!hasGuide(body)) return 0;
    return std::max(0,int(styledArticleLines(article(),l.guideColumns()).size())-l.guideRows());
}
void Guide::move(int rows,const mars::ViewLayout& l) { scroll=std::clamp(scroll+rows,0,maximumScroll(l)); }
int Guide::maximumLocationScroll(const mars::ViewLayout& l) const {
    if(!hasGuide(body)||location<0) return 0;
    return std::max(0,int(styledArticleLines(locationArticle(body,location),l.locationColumns(),1.25f).size())-l.locationRows());
}
void Guide::moveLocation(int rows,const mars::ViewLayout& l) { locationScroll=std::clamp(locationScroll+rows,0,maximumLocationScroll(l)); }
}
