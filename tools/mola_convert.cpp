#include "terrain.hpp"
#include <algorithm>
#include <iostream>

int main(int argc,char** argv) {
    if(argc!=4) { std::cerr<<"Usage: mola_convert product.lbl product.img output.mola\n"; return 2; }
    try {
        auto label=std::filesystem::absolute(argv[1]).lexically_normal();
        auto input=std::filesystem::absolute(argv[2]).lexically_normal();
        auto output=std::filesystem::absolute(argv[3]).lexically_normal();
        if(output==input || output==label || std::filesystem::exists(output))
            throw std::runtime_error("Output already exists or aliases an input; choose a new output path");
        auto field=mars::HeightField::importMola(label,input);
        field.save(output);
        auto bounds=std::minmax_element(field.samples().begin(),field.samples().end());
        std::cout<<"Converted MEGT90N000EB v2.0: 5760 x 2880 signed metres, "
                 <<*bounds.first<<" to "<<*bounds.second<<" m\n";
        for(auto p:mars::landmarks) std::cout<<p.name<<": "<<field.sample(p.lat,p.lon)<<" m (interpolated)\n";
    } catch(const std::exception& e) { std::cerr<<"mola_convert: "<<e.what()<<'\n'; return 1; }
}
