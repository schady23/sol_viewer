#include "callisto.hpp"
namespace callisto {
Image Image::load(const std::filesystem::path& path) {
    return {faceted::loadRgb(path,"CALLISTO_RGB","Callisto")};
}
std::vector<FacetVertex> makeFacetedGlobe(const Image& image,int subdivisions) {
    return faceted::makeFacetedGlobe(image.rgb,subdivisions);
}
}
