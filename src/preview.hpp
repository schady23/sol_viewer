#pragma once
#include "terrain.hpp"
namespace solar {
enum class PreviewBody { Mars, Mercury, Moon };
// Small, bundled display grids. Scientific readouts always use the full source datasets.
constexpr int previewColumns=256,previewRows=128;
void writePreview(const std::filesystem::path& path,PreviewBody body,const std::function<float(double,double)>& sample);
mars::Mesh loadPreview(const std::filesystem::path& path,PreviewBody body);
inline void writePreview(const std::filesystem::path& p,bool mercury,const std::function<float(double,double)>& sample) {
    writePreview(p,mercury?PreviewBody::Mercury:PreviewBody::Mars,sample);
}
inline mars::Mesh loadPreview(const std::filesystem::path& p,bool mercury) {
    return loadPreview(p,mercury?PreviewBody::Mercury:PreviewBody::Mars);
}
}
