#pragma once
#include "navigation.hpp"
#include "ui.hpp"
namespace solar {
struct DisplayBody { BodyId id; mars::Vec3 centre; float radius; };
struct BodyLabel { BodyId id; float x,y,depth,radius; mars::Rect rect; bool shown=false,visible=false; };
struct SceneFrame {
    mars::Camera camera;
    mars::Vec3 renderOrigin{},orbitCentre{};
    solar::BodyId belt=solar::BodyId::None;
    bool surfacePreviews=false;
    bool trueSolarScale=false,trueEarthScale=false,trueMarsScale=false,trueSaturnScale=false,trueOuterScale=false;
    std::vector<DisplayBody> bodies;
    std::vector<float> lanes;
    std::vector<float> laneTilts;
    std::vector<BodyLabel> labels;
};
struct PlaceholderGeometry { std::vector<mars::Vec3> sphere,ring,circle; };
PlaceholderGeometry makePlaceholderGeometry();
SceneFrame makeScene(const Navigation& nav,const mars::ViewLayout& layout);
BodyId pickBody(const SceneFrame& scene,const mars::ViewLayout& layout,float x,float y,bool labels);
std::vector<mars::Rect> crumbRects(const Navigation& nav,const mars::ViewLayout& layout);
}
