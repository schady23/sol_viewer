#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "mars_view.hpp"
#include "mercury_view.hpp"
#include "moon_view.hpp"
#include "scan.hpp"
#include "cli.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {
struct Desktop {
    SDL_Window* window=nullptr;
    SDL_GLContext context=nullptr;
    ~Desktop() {
        if(context) SDL_GL_DestroyContext(context);
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};
struct Point { float x=-1,y=-1; };
using solar::cli::number;
Point logicalPoint(SDL_Window* w,const mars::ViewLayout& layout,float x,float y) {
    int ww=0,wh=0; SDL_GetWindowSize(w,&ww,&wh);
    if(ww<=0||wh<=0) return {};
    return {x*layout.pixelWidth/(ww*layout.scale),y*layout.pixelHeight/(wh*layout.scale)};
}
void usage() {
    std::cout<<"Solar System Sim / hierarchical atlas\n"
        "  solar_system_sim [--start solar|system|globe|sun|phobos|deimos|europa|ganymede|callisto|io|venus|earth|mars|mercury|moon|jupiter|saturn|titan|mimas|enceladus|tethys|dione|rhea|iapetus|uranus|neptune|miranda|ariel|umbriel|titania|oberon|triton|proteus|asteroid-belt|kuiper-belt|ceres|pluto|haumea|makemake|eris] [--body name]\n"
        "           [--data data/mars16.mola] [--landmark 1..4 (Moon: 1..16)]\n"
        "           [--lat degrees --lon degrees --distance radii] [--relief 1|10]\n"
        "           [--fullscreen | --windowed] [--hide-ui] [--coarse | --detail 4|8|16]\n"
        "           [--ui-scale 0.75..2] [--topography] [--preview-storm]\n"
        "           [--pause-weather] [--environment-open]\n"
        "           [--orbit --periapsis km --apoapsis km --inclination degrees]\n"
        "           [--orbit-rate 1|10|60|300] [--orbit-paused] [--nadir] [--orbit-phase degrees]\n"
        "           [--mercury-data data/mercury64.dem] [--mercury-detail 4|8|16] [--height-colors]\n"
        "           [--moon-data data/moon64.dem] [--moon-detail 4|8|16]\n"
        "           [--earth-topography] [--no-earth-clouds] [--earth-paused] [--earth-phase 0..60]\n"
        "Earth system: F: whole system; A: approach selected body (body sizes and mean distance to scale).\n"
        "Phobos/Deimos/Europa/Ganymede: L: lighting; R: retry asset; 1-4: destination.\n"
        "Callisto: L: atlas/unlit lighting; R: retry map; 1-4: impact region; [--callisto-unlit].\n"
        "Io: L: atlas/unlit lighting; R: retry map; 1-4: volcanic region; [--io-unlit].\n"
        "Venus: T/C: clouds/inferred-color surface; E: relief; Space: pause; R: reset; 1-4: region.\n"
        "           [--venus-topography] [--venus-paused] [--venus-phase 0..60]\n"
        "Earth: T: topography; E: relief 1/10x; C: clouds; Space: pause; R: restart loop; 1-4: geographic landmark.\n"
        "Moon: 1-4 select a location on the current page; comma/period change pages.\n"
        "           [--info | --location-info] [--scan]\n"
        "           [--titan-surface] (reveal Titan beneath the illustrative haze)\n"
        "           [--sun-paused] [--sun-phase 0..360] (with --start sun)\n"
        "Sun: Space: pause activity; S: speed; R: restart; 1/2/3: close/corona/scale.\n"
        "           [--ice-phase 0..360] [--ice-paused] (with --start uranus or neptune)\n"
        "           [--saturn-rate 1|60|600|3600] [--saturn-paused] [--saturn-low] [--saturn-phase 0..360]\n"
        "           [--jupiter-rate 1|60|600|3600] [--jupiter-paused] [--jupiter-low]\n"
        "           [--jupiter-phase 0..360] (loop phase for comparison views)\n"
        "Saturn: Space: pause; S: speed; V: follow/watch; 1-4: region; R: reset/retry.\n"
        "Mars system: F: whole system; A: approach selected body (sizes and mean distances to scale).\n"
        "Jupiter: Space: pause; S: time speed; V: follow/watch rotation; 1: red spot; R: reset.\n"
        "Solar overview: F: whole system; N: inner planets; Home: reset.\n"
        "Planet systems: A: approach selected body; F: whole system.\n"
        "Jovian system: F: frame all moons; J: approach Jupiter (one physical scale).\n"
        "           [--frames N] [--screenshot output.ppm] [--benchmark]\n"
        "I: planet field guide; select a location for inline notes; R: replay archive scan.\n"
        "Guide: wheel or PgUp/PgDn scrolls; Esc closes; Source opens the cited page.\n"
        "Overview: click to select, Enter/double-click to open, Alt+Left to go up.\n"
        "Mercury: drag / arrows: orbit; wheel / +/-: zoom; 1-4: landmark; E: relief; C: palette.\n"
        "Mars: drag / arrows: orbit; wheel / +/-: zoom; 1-4: landmark; E: relief; Home: reset.\n"
        "F11: fullscreen; Tab: hide/show overlay; Esc: leave fullscreen, otherwise quit.\n"
        "[ / ]: UI scale; T: clean topography; P: pause weather; D: preview storm.\n"
        "O: satellite view; Space: pause orbit; S: orbit speed; V: horizon/nadir.\n"
        "In satellite view, drag to look; wheel adjusts lens FOV; click orbit values to edit.\n"
        "Prepare data: mola_convert docs/megt90n000eb.lbl data/source/megt90n000eb.img data/mars16.mola\n";
}
}
int main(int argc,char** argv) {
    try {
        std::filesystem::path data="data/mars16.mola",mercuryData="data/mercury64.dem",moonData="data/moon64.dem";
        std::string screenshot,startMode;
        int frames=0,meshColumns=5760,requestedLandmark=-1;
        bool moonLaunch=false,explicitMoonData=false;
        moon::View moon;
        bool benchmark=false,explicitData=false,fullscreenRequested=false,windowed=false,showHud=true;
        double sunLaunchDistance=4.8,sunLaunchLat=12,sunLaunchLon=253;
        bool culling=true,previewStorm=false,environmentOpen=false,marsLaunch=false,bodySpecified=false;
        float uiScale=1; double orbitPhase=0;
        bool explicitMercuryData=false,mercuryLaunch=false,infoRequested=false,locationInfo=false,scanRequested=false;
        sun::Clock sunClock;
        solar::Guide guide; solar::SurfaceScan scan;
        auto satelliteViews=[] {
            std::array<satellites::View,satellites::count> views;
            for(int i=0;i<satellites::count;++i) views[i]=satellites::View{satellites::ids[i]};
            return views;
        }();
        callisto::View callisto; bool callistoLaunch=false;
        io::View io; bool ioLaunch=false;
        venus::View venus; bool venusLaunch=false;
        earth::View earth; bool earthLaunch=false;
        saturn::View saturn; bool saturnLow=false,saturnLaunch=false,titanLaunch=false;
        jupiter::View jupiter; bool jupiterLow=false; bool jupiterLaunch=false;
        mercury::View mercury;
        mars::MarsView mars;
        auto& selected=mars.selected; auto& environment=mars.environment; auto& orbit=mars.orbit;
        auto& exaggeration=mars.exaggeration; auto& target=mars.target;
        mars::OrbitElements elements;
        solar::BodyId startBody=solar::BodyId::Mars;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            auto value=[&]() { if(++i>=argc) throw std::runtime_error("Missing value for "+arg); return std::string(argv[i]); };
            if(arg=="--help"||arg=="-h") { usage(); return 0; }
            if(arg=="--start") { startMode=value(); continue; }
            if(arg=="--body") { startBody=solar::bodyNamed(value()); bodySpecified=true; continue; }
            for(const char* option:{"--data","--detail","--coarse","--landmark","--lat","--lon","--distance","--relief",
                                   "--orbit","--satellite","--periapsis","--apoapsis","--inclination","--orbit-phase",
                                   "--orbit-paused","--nadir","--orbit-rate","--topography","--preview-storm","--pause-weather","--environment-open"})
                if(arg==option) marsLaunch=true;
            if(arg=="--callisto-unlit") { callisto.lighting=false; callistoLaunch=true; }
            else if(arg=="--io-unlit") { io.lighting=false; ioLaunch=true; }
            else if(arg=="--venus-topography") { venus.topography=true; venusLaunch=true; }
            else if(arg=="--venus-paused") { venus.clouds.paused=true; venusLaunch=true; }
            else if(arg=="--venus-phase") {
                double phase=number(value());
                if(phase<0||phase>60) throw std::runtime_error("--venus-phase must be 0..60 seconds");
                venus.clouds.seconds=std::fmod(phase,earth::loopSeconds); venusLaunch=true;
            }
            else if(arg=="--earth-phase") {
                double phase=number(value());
                if(phase<0||phase>60) throw std::runtime_error("--earth-phase must be 0..60 seconds");
                earth.clouds.seconds=std::fmod(phase,earth::loopSeconds); earthLaunch=true;
            }
            else if(arg=="--earth-topography") { earth.topography=true; earthLaunch=true; }
            else if(arg=="--earth-paused") { earth.clouds.paused=true; earthLaunch=true; }
            else if(arg=="--no-earth-clouds") { earth.clouds.visible=false; earthLaunch=true; }
            else if(arg=="--sun-paused") sunClock.paused=true;
            else if(arg=="--sun-phase") {
                double p=number(value());
                if(p<0||p>360) throw std::runtime_error("--sun-phase must be 0..360");
                sunClock.seconds=p/360*sun::loopSeconds;
            }
            else if(arg=="--ice-phase") {
                double phase=number(value());
                if(phase<0||phase>360) throw std::runtime_error("--ice-phase must be 0..360 degrees of the full atmospheric loop");
                for(auto& v:satelliteViews) if(ice::planet(v.body)) v.clock.seconds=phase/360*ice::rotationSeconds(v.body)*10;
            }
            else if(arg=="--ice-paused") { for(auto& v:satelliteViews) if(ice::planet(v.body)) v.clock.paused=true; }
            else if(arg=="--titan-surface") { satelliteViews.at(satellites::slot(solar::BodyId::Titan)).haze=false; titanLaunch=true; }
            else if(arg=="--saturn-rate") { saturn.clock.setRate(number(value())); saturnLaunch=true; }
            else if(arg=="--saturn-phase") {
                double phase=number(value());
                if(phase<0||phase>360) throw std::runtime_error("--saturn-phase must be 0..360");
                saturn.clock.seconds=phase/360*saturn::loopSeconds; saturnLaunch=true;
            }
            else if(arg=="--saturn-paused") { saturn.clock.paused=true; saturnLaunch=true; }
            else if(arg=="--saturn-low") { saturnLow=true; saturnLaunch=true; }
            else if(arg=="--jupiter-rate") { jupiter.clock.setRate(number(value())); jupiterLaunch=true; }
            else if(arg=="--jupiter-phase") {
                double phase=number(value());
                if(phase<0||phase>360) throw std::runtime_error("--jupiter-phase must be 0..360");
                jupiter.clock.seconds=phase/360*jupiter::loopSeconds; jupiterLaunch=true;
            }
            else if(arg=="--jupiter-paused") { jupiter.clock.paused=true; jupiterLaunch=true; }
            else if(arg=="--jupiter-low") { jupiterLow=true; jupiterLaunch=true; }
            else if(arg=="--moon-data") { moonData=value(); explicitMoonData=true; moonLaunch=true; }
            else if(arg=="--moon-detail") {
                double ppd=number(value());
                if(ppd!=4&&ppd!=8&&ppd!=16) throw std::runtime_error("--moon-detail must be 4, 8 or 16");
                moon.meshPpd=int(ppd); moonLaunch=true;
            }
            else if(arg=="--mercury-data") { mercuryData=value(); explicitMercuryData=true; mercuryLaunch=true; }
            else if(arg=="--mercury-detail") {
                double ppd=number(value());
                if(ppd!=4&&ppd!=8&&ppd!=16) throw std::runtime_error("--mercury-detail must be 4, 8 or 16");
                mercury.meshPpd=int(ppd); mercuryLaunch=true;
            }
            else if(arg=="--height-colors") { moon.heightColors=mercury.heightColors=true; mercuryLaunch=true; }
            else if(arg=="--data") { data=value(); explicitData=true; }
            else if(arg=="--info") infoRequested=true;
            else if(arg=="--location-info") locationInfo=true;
            else if(arg=="--scan") scanRequested=true;
            else if(arg=="--frames") { frames=solar::cli::integer(value()); if(frames<=0) throw std::runtime_error("--frames must be positive"); }
            else if(arg=="--screenshot") screenshot=value();
            else if(arg=="--benchmark") benchmark=true;
            else if(arg=="--fullscreen") { fullscreenRequested=true; windowed=false; }
            else if(arg=="--windowed") { fullscreenRequested=false; windowed=true; }
            else if(arg=="--hide-ui") showHud=false;
            else if(arg=="--coarse") meshColumns=720;
            else if(arg=="--no-cull") culling=false;
            else if(arg=="--detail") {
                double ppd=number(value());
                if(ppd!=4&&ppd!=8&&ppd!=16) throw std::runtime_error("--detail must be 4, 8 or 16 vertices/degree");
                meshColumns=int(360*ppd);
            }
            else if(arg=="--orbit"||arg=="--satellite") orbit.enabled=true;
            else if(arg=="--periapsis") elements.periapsisKm=number(value());
            else if(arg=="--apoapsis") elements.apoapsisKm=number(value());
            else if(arg=="--inclination") elements.inclinationDeg=number(value());
            else if(arg=="--orbit-phase") orbitPhase=number(value());
            else if(arg=="--orbit-paused") orbit.paused=true;
            else if(arg=="--nadir") orbit.limbView=false;
            else if(arg=="--orbit-rate") {
                orbit.timeScale=number(value());
                if(orbit.timeScale!=1&&orbit.timeScale!=10&&orbit.timeScale!=60&&orbit.timeScale!=300)
                    throw std::runtime_error("--orbit-rate must be 1, 10, 60 or 300");
            }
            else if(arg=="--topography") environment.topography=true;
            else if(arg=="--preview-storm") previewStorm=true;
            else if(arg=="--pause-weather") environment.paused=true;
            else if(arg=="--environment-open") environmentOpen=true;
            else if(arg=="--ui-scale") {
                uiScale=solar::cli::real(value());
                if(!std::isfinite(uiScale)||uiScale<0.75f||uiScale>2.0f)
                    throw std::runtime_error("--ui-scale must be between 0.75 and 2");
            }
            else if(arg=="--landmark") {
                int landmark=solar::cli::integer(value());
                if(landmark<1||landmark>moon::landmarkCount) throw std::runtime_error("Invalid landmark index");
                requestedLandmark=landmark-1;
                moon.select(requestedLandmark);
                if(requestedLandmark<4) {
                    selected=requestedLandmark;
                    auto p=mars::landmarks[selected]; target={p.lat,p.lon,p.distance}; mercury.select(selected); jupiter.select(selected); saturn.select(selected); earth.select(selected,false); venus.select(selected); io.select(selected); callisto.select(selected); for(auto& v:satelliteViews) v.select(selected);
                }
            } else if(arg=="--lat") { double n=number(value()); sunLaunchLat=n; for(auto& v:satelliteViews) v.target.lat=n; target.lat=mercury.target.lat=moon.target.lat=jupiter.target.lat=saturn.target.lat=earth.target.lat=venus.target.lat=io.target.lat=callisto.target.lat=n; }
            else if(arg=="--lon") { double n=number(value()); sunLaunchLon=n; for(auto& v:satelliteViews) v.target.lon=n; target.lon=mercury.target.lon=moon.target.lon=jupiter.target.lon=saturn.target.lon=earth.target.lon=venus.target.lon=io.target.lon=callisto.target.lon=n; }
            else if(arg=="--distance") { float n=solar::cli::real(value()); sunLaunchDistance=n; for(auto& v:satelliteViews) v.target.distance=n; target.distance=mercury.target.distance=moon.target.distance=jupiter.target.distance=saturn.target.distance=earth.target.distance=venus.target.distance=io.target.distance=callisto.target.distance=n; }
            else if(arg=="--relief") {
                exaggeration=mercury.exaggeration=moon.exaggeration=earth.exaggeration=venus.exaggeration=solar::cli::real(value());
                if(exaggeration!=1 && exaggeration!=10) throw std::runtime_error("--relief must be 1 or 10");
            } else throw std::runtime_error("Unknown argument: "+arg);
        }

        if(!std::isfinite(target.lat)||!std::isfinite(target.lon)||!std::isfinite(target.distance))
            throw std::runtime_error("Camera coordinates must be finite");
        callisto::View::clamp(callisto.target); callisto.camera=callisto.target;
        io::View::clamp(io.target); io.camera=io.target;
        venus::View::clamp(venus.target); venus.camera=venus.target;
        earth::View::clamp(earth.target); earth.camera=earth.target;
        saturn::View::clamp(saturn.target); saturn.camera=saturn.target;
        jupiter::View::clamp(jupiter.target); jupiter.camera=jupiter.target;
        moon::View::clamp(moon.target); moon.camera=moon.target;
        mercury::View::clamp(mercury.target); mercury.camera=mercury.target;
        target.clamp(); mars.camera=target; mars.atlasCamera=target;
        orbit.configure(elements); orbit.reset(target.lon,orbitPhase); mars.orbitStarted=orbit.enabled;
        if(orbit.enabled) mars.camera=orbit.camera();
        if(previewStorm) environment.previewStorm({mars.camera.lat,mars.camera.lon});
        solar::Navigation nav;
        if(startMode.empty()) startMode=bodySpecified?"globe":callistoLaunch?"callisto":ioLaunch?"io":venusLaunch?"venus":earthLaunch?"earth":moonLaunch?"moon":titanLaunch?"titan":saturnLaunch?"saturn":jupiterLaunch?"jupiter":mercuryLaunch?"mercury":marsLaunch?"mars":"solar";
        if(startMode=="solar") nav.start({});
        else if(startMode=="phobos"||startMode=="deimos"||startMode=="europa"||startMode=="ganymede"||startMode=="titan"||startMode=="mimas"||startMode=="enceladus"||startMode=="tethys"||startMode=="dione"||startMode=="rhea"||startMode=="iapetus"||startMode=="uranus"||startMode=="neptune"||startMode=="miranda"||startMode=="ariel"||startMode=="umbriel"||startMode=="titania"||startMode=="oberon"||startMode=="triton"||startMode=="proteus") nav.start({solar::ViewKind::Globe,solar::bodyNamed(startMode)});
        else if(startMode=="asteroid-belt"||startMode=="kuiper-belt") nav.start({solar::ViewKind::System,startMode=="asteroid-belt"?solar::BodyId::AsteroidBelt:solar::BodyId::KuiperBelt});
        else if(startMode=="ceres"||startMode=="pluto"||startMode=="haumea"||startMode=="makemake"||startMode=="eris") nav.start({solar::ViewKind::Globe,solar::bodyNamed(startMode)});
        else if(startMode=="sun") nav.start({solar::ViewKind::Globe,solar::BodyId::Sun});
        else if(startMode=="callisto") nav.start({solar::ViewKind::Globe,solar::BodyId::Callisto});
        else if(startMode=="io") nav.start({solar::ViewKind::Globe,solar::BodyId::Io});
        else if(startMode=="venus") nav.start({solar::ViewKind::Globe,solar::BodyId::Venus});
        else if(startMode=="earth") nav.start({solar::ViewKind::Globe,solar::BodyId::Earth});
        else if(startMode=="mars") nav.start({solar::ViewKind::Globe,solar::BodyId::Mars});
        else if(startMode=="moon") nav.start({solar::ViewKind::Globe,solar::BodyId::Moon});
        else if(startMode=="mercury") nav.start({solar::ViewKind::Globe,solar::BodyId::Mercury});
        else if(startMode=="saturn") nav.start({solar::ViewKind::Globe,solar::BodyId::Saturn});
        else if(startMode=="jupiter") nav.start({solar::ViewKind::Globe,solar::BodyId::Jupiter});
        else if(startMode=="system") nav.start({solar::ViewKind::System,startBody});
        else if(startMode=="globe") nav.start({solar::ViewKind::Globe,startBody});
        else throw std::runtime_error("--start must be solar, system, globe, phobos, deimos, europa, ganymede, callisto, io, venus, earth, mars, mercury, moon, jupiter, saturn, sun or a supported moon, dwarf planet, asteroid-belt or kuiper-belt");
        if(nav.sun()) { nav.state().target.lat=std::clamp(sunLaunchLat,-89.,89.); nav.state().target.lon=mars::wrapLongitude(sunLaunchLon); nav.state().target.distance=std::clamp(sunLaunchDistance,1.35,8.); nav.state().camera=nav.state().target; }
        if(nav.earth()&&earth.selected>=0&&earth::landmarks[earth.selected].seafloor) earth.topography=true;
        if(requestedLandmark>=4&&!nav.moon()) throw std::runtime_error("Only Moon supports --landmark above 4");
        // Generic globe options configure the requested body, without changing the
        // other detailed body's initial camera/landmark when visited later.
        if(nav.saturn()||nav.satellite()||nav.view().focus==solar::BodyId::Callisto||nav.view().focus==solar::BodyId::Io||nav.view().focus==solar::BodyId::Venus||nav.view().focus==solar::BodyId::Earth||nav.view().focus==solar::BodyId::Moon||nav.view().focus==solar::BodyId::Mercury||nav.view().focus==solar::BodyId::Jupiter) {
            mars.target={}; mars.atlasCamera=mars.target; mars.selected=-1; mars.exaggeration=10;
            orbit.reset(mars.target.lon,orbitPhase); mars.camera=orbit.enabled?orbit.camera():mars.target;
            if(previewStorm) environment.previewStorm({mars.camera.lat,mars.camera.lon});
        }
        if(nav.view().focus!=solar::BodyId::Mercury) {
            mercury.home(); mercury.camera=mercury.target; mercury.exaggeration=10;
            if(nav.moon()) mercury.heightColors=false;
        }
        if(nav.view().focus!=solar::BodyId::Moon) { moon.home(); moon.camera=moon.target; moon.exaggeration=10; moon.heightColors=false; }
        if(nav.view().focus!=solar::BodyId::Callisto) { callisto.home(); callisto.camera=callisto.target; }
        if(nav.view().focus!=solar::BodyId::Io) { io.home(); io.camera=io.target; }
        if(nav.view().focus!=solar::BodyId::Venus) { venus.home(); venus.camera=venus.target; venus.exaggeration=10; }
        if(nav.view().focus!=solar::BodyId::Earth) { earth.home(); earth.camera=earth.target; earth.exaggeration=10; }
        if(nav.view().focus!=solar::BodyId::Saturn) { saturn.home(); saturn.camera=saturn.target; }
        if(nav.view().focus!=solar::BodyId::Jupiter) { jupiter.home(); jupiter.camera=jupiter.target; }
        for(auto& v:satelliteViews) {
            if(nav.view().focus!=v.body) v.home();
            satellites::View::clamp(v.target); v.camera=v.target;
        }
        auto satelliteView=[&]() -> satellites::View& { return satelliteViews.at(nav.satellite()?satellites::slot(nav.view().focus):0); };
        if(locationInfo) {
            if(nav.satellite()&&satelliteView().selected<0) { satelliteView().select(0); satelliteView().camera=satelliteView().target; }
            if(nav.mars()&&mars.selected<0&&!orbit.enabled) {
                auto p=mars::landmarks[0]; mars.selected=0; mars.target={p.lat,p.lon,p.distance}; mars.camera=mars.target;
            }
            if(nav.callisto()&&callisto.selected<0) { callisto.select(0); callisto.camera=callisto.target; }
            if(nav.earth()&&earth.selected<0) { earth.select(0); earth.camera=earth.target; }
            if(nav.io()&&io.selected<0) { io.select(0); io.camera=io.target; }
            if(nav.moon()&&moon.selected<0) { moon.select(0); moon.camera=moon.target; }
            if(nav.mercury()&&mercury.selected<0) { mercury.select(0); mercury.camera=mercury.target; }
            if(nav.saturn()&&saturn.selected<0) { saturn.select(0); saturn.camera=saturn.target; }
            if(nav.jupiter()&&jupiter.selected<0) { jupiter.select(0); jupiter.camera=jupiter.target; }
        }
        if(!explicitData && !std::filesystem::exists(data)) {
            const char* executablePath=SDL_GetBasePath();
            std::filesystem::path base=executablePath?executablePath:".";
            for(auto candidate:{base/"data/mars16.mola",base.parent_path()/"data/mars16.mola",base.parent_path().parent_path()/"data/mars16.mola"})
                if(std::filesystem::exists(candidate)) { data=candidate; break; }
        }

        if(!explicitMercuryData&&!std::filesystem::exists(mercuryData)) {
            const char* executablePath=SDL_GetBasePath();
            std::filesystem::path base=executablePath?executablePath:".";
            for(auto candidate:{base/"data/mercury64.dem",base.parent_path()/"data/mercury64.dem",base.parent_path().parent_path()/"data/mercury64.dem"})
                if(std::filesystem::exists(candidate)) { mercuryData=candidate; break; }
        }
        if(!explicitMoonData&&!std::filesystem::exists(moonData)) {
            const char* executablePath=SDL_GetBasePath();
            std::filesystem::path base=executablePath?executablePath:".";
            for(auto candidate:{base/"data/moon64.dem",base.parent_path()/"data/moon64.dem",base.parent_path().parent_path()/"data/moon64.dem"})
                if(std::filesystem::exists(candidate)) { moonData=candidate; break; }
        }

        Desktop desktop;
        if(!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1); SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
        desktop.window=SDL_CreateWindow("Solar System Sim",1600,1000,
            SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY|
            (fullscreenRequested?SDL_WINDOW_FULLSCREEN:windowed?0:SDL_WINDOW_MAXIMIZED));
        if(!desktop.window) throw std::runtime_error(SDL_GetError());
        SDL_SetWindowMinimumSize(desktop.window,800,500);
        desktop.context=SDL_GL_CreateContext(desktop.window);
        if(!desktop.context) throw std::runtime_error(std::string("OpenGL 3.3 required: ")+SDL_GetError());
        bool vsync=!benchmark&&SDL_GL_SetSwapInterval(1); if(benchmark) SDL_GL_SetSwapInterval(0);
        mars::Renderer renderer; renderer.culling(culling);
        mercury::Loader mercuryLoader;
        moon::Loader moonLoader;
        mars::MarsLoader loader; // Joins its worker before renderer/context destruction.
        auto resize=[&]() { int w=0,h=0; SDL_GetWindowSizeInPixels(desktop.window,&w,&h); renderer.resize(w,h); };
        resize(); renderer.uiScale(uiScale);
        if(environmentOpen) renderer.toggleEnvironment();
        auto syncGuide=[&]() {
            auto body=nav.view().focus;
            if(nav.view().kind==solar::ViewKind::Solar||nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Earth}) body=nav.state().selected;
            guide.sync(body,nav.earth()?earth.selected:nav.mars()?mars.selected:nav.mercury()?mercury.selected:nav.moon()?moon.selected:nav.saturn()?saturn.selected:nav.jupiter()?jupiter.selected:nav.satellite()?satelliteView().selected:nav.callisto()?callisto.selected:nav.io()?io.selected:-1);
            const auto& layout=renderer.layout();
            bool locationAvailable=(layout.marsView||layout.mercuryView||layout.moonView||layout.jupiterView||layout.saturnView||layout.earthView||layout.ioView||layout.callistoView||layout.satelliteView)&&!layout.satellite&&guide.location>=0;
            renderer.guideMode(solar::hasGuide(guide.body),guide.open,locationAvailable,guide.locationOpen);
            guide.move(0,renderer.layout()); guide.moveLocation(0,renderer.layout());
        };
        auto venusActive=[&]() { return nav.view().focus==solar::BodyId::Venus&&nav.view().kind!=solar::ViewKind::Solar; };
        auto earthActive=[&]() { return nav.view().focus==solar::BodyId::Earth&&nav.view().kind!=solar::ViewKind::Solar; };
        auto saturnActive=[&]() { return nav.view().focus==solar::BodyId::Saturn&&nav.view().kind!=solar::ViewKind::Solar; };
        auto jupiterActive=[&]() { return nav.view().focus==solar::BodyId::Jupiter&&nav.view().kind!=solar::ViewKind::Solar; };
        auto syncLayout=[&](bool marsReady,bool mercuryReady,bool moonReady) {
            renderer.beltMode(nav.belt()); renderer.explorerMode(marsReady,int(nav.visibleBodies().size()),mercuryReady,nav.view().kind==solar::ViewKind::Solar,nav.view().kind==solar::ViewKind::System); renderer.satelliteMode(marsReady&&orbit.enabled);
            renderer.moonMode(moonReady);
            renderer.sunMode(nav.sun());
            renderer.titanMode(nav.view()==solar::ViewKey{solar::ViewKind::Globe,solar::BodyId::Titan});
            renderer.outerMode(nav.view().kind==solar::ViewKind::System&&ice::planet(nav.view().focus),nav.satellite()&&ice::planet(nav.view().focus));
            renderer.saturnSystemMode(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Saturn});
            renderer.marsSystemMode(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Mars});
            renderer.jupiterMode(nav.jupiter(),jupiterActive());
            renderer.saturnMode(nav.saturn(),saturnActive());
            renderer.callistoMode(nav.callisto());
            renderer.satelliteModeGlobe(nav.satellite());
            renderer.ioMode(nav.io());
            renderer.venusMode(nav.venus(),venusActive());
            renderer.earthMode(nav.earth(),nav.earthControls());
            syncGuide();
        };
        auto canScan=[&]() {
            return (nav.mars()&&loader.status==mars::MarsLoader::Status::Ready)||
                   (nav.mercury()&&mercuryLoader.status==mercury::Loader::Status::Ready)||
                   (nav.moon()&&moonLoader.status==moon::Loader::Status::Ready)||
                   (nav.view().kind==solar::ViewKind::System&&renderer.previewAvailable(nav.view().focus));
        };
        syncGuide(); guide.open=infoRequested&&solar::hasGuide(guide.body);
        auto scanView=nav.view(); if(scanRequested) scan.replay();
        std::cout<<"OpenGL: "<<renderer.device()<<" / "<<solar::bodyCount<<" catalog bodies / physical solar scale\n";
        bool running=true,pressed=false,dragging=false;
        solar::BodyId lastSceneSelection=solar::BodyId::None;
        auto lastSceneSelectionView=nav.view();
        Point mouse,previous,press;
        auto cancelDrag=[&]() { pressed=dragging=false; SDL_CaptureMouse(false); };
        auto fullscreen=[&](bool enabled) {
            cancelDrag();
            if(SDL_SetWindowFullscreen(desktop.window,enabled)) fullscreenRequested=enabled;
            else SDL_Log("Fullscreen request failed: %s",SDL_GetError());
        };
        auto changeUiScale=[&](float delta) {
            uiScale=std::clamp(uiScale+delta,.75f,2.0f); renderer.uiScale(uiScale);
            float x=0,y=0; SDL_GetMouseState(&x,&y); mouse=logicalPoint(desktop.window,renderer.layout(),x,y);
            previous=mouse; cancelDrag();
        };
        auto stopForNavigation=[&]() { cancelDrag(); mars.stopEdit(desktop.window); mouse={}; };
        Uint64 last=SDL_GetTicksNS(),start=last,fpsStart=last;
        float fps=0; int rendered=0,settledFrames=0,fpsFrames=0;
        auto previousView=nav.view();
        while(running) {
            Uint64 frameStart=SDL_GetTicksNS();
            double realDt=double(frameStart-last)/1e9,dt=std::min(realDt,.25); last=frameStart;
            resize();
            if(nav.mars()&&loader.status==mars::MarsLoader::Status::Unloaded) {
                std::cout<<"Preparing Mars: "<<data.string()<<'\n'; loader.request(data,meshColumns);
            }
            if(nav.mercury()&&mercuryLoader.status==mercury::Loader::Status::Unloaded) {
                std::cout<<"Preparing Mercury: "<<mercuryData.string()<<'\n'; mercuryLoader.request(mercuryData,360*mercury.meshPpd);
            }
            if(nav.moon()&&moonLoader.status==moon::Loader::Status::Unloaded) {
                std::cout<<"Preparing Moon: "<<moonData.string()<<'\n'; moonLoader.request(moonData,360*moon.meshPpd);
            }
            loader.poll(renderer); mercuryLoader.poll(renderer); moonLoader.poll(renderer);
            bool marsReady=nav.mars()&&loader.status==mars::MarsLoader::Status::Ready;
            bool moonReady=nav.moon()&&moonLoader.status==moon::Loader::Status::Ready;
            bool mercuryReady=nav.mercury()&&mercuryLoader.status==mercury::Loader::Status::Ready;
            syncLayout(marsReady,mercuryReady,moonReady);
            auto scene=solar::makeScene(nav,renderer.layout());
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT) running=false;
                else if(event.type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED||event.type==SDL_EVENT_WINDOW_RESIZED) {
                    resize(); cancelDrag(); mouse={}; previous={};
                    scene=solar::makeScene(nav,renderer.layout());
                } else if(event.type==SDL_EVENT_WINDOW_ENTER_FULLSCREEN) fullscreenRequested=true;
                else if(event.type==SDL_EVENT_WINDOW_LEAVE_FULLSCREEN) fullscreenRequested=false;
                else if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST) cancelDrag();
                else if(event.type==SDL_EVENT_WINDOW_MOUSE_LEAVE&&!pressed) mouse={};
                else if(event.type==SDL_EVENT_KEY_DOWN) {
                    auto key=event.key.key;
                    if(marsReady&&mars.edit.field>=0) { mars.key(key,desktop.window); continue; }
                    if(event.key.repeat) continue;
                    if(key==SDLK_ESCAPE&&guide.open&&showHud) { guide.open=false; syncGuide(); }
                    else if(key==SDLK_ESCAPE&&showHud&&renderer.layout().locationAvailable&&guide.locationOpen) { guide.locationOpen=false; syncGuide(); }
                    else if(key==SDLK_ESCAPE) { if(fullscreenRequested) fullscreen(false); else running=false; }
                    else if(key==SDLK_F11) fullscreen(!fullscreenRequested);
                    else if(key==SDLK_TAB) showHud=!showHud;
                    else if(key==SDLK_LEFTBRACKET) changeUiScale(-.25f);
                    else if(key==SDLK_RIGHTBRACKET) changeUiScale(.25f);
                    else if(key==SDLK_I&&solar::hasGuide(guide.body)) {
                        if(!showHud) { showHud=true; guide.open=true; } else guide.toggle();
                        syncGuide(); cancelDrag();
                    }
                    else if(guide.open&&showHud&&(key==SDLK_PAGEUP||key==SDLK_PAGEDOWN||key==SDLK_UP||key==SDLK_DOWN||key==SDLK_HOME||key==SDLK_END)) {
                        int rows=key==SDLK_HOME?-10000:key==SDLK_END?10000:key==SDLK_PAGEUP?-renderer.layout().guideRows():
                                 key==SDLK_PAGEDOWN?renderer.layout().guideRows():key==SDLK_UP?-3:3;
                        guide.move(rows,renderer.layout());
                    }
                    else if(showHud&&renderer.layout().locationAvailable&&guide.locationOpen&&(key==SDLK_PAGEUP||key==SDLK_PAGEDOWN))
                        guide.moveLocation(key==SDLK_PAGEUP?-renderer.layout().locationRows():renderer.layout().locationRows(),renderer.layout());
                    else if(nav.transitioning()) continue;
                    else if(key==SDLK_LEFT&&(event.key.mod&SDL_KMOD_ALT)) { nav.up(); stopForNavigation(); }
                    else if(nav.mars()&&key==SDLK_R&&loader.status==mars::MarsLoader::Status::Failed) loader.request(data,meshColumns);
                    else if(nav.moon()&&key==SDLK_R&&moonLoader.status==moon::Loader::Status::Failed) moonLoader.request(moonData,360*moon.meshPpd);
                    else if(nav.mercury()&&key==SDLK_R&&mercuryLoader.status==mercury::Loader::Status::Failed) mercuryLoader.request(mercuryData,360*mercury.meshPpd);
                    else if(nav.view().kind!=solar::ViewKind::Globe&&key==SDLK_A) nav.approachSelected();
                    else if(nav.belt()&&key==SDLK_F) nav.resetCamera();
                    else if(nav.view().kind==solar::ViewKind::System&&key==SDLK_F&&solar::children(nav.view().focus).empty()) nav.resetCamera();
                    else if(nav.view().kind==solar::ViewKind::Solar&&(key==SDLK_F||key==SDLK_N)) nav.frameSolar(key==SDLK_F);
                    else if(nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Mars}&&(key==SDLK_F||key==SDLK_A)) nav.frameMars(key==SDLK_F);
                    else if(jupiterActive()&&!nav.jupiter()&&(key==SDLK_F||key==SDLK_J)) nav.frameJovian(key==SDLK_F);
                    else if(jupiterActive()&&(nav.jupiter()||key==SDLK_SPACE||key==SDLK_S||key==SDLK_R)) {
                        if(key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        jupiter.key(key);
                    }
                    else if(nav.view().kind==solar::ViewKind::System&&ice::planet(nav.view().focus)&&(key==SDLK_SPACE||key==SDLK_S||key==SDLK_R)) satelliteViews.at(satellites::slot(nav.view().focus)).key(key);
                    else if(nav.view().kind==solar::ViewKind::System&&ice::planet(nav.view().focus)&&(key==SDLK_F||key==SDLK_N||key==SDLK_A)) nav.frameOuter(key==SDLK_F?0:key==SDLK_N?1:2);
                    else if(saturnActive()&&!nav.saturn()&&(key==SDLK_F||key==SDLK_N||key==SDLK_A)) nav.frameSaturn(key==SDLK_F?0:key==SDLK_N?1:2);
                    else if(saturnActive()&&(nav.saturn()||key==SDLK_SPACE||key==SDLK_S||key==SDLK_R)) {
                        if(key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        saturn.key(key);
                    }
                    else if(nav.satellite()) {
                        if(key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        satelliteView().key(key);
                    }
                    else if(nav.callisto()) {
                        if(key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        callisto.key(key);
                    }
                    else if(nav.io()) {
                        if(key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        io.key(key);
                    }
                    else if(venusActive()&&(nav.venus()||key==SDLK_T||key==SDLK_C||key==SDLK_SPACE||key==SDLK_R)) venus.key(key);
                    else if(earthActive()&&!nav.earth()&&(key==SDLK_F||key==SDLK_A)) nav.frameEarth(key==SDLK_F);
                    else if(nav.earthControls()&&(nav.earth()||key==SDLK_C||key==SDLK_SPACE||key==SDLK_R||key==SDLK_T)) {
                        if(nav.earth()&&key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        earth.key(key);
                    }
                    else if(nav.sun()&&(key==SDLK_SPACE||key==SDLK_S||key==SDLK_R||key==SDLK_1||key==SDLK_2||key==SDLK_3||key==SDLK_LEFT||key==SDLK_RIGHT||key==SDLK_UP||key==SDLK_DOWN)) {
                        if(key==SDLK_SPACE) sunClock.paused=!sunClock.paused;
                        if(key==SDLK_S) sunClock.cycle();
                        if(key==SDLK_R) sunClock.seconds=0;
                        if(key>=SDLK_1&&key<=SDLK_3) nav.state().target.distance=key==SDLK_1?2.15:key==SDLK_2?5.8:4.8;
                        if(key==SDLK_LEFT||key==SDLK_RIGHT) nav.drag(key==SDLK_LEFT?20:-20,0);
                        if(key==SDLK_UP||key==SDLK_DOWN) nav.drag(0,key==SDLK_UP?20:-20);
                    }
                    else if(key==SDLK_R&&canScan()) scan.replay();
                    else if(mercuryReady||moonReady||marsReady) {
                        if(key>=SDLK_1&&key<=SDLK_4) { guide.open=false; guide.locationOpen=true; }
                        if(moonReady) moon.key(key);
                        else if(mercuryReady) mercury.key(key);
                        else { mars.key(key,desktop.window); renderer.satelliteMode(orbit.enabled); }
                    }
                    else {
                        if(key==SDLK_RETURN||key==SDLK_KP_ENTER) { nav.open(); stopForNavigation(); }
                        if(key==SDLK_UP) nav.cycleSelection(-1);
                        if(key==SDLK_DOWN) nav.cycleSelection(1);
                        if(key==SDLK_HOME) nav.resetCamera();
                        if(key==SDLK_EQUALS||key==SDLK_KP_PLUS) nav.zoom(1);
                        if(key==SDLK_MINUS||key==SDLK_KP_MINUS) nav.zoom(-1);
                    }
                } else if(event.type==SDL_EVENT_TEXT_INPUT&&marsReady&&mars.edit.field>=0) {
                    auto& edit=mars.edit;
                    if(edit.replace) { edit.value.clear(); edit.replace=false; }
                    if(edit.value.size()+std::char_traits<char>::length(event.text.text)<=16) edit.value+=event.text.text;
                    edit.error.clear();
                } else if(event.type==SDL_EVENT_MOUSE_MOTION) {
                    mouse=logicalPoint(desktop.window,renderer.layout(),event.motion.x,event.motion.y);
                    if(pressed&&!nav.transitioning()) {
                        float dx=mouse.x-press.x,dy=mouse.y-press.y;
                        dragging=dragging||dx*dx+dy*dy>16;
                        if(dragging) {
                            if(marsReady) mars.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(moonReady) moon.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(mercuryReady) mercury.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.satellite()) satelliteView().drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.callisto()) callisto.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.io()) io.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.venus()) venus.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.earth()) earth.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.saturn()) saturn.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else if(nav.jupiter()) jupiter.drag(mouse.x-previous.x,mouse.y-previous.y,renderer.layout().height());
                            else nav.drag(mouse.x-previous.x,mouse.y-previous.y);
                        }
                    }
                    previous=mouse;
                } else if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT) {
                    if(nav.transitioning()) continue;
                    const auto layout=renderer.layout();
                    mouse=logicalPoint(desktop.window,layout,event.button.x,event.button.y); previous=press=mouse;
                    if(showHud&&solar::hasGuide(guide.body)) {
                        if(layout.guideToggle().contains(mouse.x,mouse.y)) {
                            if(marsReady&&mars.edit.field>=0&&!mars.commitEdit(desktop.window)) continue;
                            guide.toggle(); syncGuide(); cancelDrag(); continue;
                        }
                        if(canScan()&&layout.scanButton().contains(mouse.x,mouse.y)) { scan.replay(); cancelDrag(); continue; }
                        if(guide.open&&layout.guidePanel().contains(mouse.x,mouse.y)) {
                            if(layout.guideClose().contains(mouse.x,mouse.y)) guide.open=false;
                            else if(layout.guideUp().contains(mouse.x,mouse.y)) guide.move(-renderer.layout().guideRows(),layout);
                            else if(layout.guideDown().contains(mouse.x,mouse.y)) guide.move(renderer.layout().guideRows(),layout);
                            else if(layout.guideSource().contains(mouse.x,mouse.y)) {
                                if(!SDL_OpenURL(guide.article().source)) SDL_Log("Could not open source: %s",SDL_GetError());
                            }
                            syncGuide(); cancelDrag(); continue;
                        }
                        if(guide.open) { guide.open=false; syncGuide(); }
                        if(layout.locationAvailable) {
                            if(layout.locationToggle().contains(mouse.x,mouse.y)) {
                                guide.locationOpen=!guide.locationOpen; syncGuide(); cancelDrag(); continue;
                            }
                            if(guide.locationOpen&&layout.locationPanel().contains(mouse.x,mouse.y)) {
                                if(layout.locationUp().contains(mouse.x,mouse.y)) guide.moveLocation(-layout.locationRows(),layout);
                                else if(layout.locationDown().contains(mouse.x,mouse.y)) guide.moveLocation(layout.locationRows(),layout);
                                else if(layout.locationSource().contains(mouse.x,mouse.y)) {
                                    if(!SDL_OpenURL(solar::locationArticle(guide.body,guide.location).source)) SDL_Log("Could not open source: %s",SDL_GetError());
                                }
                                cancelDrag(); continue;
                            }
                            for(int i=0;i<4;++i) if(layout.landmarkButton(i).contains(mouse.x,mouse.y)) guide.locationOpen=true;
                        }
                    }
                    if(showHud) {
                        auto crumbs=nav.breadcrumbs(); auto boxes=solar::crumbRects(nav,layout);
                        bool navigationClick=false;
                        for(std::size_t i=0;i<boxes.size();++i) if(boxes[i].contains(mouse.x,mouse.y)) {
                            nav.go(crumbs[i].view); navigationClick=true;
                        }
                        if(layout.navigationUp().contains(mouse.x,mouse.y)&&nav.view().kind!=solar::ViewKind::Solar) {
                            nav.up(); navigationClick=true;
                        }
                        if(navigationClick) { stopForNavigation(); continue; }
                    }
                    if(marsReady&&mars.edit.field>=0&&!layout.orbitValue(mars.edit.field).contains(mouse.x,mouse.y)&&!mars.commitEdit(desktop.window)) continue;
                    if(showHud&&layout.overHud(mouse.x,mouse.y)) {
                        if(layout.fullscreenButton().contains(mouse.x,mouse.y)) fullscreen(!fullscreenRequested);
                        if(layout.scaleDown().contains(mouse.x,mouse.y)) changeUiScale(-.25f);
                        else if(layout.scaleUp().contains(mouse.x,mouse.y)) changeUiScale(.25f);
                        if(nav.view().kind==solar::ViewKind::Solar) {
                            if(layout.solarWhole().contains(mouse.x,mouse.y)) nav.frameSolar(true);
                            if(layout.solarInner().contains(mouse.x,mouse.y)) nav.frameSolar(false);
                        }
                        if(nav.view().kind==solar::ViewKind::System&&layout.earthApproach().contains(mouse.x,mouse.y)&&!layout.marsSystem&&!layout.saturnSystem&&!layout.outerSystem&&!jupiterActive()&&!earthActive()) nav.approachSelected();
                        if(nav.view().kind==solar::ViewKind::System&&(nav.belt()||solar::children(nav.view().focus).empty())&&layout.earthWhole().contains(mouse.x,mouse.y)) nav.resetCamera();
                        if(nav.sun()) {
                            if(layout.sunPause().contains(mouse.x,mouse.y)) sunClock.paused=!sunClock.paused;
                            if(layout.sunSpeed().contains(mouse.x,mouse.y)) sunClock.cycle();
                            if(layout.sunReset().contains(mouse.x,mouse.y)) sunClock.seconds=0;
                            if(layout.sunClose().contains(mouse.x,mouse.y)) nav.state().target.distance=2.15;
                            if(layout.sunCrown().contains(mouse.x,mouse.y)) nav.state().target.distance=5.8;
                            if(layout.sunScale().contains(mouse.x,mouse.y)) nav.state().target.distance=4.8;
                        }
                        if(layout.marsSystem) {
                            if(layout.marsWhole().contains(mouse.x,mouse.y)) nav.frameMars(true);
                            if(layout.marsApproach().contains(mouse.x,mouse.y)) nav.approachSelected();
                        }
                        if(layout.outerSystem) {
                            auto& v=satelliteViews.at(satellites::slot(nav.view().focus));
                            if(layout.ioLighting().contains(mouse.x,mouse.y)) v.key(SDLK_SPACE);
                            if(layout.icePause().contains(mouse.x,mouse.y)) v.key(SDLK_S);
                            if(layout.iceSpeed().contains(mouse.x,mouse.y)) v.key(SDLK_R);
                            if(layout.saturnWhole().contains(mouse.x,mouse.y)) nav.frameOuter(0);
                            if(layout.saturnInner().contains(mouse.x,mouse.y)) nav.frameOuter(1);
                            if(layout.saturnApproach().contains(mouse.x,mouse.y)) nav.approachSelected();
                        }
                        if(layout.saturnSystem) {
                            if(layout.saturnWhole().contains(mouse.x,mouse.y)) nav.frameSaturn(0);
                            if(layout.saturnInner().contains(mouse.x,mouse.y)) nav.frameSaturn(1);
                            if(layout.saturnApproach().contains(mouse.x,mouse.y)) nav.approachSelected();
                        }
                        if(saturnActive()) saturn.click(layout,mouse.x,mouse.y);
                        if(jupiterActive()) {
                            jupiter.click(layout,mouse.x,mouse.y);
                            if(!nav.jupiter()) {
                                if(layout.jupiterFrame().contains(mouse.x,mouse.y)) nav.frameJovian(true);
                                if(layout.jupiterApproach().contains(mouse.x,mouse.y)) nav.approachSelected();
                            }
                        }
                        if(nav.satellite()) satelliteView().click(layout,mouse.x,mouse.y);
                        if(nav.callisto()) callisto.click(layout,mouse.x,mouse.y);
                        if(nav.io()) io.click(layout,mouse.x,mouse.y);
                        if(venusActive()) venus.click(layout,mouse.x,mouse.y);
                        if(earthActive()) {
                            if(nav.earthControls()) earth.click(layout,mouse.x,mouse.y);
                            if(!nav.earth()) {
                                if(layout.earthWhole().contains(mouse.x,mouse.y)) { if(nav.belt()) nav.resetCamera(); else nav.frameEarth(true); }
                                if(layout.earthApproach().contains(mouse.x,mouse.y)) nav.approachSelected();
                            }
                        }
                        if(nav.saturn()||nav.satellite()||nav.callisto()||nav.io()||nav.venus()||nav.earth()||nav.jupiter()) { /* Dedicated globe controls handled above. */ }
                        else if(marsReady) {
                            if(layout.environmentButton().contains(mouse.x,mouse.y)) renderer.toggleEnvironment();
                            mars.click(layout,mouse.x,mouse.y,desktop.window); renderer.satelliteMode(orbit.enabled);
                        } else if(moonReady) { moon.click(layout,mouse.x,mouse.y); } else if(mercuryReady) { mercury.click(layout,mouse.x,mouse.y); } else {
                            if(layout.homeButton().contains(mouse.x,mouse.y)) nav.resetCamera();
                            if(nav.view().kind==solar::ViewKind::Solar) {
                                if(layout.asteroidBelt().contains(mouse.x,mouse.y)) nav.selectAndApproach(solar::BodyId::AsteroidBelt);
                                else if(layout.kuiperBelt().contains(mouse.x,mouse.y)) nav.selectAndApproach(solar::BodyId::KuiperBelt);
                            }
                            auto ids=nav.visibleBodies();
                            for(std::size_t i=0;i<ids.size();++i) if(layout.bodyButton(int(i)).contains(mouse.x,mouse.y)) {
                                nav.selectAndApproach(ids[i]); if(event.button.clicks>=2) nav.open();
                            }
                            if(layout.openBody().contains(mouse.x,mouse.y)) {
                                if(nav.mars()) loader.request(data,meshColumns);
                                else if(nav.moon()) moonLoader.request(moonData,360*moon.meshPpd);
                                else if(nav.mercury()) mercuryLoader.request(mercuryData,360*mercury.meshPpd);
                                else nav.open();
                            }
                        }
                        if(nav.transitioning()) stopForNavigation();
                    } else { pressed=true; dragging=false; SDL_CaptureMouse(true); }
                } else if(event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_LEFT) {
                    mouse=logicalPoint(desktop.window,renderer.layout(),event.button.x,event.button.y);
                    if(pressed&&!dragging&&!marsReady&&!mercuryReady&&!moonReady&&!nav.jupiter()&&!nav.saturn()&&!nav.earth()&&!nav.venus()&&!nav.io()&&!nav.callisto()&&!nav.satellite()&&!nav.transitioning()) {
                        auto id=solar::pickBody(scene,renderer.layout(),mouse.x,mouse.y,showHud);
                        // The first click starts a camera move, so the second
                        // click may no longer hit the body's original locator.
                        if(event.button.clicks>=2&&lastSceneSelection!=solar::BodyId::None&&lastSceneSelectionView==nav.view()&&nav.state().selected==lastSceneSelection) {
                            nav.open(); lastSceneSelection=solar::BodyId::None;
                        } else if(id!=solar::BodyId::None) {
                            lastSceneSelection=id; lastSceneSelectionView=nav.view();
                            nav.selectAndApproach(id); if(event.button.clicks>=2) nav.open();
                        } else lastSceneSelection=solar::BodyId::None;
                    }
                    cancelDrag(); if(nav.transitioning()) stopForNavigation();
                } else if(event.type==SDL_EVENT_MOUSE_WHEEL) {
                    if(showHud&&guide.open&&renderer.layout().guidePanel().contains(mouse.x,mouse.y)) {
                        float amount=event.wheel.y*(event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-1.f:1.f);
                        if(amount!=0) guide.move(-int(std::copysign(std::max(1.f,std::abs(amount)*3),amount)),renderer.layout());
                        continue;
                    }
                    if(showHud&&renderer.layout().locationAvailable&&guide.locationOpen&&renderer.layout().locationPanel().contains(mouse.x,mouse.y)) {
                        float amount=event.wheel.y*(event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-1.f:1.f);
                        if(amount!=0) guide.moveLocation(-int(std::copysign(std::max(1.f,std::abs(amount)*3),amount)),renderer.layout());
                        continue;
                    }
                    if(nav.transitioning()||(showHud&&renderer.layout().overHud(mouse.x,mouse.y))) continue;
                    float amount=event.wheel.y*(event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-1.f:1.f);
                    if(marsReady) mars.zoom(amount); else if(moonReady) moon.zoom(amount); else if(mercuryReady) mercury.zoom(amount); else if(nav.satellite()) satelliteView().zoom(amount); else if(nav.callisto()) callisto.zoom(amount); else if(nav.io()) io.zoom(amount); else if(nav.venus()) venus.zoom(amount); else if(nav.earth()) earth.zoom(amount); else if(nav.saturn()) saturn.zoom(amount); else if(nav.jupiter()) jupiter.zoom(amount); else nav.zoom(amount);
                }
            }
            if(!running) break;
            if((SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_MINIMIZED)&&frames==0) { SDL_Delay(40); continue; }
            if(marsReady&&!nav.transitioning()) mars.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            if(mercuryReady&&!nav.transitioning()) mercury.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            if(moonReady&&!nav.transitioning()) moon.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            if(nav.satellite()&&!nav.transitioning()) satelliteView().update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            if(nav.callisto()&&!nav.transitioning()) callisto.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            if(nav.io()&&!nav.transitioning()) io.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            if(venusActive()&&!nav.transitioning()) {
                if(renderer.venusAvailable()&&!venus.topography) venus.clouds.advance(realDt);
                if(nav.venus()) venus.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0);
            }
            if((earthActive()||nav.view().kind==solar::ViewKind::Solar)&&!nav.transitioning()) {
                if(renderer.earthAvailable()) earth.clouds.advance(realDt);
                if(nav.earth()) earth.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            }
            if(saturnActive()&&!nav.transitioning()) {
                if(renderer.saturnAvailable()) saturn.clock.advance(dt);
                if(nav.saturn()) saturn.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            }
            if(jupiterActive()&&!nav.transitioning()) {
                if(renderer.jupiterAvailable()) jupiter.clock.advance(dt);
                if(nav.jupiter()) jupiter.update(dt,SDL_GetKeyboardState(nullptr),(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_INPUT_FOCUS)!=0&&!(guide.open&&showHud));
            }
            if((nav.sun()||nav.view().kind==solar::ViewKind::Solar)&&!nav.transitioning()) sunClock.advance(dt);
            nav.update(dt);
            if(nav.view()!=previousView) {
                previousView=nav.view();
                std::cout<<"View: "<<int(nav.view().kind)<<" / "<<solar::body(nav.view().focus).name<<'\n';
            }
            marsReady=nav.mars()&&loader.status==mars::MarsLoader::Status::Ready;
            moonReady=nav.moon()&&moonLoader.status==moon::Loader::Status::Ready;
            mercuryReady=nav.mercury()&&mercuryLoader.status==mercury::Loader::Status::Ready;
            syncLayout(marsReady,mercuryReady,moonReady);
            scene=solar::makeScene(nav,renderer.layout());
            std::string status;
            bool loading=(nav.mars()&&!marsReady&&loader.status!=mars::MarsLoader::Status::Failed)||
                         (nav.mercury()&&!mercuryReady&&mercuryLoader.status!=mercury::Loader::Status::Failed)||
                         (nav.moon()&&!moonReady&&moonLoader.status!=moon::Loader::Status::Failed);
            if(scanView!=nav.view()) { scan={}; scanView=nav.view(); }
            scan.update(dt,loading); renderer.scan(scan.phase(),scan.strength());
            if(loading) {
                int phase=nav.mars()?loader.phase.load(std::memory_order_relaxed):nav.moon()?moonLoader.phase.load(std::memory_order_relaxed):mercuryLoader.phase.load(std::memory_order_relaxed);
                status=phase==0?"READING ARCHIVED ELEVATIONS":phase==1?"RESOLVING SURFACE RELIEF":"ASSEMBLING GLOBE";
            } else if((nav.mars()&&!marsReady)||(nav.mercury()&&!mercuryReady)||(nav.moon()&&!moonReady)) status="DATA UNAVAILABLE - SEE TERMINAL";
            bool full=(SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_FULLSCREEN)!=0;
            if(nav.view().kind==solar::ViewKind::Solar||saturnActive()) renderer.prepareSaturn(saturn.clock,saturnLow);
            if(nav.view().kind==solar::ViewKind::Solar||jupiterActive()) renderer.prepareJupiter(jupiter.clock,jupiterLow);
            if(nav.sun()||nav.belt()||nav.view().kind==solar::ViewKind::Solar) renderer.prepareSun(sunClock);
            if(nav.view().kind==solar::ViewKind::Solar||earthActive()) renderer.prepareEarth(earth);
            if(nav.callisto()||nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Jupiter}) renderer.prepareCallisto(callisto);
            if(nav.io()||nav.view()==solar::ViewKey{solar::ViewKind::System,solar::BodyId::Jupiter}) renderer.prepareIo(io);
            for(auto& v:satelliteViews) if(ice::planet(v.body)&&nav.view().focus==v.body&&nav.view().kind!=solar::ViewKind::Solar&&!nav.transitioning()&&renderer.satelliteAvailable(v.body)) v.clock.advance(dt,v.body);
            for(const auto& v:satelliteViews) if((ice::planet(v.body)&&(nav.view().kind==solar::ViewKind::Solar||nav.view().focus==v.body))||(nav.satellite()&&nav.view().focus==v.body)||nav.view()==solar::ViewKey{solar::ViewKind::System,solar::body(v.body).parent}) renderer.prepareSatellite(v);
            if(venusActive()||(nav.view().kind==solar::ViewKind::Solar&&nav.state().approached==solar::BodyId::Venus)) renderer.prepareVenus(venus);
            if(nav.satellite()) {
                renderer.satelliteGlobe(satelliteView());
                if(showHud) renderer.satelliteHud(satelliteView(),mouse.x,mouse.y,fps,full);
            }
            else if(nav.callisto()) {
                renderer.callistoGlobe(callisto);
                if(showHud) renderer.callistoHud(callisto,mouse.x,mouse.y,fps,full);
            }
            else if(nav.io()) {
                renderer.ioGlobe(io);
                if(showHud) renderer.ioHud(io,mouse.x,mouse.y,fps,full);
            }
            else if(nav.venus()) {
                renderer.venusGlobe(venus);
                if(showHud) renderer.venusHud(venus,mouse.x,mouse.y,fps,full,true);
            }
            else if(nav.earth()) {
                renderer.earthGlobe(earth);
                if(showHud) renderer.earthHud(earth,mouse.x,mouse.y,fps,full,true);
            }
            else if(nav.saturn()) {
                renderer.saturnGlobe(saturn);
                if(showHud) renderer.saturnHud(saturn,mouse.x,mouse.y,fps,full,true);
            }
            else if(nav.jupiter()) {
                renderer.jupiterGlobe(jupiter);
                if(showHud) renderer.jupiterHud(jupiter,mouse.x,mouse.y,fps,full,true);
            }
            else if(marsReady) mars.draw(renderer,*loader.field,mouse.x,mouse.y,fps,showHud,full,frames>0);
            else if(moonReady) moon.draw(renderer,*moonLoader.field,mouse.x,mouse.y,fps,showHud,full,frames>0);
            else if(mercuryReady) mercury.draw(renderer,*mercuryLoader.field,mouse.x,mouse.y,fps,showHud,full,frames>0);
            else renderer.placeholders(scene);
            if(showHud||!status.empty()) renderer.navigationHud(nav,scene,mouse.x,mouse.y,full,fps,status,loading);
            if(showHud&&nav.sun()) renderer.sunHud(sunClock,mouse.x,mouse.y);
            if(showHud&&venusActive()&&!nav.venus()) renderer.venusHud(venus,mouse.x,mouse.y,fps,full,false);
            if(showHud&&nav.earthControls()&&!nav.earth()) renderer.earthHud(earth,mouse.x,mouse.y,fps,full,false);
            if(showHud&&nav.view().kind==solar::ViewKind::System&&ice::planet(nav.view().focus)) renderer.outerSystemHud(satelliteViews.at(satellites::slot(nav.view().focus)),mouse.x,mouse.y);
            if(showHud&&saturnActive()&&!nav.saturn()) renderer.saturnHud(saturn,mouse.x,mouse.y,fps,full,false);
            if(showHud&&jupiterActive()&&!nav.jupiter()) renderer.jupiterHud(jupiter,mouse.x,mouse.y,fps,full,false);
            if(showHud) renderer.guideHud(guide,mouse.x,mouse.y,canScan());
            renderer.transition(nav.fade()); renderer.present(); renderer.check(); SDL_GL_SwapWindow(desktop.window);
            ++rendered; ++fpsFrames;
            if(frameStart-fpsStart>=250000000) { fps=float(double(fpsFrames)*1e9/double(frameStart-fpsStart)); fpsFrames=0; fpsStart=frameStart; }
            if((!nav.mars()||loader.status==mars::MarsLoader::Status::Ready||loader.status==mars::MarsLoader::Status::Failed)&&
               (!nav.mercury()||mercuryLoader.status==mercury::Loader::Status::Ready||mercuryLoader.status==mercury::Loader::Status::Failed)&&
               (!nav.moon()||moonLoader.status==moon::Loader::Status::Ready||moonLoader.status==moon::Loader::Status::Failed)) ++settledFrames;
            if(frames>0&&settledFrames>=frames) running=false;
            if(!vsync&&!benchmark) { auto elapsed=SDL_GetTicksNS()-frameStart; if(elapsed<16666667) SDL_DelayNS(16666667-elapsed); }
        }
        if(!screenshot.empty()) renderer.screenshot(screenshot);
        std::cout<<"Final view: "<<int(nav.view().kind)<<" / "<<solar::body(nav.view().focus).name
                 <<", selected "<<solar::body(nav.state().selected).name<<", distance "<<nav.state().camera.distance<<'\n';
        std::cout<<"Mars load status: "<<int(loader.status)<<"; cached triangles: "<<renderer.triangles()
                 <<"; last drawn: "<<renderer.drawnTriangles()<<'\n';
        std::cout<<"Mars state: "<<mars.camera.lat<<" N, "<<mars.camera.lon<<" E, distance "<<mars.camera.distance
                 <<", relief "<<exaggeration<<", satellite "<<orbit.enabled<<", phase "<<orbit.meanAnomaly()
                 <<", weather clock "<<environment.time()<<'\n';
        std::cout<<"Moon load status: "<<int(moonLoader.status)<<"; cached triangles: "<<renderer.triangles(4)
                 <<"; last drawn: "<<renderer.drawnTriangles(4)<<"; LROC appearance: "<<renderer.moonAppearance()<<'\n';
        std::cout<<"Moon state: "<<moon.camera.lat<<" / "<<moon.camera.lon<<" / "<<moon.camera.distance
                 <<"; selected "<<moon.selected<<"; page "<<moon.page<<"; relief "<<moon.exaggeration
                 <<"; height colors "<<moon.heightColors<<'\n';
        std::cout<<"Mercury load status: "<<int(mercuryLoader.status)<<"; cached triangles: "<<renderer.triangles(1)
                 <<"; last drawn: "<<renderer.drawnTriangles(1)<<'\n';
        std::cout<<"Mercury state: "<<mercury.camera.lat<<" N, "<<mercury.camera.lon<<" E, distance "<<mercury.camera.distance
                 <<", relief "<<mercury.exaggeration<<", height colors "<<mercury.heightColors<<'\n';
        for(const auto& v:satelliteViews) std::cout<<solar::body(v.body).name<<": source "<<renderer.satelliteAvailable(v.body)<<", lighting "<<v.lighting<<", selected "<<v.selected<<", camera "<<v.camera.lat<<" / "<<v.camera.lon<<" / "<<v.camera.distance<<'\n';
        std::cout<<"Callisto: source "<<renderer.callistoAvailable()<<", lighting "<<callisto.lighting<<", selected "<<callisto.selected
                 <<", camera "<<callisto.camera.lat<<" / "<<callisto.camera.lon<<" / "<<callisto.camera.distance<<'\n';
        std::cout<<"Io: source "<<renderer.ioAvailable()<<", lighting "<<io.lighting<<", selected "<<io.selected
                 <<", camera "<<io.camera.lat<<" / "<<io.camera.lon<<" / "<<io.camera.distance<<'\n';
        std::cout<<"Venus: topography "<<venus.topography<<", relief "<<venus.exaggeration
                 <<", source "<<renderer.venusAvailable()<<", cloud seconds "<<venus.clouds.seconds<<'\n';
        std::cout<<"Earth: clouds "<<earth.clouds.visible<<", paused "<<earth.clouds.paused<<", seconds "<<earth.clouds.seconds
                 <<", topography "<<earth.topography<<", relief "<<earth.exaggeration<<", source "<<renderer.earthAvailable()<<", camera "<<earth.camera.lat<<" / "<<earth.camera.lon<<" / "<<earth.camera.distance<<'\n';
        std::cout<<"Saturn: source "<<renderer.saturnAvailable()<<", seconds "<<saturn.clock.seconds<<", selected "<<saturn.selected<<", camera "<<saturn.camera.lat<<" / "<<saturn.camera.lon<<" / "<<saturn.camera.distance<<'\n';
        std::cout<<"Jupiter: seconds "<<jupiter.clock.seconds<<", rate "<<jupiter::rates[jupiter.clock.speed]
                 <<", paused "<<jupiter.clock.paused<<", frame "<<int(jupiter.frame)<<", selected "<<jupiter.selected
                 <<", mesh columns "<<renderer.jupiterResolution()<<", source "<<renderer.jupiterAvailable()
                 <<", camera "<<jupiter.camera.lat<<" / "<<jupiter.camera.lon<<" / "<<jupiter.camera.distance<<'\n';
        std::cout<<"Guide: "<<int(guide.body)<<", open "<<guide.open<<", index "<<guide.location<<", scroll "<<guide.scroll
                 <<"; location open "<<guide.locationOpen<<", scroll "<<guide.locationScroll<<'\n';
        std::cout<<"Previews: Mars "<<renderer.triangles(2)<<", Mercury "<<renderer.triangles(3)<<" triangles\n";
        const auto finalOrbit=orbit.elements();
        std::cout<<"Orbit settings: "<<finalOrbit.periapsisKm<<" / "<<finalOrbit.apoapsisKm<<" km, "
                 <<finalOrbit.inclinationDeg<<" deg, time "<<orbit.timeScale<<", paused "<<orbit.paused<<'\n';
        std::cout<<"Drawable: "<<renderer.layout().pixelWidth<<" x "<<renderer.layout().pixelHeight
                 <<", fullscreen "<<((SDL_GetWindowFlags(desktop.window)&SDL_WINDOW_FULLSCREEN)!=0)<<", UI scale "<<uiScale<<'\n';
        double seconds=double(SDL_GetTicksNS()-start)/1e9;
        std::cout<<"Rendered "<<rendered<<" frames in "<<seconds<<" s ("<<rendered/std::max(seconds,.001)<<" FPS)\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"Solar System Sim: "<<e.what()<<"\nRun solar_system_sim --help for controls.\n"; return 1;
    }
}
