#pragma once
#include "gl.hpp"
#include "sun.hpp"
#include "terrain.hpp"
#include "environment.hpp"
#include "orbit.hpp"
#include "ui.hpp"
#include "placeholders.hpp"
#include "mercury.hpp"
#include "moon.hpp"
#include "guide.hpp"
#include "jupiter_view.hpp"
#include "saturn_view.hpp"
#include "earth_view.hpp"
#include "venus_view.hpp"
#include "io_view.hpp"
#include "callisto_view.hpp"
#include "satellites_view.hpp"
#include <optional>
#include <string>

namespace mars {
struct Probe { Location location; float meters{}; bool underPointer{},subsatellite{}; };
class Renderer {
public:
    Renderer();
    void uploadTerrain(const Mesh& mesh,int slot=0);
    void clearTerrain(int slot=0);
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    void resize(int pixelWidth,int pixelHeight);
    void uiScale(float requested) { layout_.resize(layout_.pixelWidth,layout_.pixelHeight,requested); }
    void toggleEnvironment() { layout_.environmentOpen=!layout_.environmentOpen; }
    void satelliteMode(bool active) { layout_.satellite=active; }
    const ViewLayout& layout() const { return layout_; }
    int triangles(int slot=0) const { return terrain_.at(slot).indexCount/3; }
    int drawnTriangles(int slot=0) const { return terrain_.at(slot).drawnTriangles; }
    void culling(bool enabled) { culling_=enabled; }
    void terrain(const Camera& camera,float exaggeration,const Environment& environment,int slot=0,bool heightColors=false);
    void mercuryHud(const Camera& camera,const mercury::HeightField& field,const Probe& probe,float exaggeration,
                    int selected,float mouseX,float mouseY,float fps,bool fullscreen,bool heightColors,int meshPpd);
    void moonHud(const Camera& camera,const moon::HeightField& field,const Probe& probe,float exaggeration,
                 int selected,int page,float mouseX,float mouseY,float fps,bool fullscreen,bool heightColors,int meshPpd);
    bool moonAppearance() const { return moonSource_!=0; }
    void uploadMoonSlopes(const std::vector<float>& slopes);
    void beltMode(bool active) { layout_.beltView=active; }
    void outerMode(bool system,bool globe) { layout_.outerSystem=system; layout_.iceView=globe; }
    void saturnSystemMode(bool active) { layout_.saturnSystem=active; }
    void titanMode(bool active) { layout_.titanView=active; }
    void marsSystemMode(bool active) { layout_.marsSystem=active; }
    void moonMode(bool active) { layout_.moonView=active; }
    void weather(const Camera& camera,float exaggeration,const Environment& environment);
    std::optional<Location> pick(float x,float y,const Camera& camera);
    void hud(const Camera& camera,const HeightField& field,const Probe& probe,
             float exaggeration,int selected,float mouseX,float mouseY,float fps,bool fullscreen,const Environment& environment,
             const SatelliteOrbit& orbit,const NumberEdit& edit);
    void explorerMode(bool mars,int rows=9,bool mercury=false,bool solar=false,bool system=false) { layout_.systemView=system; layout_.marsView=mars; layout_.mercuryView=mercury; layout_.bodyRows=rows; layout_.solarView=solar; }
    void guideMode(bool available,bool open,bool locationAvailable,bool locationOpen) {
        layout_.guideAvailable=available; layout_.guideOpen=open;
        layout_.locationAvailable=locationAvailable; layout_.locationOpen=locationOpen;
    }
    void guideHud(const solar::Guide& guide,float mouseX,float mouseY,bool scanAvailable);
    void satelliteModeGlobe(bool globe) { layout_.satelliteView=globe; }
    void prepareSatellite(const satellites::View& view);
    void satelliteGlobe(const satellites::View& view);
    void satelliteHud(const satellites::View& view,float mx,float my,float fps,bool fullscreen);
    void outerSystemHud(const satellites::View& view,float mx,float my);
    bool satelliteAvailable(solar::BodyId id) const;
    void callistoMode(bool globe) { layout_.callistoView=globe; }
    void prepareCallisto(const callisto::View& view);
    void callistoGlobe(const callisto::View& view);
    void callistoHud(const callisto::View& view,float mx,float my,float fps,bool fullscreen);
    bool callistoAvailable() const { return callistoVao_!=0; }
    void ioMode(bool globe) { layout_.ioView=globe; }
    void prepareIo(const io::View& view);
    void ioGlobe(const io::View& view);
    void ioHud(const io::View& view,float mx,float my,float fps,bool fullscreen);
    bool ioAvailable() const { return ioVao_!=0; }
    void venusMode(bool globe,bool controls) { layout_.venusView=globe; layout_.venusControls=controls; }
    void prepareVenus(const venus::View& view);
    void venusGlobe(const venus::View& view);
    void venusHud(const venus::View& view,float mx,float my,float fps,bool fullscreen,bool globe);
    bool venusAvailable() const { return venusProgram_!=0&&(venusTopography_?venusVao_!=0:venusCloudVao_!=0); }
    void earthMode(bool globe,bool controls) { layout_.earthView=globe; layout_.earthControls=controls; }
    void prepareEarth(const earth::View& view);
    void earthGlobe(const earth::View& view);
    void earthHud(const earth::View& view,float mx,float my,float fps,bool fullscreen,bool globe);
    bool earthAvailable() const { return earthVao_!=0; }
    void saturnMode(bool globe,bool controls) { layout_.saturnView=globe; layout_.saturnControls=controls; }
    void prepareSaturn(const saturn::Clock& clock,bool lowDetail);
    void saturnGlobe(const saturn::View& view);
    void saturnHud(const saturn::View& view,float mx,float my,float fps,bool fullscreen,bool globe);
    bool saturnAvailable() const { return saturnSource_!=0; }
    int saturnResolution() const { return saturnSize_; }
    void jupiterMode(bool globe,bool controls) { layout_.jupiterView=globe; layout_.jupiterControls=controls; }
    void sunMode(bool globe) { layout_.sunView=globe; }
    void prepareSun(const sun::Clock& clock);
    void sunHud(const sun::Clock& clock,float mx,float my);
    void prepareJupiter(const jupiter::Clock& clock,bool lowDetail);
    void jupiterGlobe(const jupiter::View& view);
    void jupiterHud(const jupiter::View& view,float mx,float my,float fps,bool fullscreen,bool globe);
    bool jupiterAvailable() const { return jupiterSource_!=0; }
    int jupiterResolution() const { return jupiterSize_; }
    const std::string& jupiterError() const { return jupiterError_; }
    void scan(float phase,float strength) { scanPhase_=phase; scanStrength_=strength; }
    void placeholders(const solar::SceneFrame& scene);
    bool previewAvailable(solar::BodyId body) const;
    void navigationHud(const solar::Navigation& nav,const solar::SceneFrame& scene,float mouseX,float mouseY,
                       bool fullscreen,float fps,const std::string& status={},bool loading=false);
    void transition(float alpha);
    void present();
    void screenshot(const std::string& path);
    std::string device() const;
    void check() const;
private:
    struct UiVertex { float x,y; Vec3 color; float alpha; };
    ViewLayout layout_;
    GL gl_;
    GLuint spaceProgram_=0,spaceVao_=0;
    void spaceBackground(const Camera& camera);
    GLuint terrainProgram_=0,uiProgram_=0,weatherProgram_=0;
    struct GpuBelt { GLuint vao=0,vbo=0; GLsizei points=0,lines=0,ringFirst=0,ringCount=0; };
    std::array<GpuBelt,2> belts_{};
    GLuint beltProgram_=0;
    void prepareBelt(int slot);
    void beltCamera(const Camera& camera,Vec3 origin,float scale);
    void drawBelts(const solar::SceneFrame& scene);
    void drawHaumeaRing(const Camera& camera,Vec3 centre,float radius);
    GLuint bodyProgram_=0,bodyVao_=0,bodyVbo_=0;
    GLsizei sphereCount_=0,ringCount_=0,circleCount_=0;
    GLuint uiVao_=0,uiVbo_=0,fbo_=0,color_=0,depth_=0;
    struct GpuTerrain {
        GLuint vao=0,vbo=0,indices=0;
        GLsizei indexCount=0;
        std::vector<TerrainBatch> batches;
        std::vector<GLsizei> drawCounts;
        std::vector<const void*> drawOffsets;
        float chordFactor=1;
        int drawnTriangles=0;
    };
    std::array<GpuTerrain,6> terrain_{}; // 0/1 Mars/Mercury detail; 2/3 previews; 4 Moon detail; 5 Moon preview.
    GLuint moonSource_=0,moonSlopes_=0;
    void initMoonColor();
    bool culling_=true;
    struct GpuSatellite {
        GLuint vao=0,vbo=0,ringVao=0,ringVbo=0,cloudMap=0,noise=0;
        GLsizei count=0,ringCount=0;
        float spin=0,phase=0;
        bool attempted=false;
        unsigned revision=0;
        bool haze=true;
        std::string error;
    };
    std::array<GpuSatellite,satellites::count> satellites_{};
    GLuint satelliteProgram_=0;
    void clearSatellite(GpuSatellite& surface);
    void drawOuterRings(solar::BodyId body,const Camera& camera,Vec3 centre,float radius);
    void drawSatellite(solar::BodyId body,const Camera& camera,Vec3 centre,float radius,float spin=0,bool lighting=true);
    GLuint callistoProgram_=0,callistoVao_=0,callistoVbo_=0;
    GLsizei callistoVertexCount_=0;
    bool callistoAttempted_=false;
    unsigned callistoRevision_=0;
    std::string callistoError_;
    void initCallisto();
    void clearCallisto();
    void drawCallisto(const Camera& camera,Vec3 centre,float radius,float spin=0,bool lighting=true);
    GLuint ioProgram_=0,ioVao_=0,ioVbo_=0;
    GLsizei ioVertexCount_=0;
    bool ioAttempted_=false;
    unsigned ioRevision_=0;
    std::string ioError_;
    void initIo();
    void clearIo();
    void drawIo(const Camera& camera,Vec3 centre,float radius,float spin=0,bool lighting=true);
    float scanPhase_=0,scanStrength_=0;
    GLuint saturnProgram_=0,saturnSource_=0,saturnWind_=0,saturnNoise_=0;
    GLuint saturnVao_=0,saturnVbo_=0,saturnIbo_=0,saturnRingProgram_=0,saturnRingMap_=0,saturnRingVao_=0,saturnRingVbo_=0;
    GLsizei saturnIndexCount_=0,saturnRingCount_=0;
    int saturnSize_=0;
    unsigned saturnRevision_=0;
    bool saturnAttempted_=false;
    float saturnPhase_=0,saturnRotation_=0;
    std::string saturnError_;
    void initSaturn(bool lowDetail);
    void clearSaturn();
    void drawSaturn(const Camera& camera,Vec3 centre,float radius,float spin);
    GLuint jupiterProgram_=0,jupiterSource_=0,jupiterWind_=0,jupiterNoise_=0;
    GLuint jupiterVao_=0,jupiterVbo_=0,jupiterIbo_=0;
    GLsizei jupiterIndexCount_=0;
    int jupiterSize_=0;
    unsigned jupiterRevision_=0;
    bool jupiterAttempted_=false;
    float jupiterPhase_=0,jupiterRotation_=0;
    std::string jupiterError_;
    GLuint venusProgram_=0,venusNoise_=0,venusSurface_=0,venusVao_=0,venusVbo_=0;
    GLuint venusCloudVao_=0,venusCloudVbo_=0,venusCloudIbo_=0;
    GLsizei venusCloudIndexCount_=0;
    GLsizei venusVertexCount_=0;
    bool venusAttempted_=false,venusSurfaceAttempted_=false,venusTopography_=false;
    float venusExaggeration_=10;
    venus::HeightField venusField_;
    unsigned venusRevision_=0;
    Vec3 venusPhase_{1,0,0};
    std::string venusError_,venusSurfaceError_;
    void initVenus();
    void initVenusSurface();
    void clearVenus();
    void clearVenusSurface();
    void drawVenus(const Camera& camera,Vec3 centre,float radius,bool physicalScale=false);
    GLuint earthProgram_=0,earthNoise_=0,earthCloudMap_=0,earthVao_=0,earthVbo_=0,earthIbo_=0,earthSurface_=0,earthElevation_=0;
    GLuint earthCloudVao_=0,earthCloudVbo_=0,earthCloudIbo_=0;
    GLsizei earthVertexCount_=0,earthCloudIndexCount_=0;
    bool earthAttempted_=false,earthClouds_=true,earthTopography_=false;
    float earthExaggeration_=10;
    earth::HeightField earthField_;
    unsigned earthRevision_=0;
    Vec3 earthPhase_{1,0,0};
    std::string earthError_;
    void initEarth();
    void clearEarth();
    void drawEarth(const Camera& camera,Vec3 centre,float radius,bool physicalScale=false);
    GLuint sunProgram_=0,sunVao_=0,sunNoise_=0;
    float sunPhase_=0;
    void clearSun();
    void drawSun(const Camera& camera,Vec3 centre,float radius);
    std::vector<UiVertex> ui_;
    GLuint program(const char* vertex,const char* fragment);
    void initPlaceholders();
    void initPreviews();
    void initJupiter(bool lowDetail);
    void clearJupiter();
    void drawJupiter(const Camera& camera,Vec3 centre,float radius,float spin);
    void drawTerrain(const Camera& camera,float exaggeration,const Environment& environment,int slot,bool heightColors,
                     Vec3 centre={},float displayRadius=1);
    void flushUi();
    void rect(float x,float y,float w,float h,Vec3 color,float alpha=1);
    void text(int x,int y,const std::string& text,Vec3 color,float scale=1);
    void buttonLabel(int x,int y,const std::string& label,Vec3 color,bool enabled=true);
    void border(Rect r,Vec3 color,float alpha=1);
    void crosshair(int x,int y,Vec3 color);
};
} // namespace mars
