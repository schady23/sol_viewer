#include "moon.hpp"
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=4) { std::cerr<<"Usage: moon_convert ldem_64.lbl ldem_64.img output.dem\n"; return 2; }
    try {
        auto output=std::filesystem::absolute(argv[3]).lexically_normal();
        if(std::filesystem::exists(output)) throw std::runtime_error("Output already exists; choose a new output path");
        auto field=moon::HeightField::importPds(argv[1],argv[2]);
        field.save(output);
        std::cout<<"Converted LOLA LDEM_64 V3.1: 23040 x 11520, signed little-endian half-metres; "
                 <<"No missing values declared. Datum: 1737.4 km sphere.\n";
        for(auto p:moon::landmarks) std::cout<<p.name<<": "<<field.sample(p.lat,p.lon)<<" m (interpolated)\n";
    } catch(const std::exception& e) { std::cerr<<"moon_convert: "<<e.what()<<'\n'; return 1; }
}
