#include "io.hpp"
namespace io {
Image Image::load(const std::filesystem::path& path) {
    return {faceted::loadRgb(path,"IO_RGB","Io")};
}
std::vector<FacetVertex> makeFacetedGlobe(const Image& image,int subdivisions) {
    return faceted::makeFacetedGlobe(image.rgb,subdivisions);
}
}
