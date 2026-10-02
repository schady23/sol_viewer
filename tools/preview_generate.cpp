#include "preview.hpp"
#include "mercury.hpp"
#include "moon.hpp"
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=4) { std::cerr<<"Usage: preview_generate mars|mercury|moon source.mola|dem output.preview\n"; return 2; }
    try {
        std::string body=argv[1];
        if(body=="mars") {
            auto f=mars::HeightField::load(argv[2]); solar::writePreview(argv[3],false,[&](double lat,double lon) { return f.sample(lat,lon); });
        } else if(body=="mercury") {
            auto f=mercury::HeightField::load(argv[2]); solar::writePreview(argv[3],true,[&](double lat,double lon) { return f.sample(lat,lon); });
        } else if(body=="moon") {
            auto f=moon::HeightField::load(argv[2]); solar::writePreview(argv[3],solar::PreviewBody::Moon,[&](double lat,double lon) { return f.sample(lat,lon); });
        } else throw std::runtime_error("Unknown preview body");
        auto mesh=solar::loadPreview(argv[3],body=="moon"?solar::PreviewBody::Moon:body=="mercury"?solar::PreviewBody::Mercury:solar::PreviewBody::Mars);
        std::cout<<body<<": 256 x 128 source-derived preview; "<<mesh.indices.size()/3<<" triangles\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
