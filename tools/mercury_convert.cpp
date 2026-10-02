#include "mercury.hpp"
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=4) { std::cerr<<"Usage: mercury_convert V02.LBL V02.IMG output.dem\n"; return 2; }
    try {
        auto output=std::filesystem::absolute(argv[3]).lexically_normal();
        if(std::filesystem::exists(output)) throw std::runtime_error("Output already exists; choose a new output path");
        auto field=mercury::HeightField::importPds(argv[1],argv[2]);
        field.save(output);
        std::cout<<"Converted MSGR_DEM_USG_SC_I_V02: 23040 x 11520, signed little-endian half-metres; "
                 <<field.missingCount()<<" missing samples. Datum: 2439.4 km sphere.\n";
        for(auto p:mercury::landmarks) std::cout<<p.name<<": "<<field.sample(p.lat,p.lon)<<" m (interpolated)\n";
    } catch(const std::exception& e) { std::cerr<<"mercury_convert: "<<e.what()<<'\n'; return 1; }
}
