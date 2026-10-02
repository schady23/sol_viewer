#include "preview.hpp"
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>
namespace solar {
namespace {
const char* name(PreviewBody b) { return b==PreviewBody::Moon?"MOON":b==PreviewBody::Mercury?"MERCURY":"MARS"; }
int radius(PreviewBody b) { return b==PreviewBody::Moon?1737400:b==PreviewBody::Mercury?2439400:3396000; }
float minimum(PreviewBody b) { return b==PreviewBody::Moon?-9125.5f:b==PreviewBody::Mercury?-5382:-8177; }
float maximum(PreviewBody b) { return b==PreviewBody::Moon?10773:b==PreviewBody::Mercury?4497:21171; }
}

void writePreview(const std::filesystem::path& path,PreviewBody b,const std::function<float(double,double)>& sample) {
    if(std::filesystem::exists(path)) throw std::runtime_error("Preview output exists; choose another path");
    std::ofstream out(path); out.imbue(std::locale::classic());
    out<<"SOLAR_PREVIEW 1 "<<name(b)<<" 256 128 "<<radius(b)<<'\n'<<std::fixed<<std::setprecision(1);
    for(int y=0;y<previewRows;++y) {
        for(int x=0;x<previewColumns;++x) out<<sample(90-(y+.5)*180/previewRows,(x+.5)*360/previewColumns)<<' ';
        out<<'\n';
    }
    out.close(); if(!out) throw std::runtime_error("Cannot write preview: "+path.string());
}
mars::Mesh loadPreview(const std::filesystem::path& path,PreviewBody b) {
    std::ifstream in(path); in.imbue(std::locale::classic());
    std::string magic,body; int version=0,w=0,h=0,r=0;
    if(!(in>>magic>>version>>body>>w>>h>>r)||magic!="SOLAR_PREVIEW"||version!=1||
       body!=name(b)||w!=previewColumns||h!=previewRows||r!=radius(b))
        throw std::runtime_error("Invalid or absent surface preview: "+path.string());
    std::vector<float> values(std::size_t(w)*h);
    for(float& value:values) if(!(in>>value)||!std::isfinite(value)||value<minimum(b)||value>maximum(b))
        throw std::runtime_error("Invalid preview elevation: "+path.string());
    std::string extra; if(in>>extra) throw std::runtime_error("Unexpected preview data");
    float north=0,south=0;
    for(int x=0;x<w;++x) { north+=values[x]/w; south+=values[std::size_t(h-1)*w+x]/w; }
    return mars::makeSampledGlobe(w,[&](double lat,double lon) {
        if(lat==90) return north;
        if(lat== -90) return south;
        int y=int(std::llround((90-lat)*h/180-.5)),x=int(std::llround(lon*w/360-.5));
        return values.at(std::size_t(y)*w+x);
    });
}
}
