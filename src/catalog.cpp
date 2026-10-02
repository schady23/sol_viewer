#include "catalog.hpp"
#include "ice_giants.hpp"
#include "belts.hpp"
#include <cctype>
#include <stdexcept>
namespace solar {
const std::array<Body,bodyCount>& bodies() {
    static const std::array<Body,bodyCount> catalog={{
        {BodyId::Sun,BodyId::None,"SUN",BodyKind::Star,{.98f,.76f,.30f},.095f,3,""},
        {BodyId::Mercury,BodyId::Sun,"MERCURY",BodyKind::Planet,{.56f,.53f,.48f},.021f,0,"mdis64"},
        {BodyId::Venus,BodyId::Sun,"VENUS",BodyKind::Planet,{.86f,.67f,.40f},.030f,0,"magellan-v02"},
        {BodyId::Earth,BodyId::Sun,"EARTH",BodyKind::Planet,{.32f,.60f,.69f},.032f,0,"blue-marble-2004+etopo2022"},
        {BodyId::Mars,BodyId::Sun,"MARS",BodyKind::Planet,{.81f,.39f,.25f},.027f,0,"mola16"},
        {BodyId::Jupiter,BodyId::Sun,"JUPITER",BodyKind::Planet,{.82f,.62f,.45f},.068f,1,""},
        {BodyId::Saturn,BodyId::Sun,"SATURN",BodyKind::Planet,{.88f,.77f,.53f},.056f,2,"vtad-saturn"},
        {BodyId::Uranus,BodyId::Sun,"URANUS",BodyKind::Planet,{.47f,.76f,.75f},.043f,0,"nasa-uranus-model"},
        {BodyId::Neptune,BodyId::Sun,"NEPTUNE",BodyKind::Planet,{.33f,.44f,.76f},.041f,0,"nasa-neptune-model"},
        {BodyId::Moon,BodyId::Earth,"MOON",BodyKind::Moon,{.70f,.69f,.62f},.035f,0,"lola64"},
        {BodyId::Phobos,BodyId::Mars,"PHOBOS",BodyKind::Moon,{.58f,.44f,.34f},.032f,0,"nasa-phobos-model"},
        {BodyId::Deimos,BodyId::Mars,"DEIMOS",BodyKind::Moon,{.73f,.62f,.46f},.025f,0,"nasa-deimos-model"},
        {BodyId::Io,BodyId::Jupiter,"IO",BodyKind::Moon,{.88f,.73f,.35f},.031f,0,"voyager-galileo-color"},
        {BodyId::Europa,BodyId::Jupiter,"EUROPA",BodyKind::Moon,{.85f,.80f,.65f},.029f,0,"voyager-galileo-monochrome"},
        {BodyId::Ganymede,BodyId::Jupiter,"GANYMEDE",BodyKind::Moon,{.58f,.54f,.47f},.042f,0,"voyager-galileo-monochrome"},
        {BodyId::Callisto,BodyId::Jupiter,"CALLISTO",BodyKind::Moon,{.40f,.43f,.42f},.039f,0,"voyager-galileo-monochrome"},
        {BodyId::Titan,BodyId::Saturn,"TITAN",BodyKind::Moon,{.81f,.56f,.24f},.042f,0,"nasa-titan-model"},
        {BodyId::Titania,BodyId::Uranus,"TITANIA",BodyKind::Moon,{.68f,.66f,.63f},.032f,0,"nasa-titania-model"},
        {BodyId::Triton,BodyId::Neptune,"TRITON",BodyKind::Moon,{.72f,.76f,.74f},.035f,0,"nasa-triton-model"},
        {BodyId::Mimas,BodyId::Saturn,"MIMAS",BodyKind::Moon,{.78f,.77f,.72f},.025f,0,"nasa-mimas-model"},
        {BodyId::Enceladus,BodyId::Saturn,"ENCELADUS",BodyKind::Moon,{.78f,.77f,.72f},.025f,0,"nasa-enceladus-model"},
        {BodyId::Tethys,BodyId::Saturn,"TETHYS",BodyKind::Moon,{.78f,.77f,.72f},.025f,0,"nasa-tethys-model"},
        {BodyId::Dione,BodyId::Saturn,"DIONE",BodyKind::Moon,{.78f,.77f,.72f},.025f,0,"nasa-dione-model"},
        {BodyId::Rhea,BodyId::Saturn,"RHEA",BodyKind::Moon,{.78f,.77f,.72f},.025f,0,"nasa-rhea-model"},
        {BodyId::Iapetus,BodyId::Saturn,"IAPETUS",BodyKind::Moon,{.78f,.77f,.72f},.025f,0,"nasa-iapetus-model"},
        {BodyId::Miranda,BodyId::Uranus,"MIRANDA",BodyKind::Moon,{.58f,.57f,.54f},.025f,0,"nasa-miranda-model"},
        {BodyId::Ariel,BodyId::Uranus,"ARIEL",BodyKind::Moon,{.58f,.57f,.54f},.025f,0,"nasa-ariel-model"},
        {BodyId::Umbriel,BodyId::Uranus,"UMBRIEL",BodyKind::Moon,{.58f,.57f,.54f},.025f,0,"nasa-umbriel-model"},
        {BodyId::Oberon,BodyId::Uranus,"OBERON",BodyKind::Moon,{.58f,.57f,.54f},.025f,0,"nasa-oberon-model"},
        {BodyId::Proteus,BodyId::Neptune,"PROTEUS",BodyKind::Moon,{.58f,.57f,.54f},.025f,0,"reference-proteus"},
        {BodyId::AsteroidBelt,BodyId::Sun,"ASTEROID BELT",BodyKind::Region,{.65f,.57f,.44f},.01f,0,"orbital-distribution"},
        {BodyId::KuiperBelt,BodyId::Sun,"KUIPER BELT",BodyKind::Region,{.48f,.66f,.77f},.01f,0,"orbital-distribution"},
        {BodyId::Ceres,BodyId::AsteroidBelt,"CERES",BodyKind::DwarfPlanet,{.55f,.53f,.49f},.01f,0,"nasa-ceres-model"},
        {BodyId::Pluto,BodyId::KuiperBelt,"PLUTO",BodyKind::DwarfPlanet,{.77f,.66f,.54f},.01f,0,"nasa-pluto-model"},
        {BodyId::Haumea,BodyId::KuiperBelt,"HAUMEA",BodyKind::DwarfPlanet,{.78f,.79f,.78f},.01f,0,"reference-haumea"},
        {BodyId::Makemake,BodyId::KuiperBelt,"MAKEMAKE",BodyKind::DwarfPlanet,{.71f,.49f,.35f},.01f,0,"reference-makemake"},
        {BodyId::Eris,BodyId::KuiperBelt,"ERIS",BodyKind::DwarfPlanet,{.83f,.84f,.83f},.01f,0,"reference-eris"}
    }};
    return catalog;
}
const Body& body(BodyId id) {
    if(int(id)<0||int(id)>=bodyCount) throw std::runtime_error("Invalid body ID");
    return bodies()[int(id)];
}
BodyId bodyNamed(std::string name) {
    for(auto& c:name) c=char(std::toupper(static_cast<unsigned char>(c)));
    for(const auto& b:bodies()) if(name==b.name) return b.id;
    throw std::runtime_error("Unknown body: "+name);
}
std::vector<BodyId> children(BodyId parent) {
    if(ice::planet(parent)) { std::vector<BodyId> ids; for(const auto& m:outerMoons(parent)) ids.push_back(m.id); return ids; }
    if(parent==BodyId::Saturn) {
        std::vector<BodyId> ids; for(const auto& m:saturnianMoons()) ids.push_back(m.id); return ids;
    }
    std::vector<BodyId> result;
    for(const auto& b:bodies()) if(b.parent==parent&&(parent!=BodyId::Sun||b.kind==BodyKind::Planet)) result.push_back(b.id);
    return result;
}
const std::array<SolarBody,9>& solarBodies() {
    // JPL physical parameters (equatorial radii), J2000 fitted semimajor axes
    // from JPL approximate positions Table 1, and NASA's mean solar radius.
    // The Earth uses the Earth-Moon barycentre's semimajor axis as a proxy.
    static const std::array<SolarBody,9> physical{{
        {BodyId::Sun,695700,0}, {BodyId::Mercury,2440.53,.38709927},
        {BodyId::Venus,6051.8,.72333566}, {BodyId::Earth,6378.1366,1.00000261},
        {BodyId::Mars,3396.19,1.52371034}, {BodyId::Jupiter,71492,5.20288700},
        {BodyId::Saturn,60268,9.53667594}, {BodyId::Uranus,25559,19.18916464},
        {BodyId::Neptune,24764,30.06992276}
    }};
    return physical;
}
const SolarBody& solarBody(BodyId id) {
    for(const auto& b:solarBodies()) if(b.id==id) return b;
    throw std::runtime_error("Body is not in the solar overview");
}
float solarBodyRadius(BodyId id) {
    return float(solarBody(id).radiusKm/astronomicalUnitKm*solarUnitsPerAu);
}
const std::array<JovianMoon,4>& jovianMoons() {
    // JPL SSD physical parameters and JUP365 mean semimajor axes, retrieved
    // 2026-09-28. Mean radii; orbital distances measured from Jupiter's centre.
    static const std::array<JovianMoon,4> moons{{
        {BodyId::Io,1821.49,421800}, {BodyId::Europa,1560.80,671100},
        {BodyId::Ganymede,2631.20,1070400}, {BodyId::Callisto,2410.30,1882700}
    }};
    return moons;
}
const std::array<MartianMoon,2>& martianMoons() {
    // JPL SSD mean radii and MAR099 mean semimajor axes (2026-09-29).
    // Supplied NASA meshes are normalized by bounding radius. The ratios below
    // come from their signed-volume equivalent radii; calibrate their volumes
    // to the JPL mean radii while preserving each visualization's proportions.
    static const std::array<MartianMoon,2> moons{{
        {BodyId::Phobos,11.08,9375,1.2456285520088957},
        {BodyId::Deimos,6.2,23457,1.3455977654939208}
    }};
    return moons;
}
const std::array<SaturnianMoon,7>& saturnianMoons() {
    // JPL SSD SAT441 mean radii / SAT441 mean elements, checked 2026-09-29.
    // Bounding ratios are calibrated to the signed volume of the pinned meshes.
    static const std::array<SaturnianMoon,7> moons{{
        {BodyId::Mimas,198.20,186000.,1.0615142192004612},
        {BodyId::Enceladus,252.10,238400.,1.0002920208926382},
        {BodyId::Tethys,531.10,295000.,1.0070996774340248},
        {BodyId::Dione,561.40,377700.,1.0002920208926382},
        {BodyId::Rhea,763.50,527200.,1.0002920208926382},
        {BodyId::Titan,2574.76,1221900.,1.0002920208926382},
        {BodyId::Iapetus,734.30,3561700.,1.0002920208926382}
    }};
    return moons;
}
const std::vector<OuterMoon>& outerMoons(BodyId parent) {
    // JPL SSD mean radii and URA182 / NEP097 mean semimajor axes, 2026-09-30.
    // NASA sphere visualization volumes are calibrated to their mean radii.
    static const std::vector<OuterMoon> uranus{{BodyId::Miranda,235.8,129846,1.0002920208926382,4.4},
        {BodyId::Ariel,578.9,190929,1.0002920208926382,0},
        {BodyId::Umbriel,584.7,265986,1.0002920208926382,.1},
        {BodyId::Titania,788.9,436298,1.0002920208926382,.1},
        {BodyId::Oberon,761.4,583511,1.0002920208926382,.1}};
    static const std::vector<OuterMoon> neptune{{BodyId::Proteus,208,117600,1.000083671904011,0},
        {BodyId::Triton,1352.6,354800,1.0002920208926382,157.3}};
    if(parent==BodyId::Uranus) return uranus;
    if(parent==BodyId::Neptune) return neptune;
    throw std::runtime_error("Not an ice giant system");
}
float systemBodyRadius(BodyId parent,BodyId id) {
    if(belts::region(parent)) return float((id==BodyId::Sun?695700.:belts::dwarf(id).boundingKm)/astronomicalUnitKm*belts::units(parent));
    if(ice::planet(parent)) {
        if(id==parent) return float(ice::equatorialKm(id)/outerSystemKmPerUnit);
        for(const auto& m:outerMoons(parent)) if(m.id==id) return float(m.radiusKm*m.boundingRatio/outerSystemKmPerUnit);
    }
    if(parent==BodyId::Saturn) {
        if(id==parent) return float(saturnRadiusKm/saturnSystemKmPerUnit);
        for(const auto& moon:saturnianMoons()) if(moon.id==id)
            return float(moon.radiusKm*moon.boundingRatio/saturnSystemKmPerUnit);
    }
    if(parent==BodyId::Jupiter) {
        if(id==parent) return jovianSceneRadius;
        for(const auto& moon:jovianMoons()) if(moon.id==id)
            return float(moon.radiusKm/jovianRadiusKm*jovianSceneRadius);
    }
    if(parent==BodyId::Mars) {
        if(id==parent) return float(marsRadiusKm/marsSystemKmPerUnit);
        for(const auto& moon:martianMoons()) if(moon.id==id)
            return float(moon.radiusKm*moon.boundingRatio/marsSystemKmPerUnit);
    }
    if(parent==BodyId::Earth) {
        if(id==BodyId::Earth) return float(earthRadiusKm/earthSystemKmPerUnit);
        if(id==BodyId::Moon) return float(moonRadiusKm/earthSystemKmPerUnit);
    }
    if(parent==id) return (id==BodyId::Mars||id==BodyId::Mercury)?.24f:.15f;
    return body(id).displayRadius;
}
float laneRadius(BodyId parent,std::size_t index) {
    if(belts::region(parent)) return float(belts::dwarf(children(parent).at(index)).a*belts::units(parent));
    if(ice::planet(parent)) return float(outerMoons(parent).at(index).orbitKm/outerSystemKmPerUnit);
    if(parent==BodyId::Saturn) return float(saturnianMoons().at(index).orbitKm/saturnSystemKmPerUnit);
    if(parent==BodyId::Sun) return float(solarBodies().at(index+1).orbitAu*solarUnitsPerAu);
    if(parent==BodyId::Jupiter) return float(jovianMoons().at(index).orbitKm/jovianRadiusKm*jovianSceneRadius);
    if(parent==BodyId::Earth) return float(moonOrbitKm/earthSystemKmPerUnit);
    if(parent==BodyId::Mars) return float(martianMoons().at(index).orbitKm/marsSystemKmPerUnit);
    if(children(parent).size()==1) return .65f;
    return .38f+.18f*float(index);
}
mars::Vec3 mapPosition(BodyId parent,BodyId child) {
    if(parent==child) return {};
    if(belts::region(parent)&&belts::isDwarf(child)) return belts::position(belts::dwarf(child),belts::units(parent));
    const double planets[]={55,210,325,125,270,35,160,315},moons[]={35,155,270,335},saturnMoons[]={50,145,245,325,85,205,315};
    auto ids=children(parent);
    for(std::size_t i=0;i<ids.size();++i) if(ids[i]==child) {
        if(ice::planet(parent)) return ice::rotate(mars::direction(0,35+i*137.5)*laneRadius(parent,i),ice::tilt(parent)+outerMoons(parent)[i].inclination);
        return mars::direction(0,parent==BodyId::Sun?planets[i]:parent==BodyId::Saturn?saturnMoons[i]:moons[i%4])*laneRadius(parent,i);
    }
    return {};
}
const SurfaceDataset& moonDataset() {
    static const SurfaceDataset d={"lola64",BodyId::Moon,"data/moon64.dem","MOONDEM1 checked signed half-metre grid",
        "DE421 mean Earth/polar axis; planetocentric latitude; east longitude; pixel centres",
        "Metres above 1737.4 km sphere",1737.4,64,"Global; 23040 x 11520","docs/MOON.md; docs/moon/ldem_64.lbl"};
    return d;
}
const SurfaceDataset& mercuryDataset() {
    static const SurfaceDataset d={"mdis64",BodyId::Mercury,"data/mercury64.dem","MERCDEM2 checked signed half-metre grid",
        "Planetocentric latitude; east longitude; pixel centres","Metres above 2439.4 km sphere",2439.4,64,
        "Global; 23040 x 11520","docs/MERCURY.md; docs/mercury/MSGR_DEM_USG_SC_I_V02.LBL"};
    return d;
}
const SurfaceDataset& molaDataset() {
    static const SurfaceDataset d={"mola16",BodyId::Mars,"data/mars16.mola","MOLA checked signed metre grid",
        "Planetocentric latitude; east longitude; pixel centres","Metres above MOLA areoid",3396.0,16,
        "Global; 5760 x 2880","docs/DATA.md; docs/megt90n000eb.lbl"};
    return d;
}
}
