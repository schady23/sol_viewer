#include "faceted.hpp"
#include <charconv>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

namespace faceted {
std::vector<unsigned char> loadRgb(const std::filesystem::path& path,const std::string& expectedMagic,const std::string& label,int mapWidth,int mapHeight) {
    if(mapWidth<1||mapHeight<1||mapWidth>8192||mapHeight>4096) throw std::runtime_error("Invalid atlas dimensions");
    std::ifstream input(path,std::ios::binary);
    if(!input) throw std::runtime_error("Cannot open "+label+" map: "+path.string());
    std::string magic,hash; int version=0,w=0,h=0;
    if(!(input>>magic>>version>>w>>h>>hash)||magic!=expectedMagic||version!=1||
       w!=mapWidth||h!=mapHeight||hash.size()!=16||input.get()!='\n')
        throw std::runtime_error("Invalid "+label+" map header: "+path.string());
    std::uint64_t expected=0;
    auto parsed=std::from_chars(hash.data(),hash.data()+hash.size(),expected,16);
    if(parsed.ec!=std::errc{}||parsed.ptr!=hash.data()+hash.size())
        throw std::runtime_error("Invalid "+label+" map checksum: "+path.string());
    std::vector<unsigned char> rgb(std::size_t(mapWidth)*mapHeight*3);
    if(!input.read(reinterpret_cast<char*>(rgb.data()),std::streamsize(rgb.size()))||input.peek()!=EOF)
        throw std::runtime_error(label+" map length mismatch: "+path.string());
    std::uint64_t checksum=14695981039346656037ULL;
    for(auto byte:rgb) { checksum^=byte; checksum*=1099511628211ULL; }
    if(checksum!=expected) throw std::runtime_error(label+" map checksum mismatch: "+path.string());
    return rgb;
}
std::vector<FacetVertex> makeFacetedGlobe(const std::vector<unsigned char>& rgb,int subdivisions) {
    if(rgb.size()!=std::size_t(width)*height*3||subdivisions<0||subdivisions>8)
        throw std::runtime_error("Invalid facet map or subdivision level");
    float t=float((1+std::sqrt(5.))/2);
    std::array<mars::Vec3,12> points{{{-1,t,0},{1,t,0},{-1,-t,0},{1,-t,0},
        {0,-1,t},{0,1,t},{0,-1,-t},{0,1,-t},{t,0,-1},{t,0,1},{-t,0,-1},{-t,0,1}}};
    for(auto& p:points) p=mars::normalized(p);
    constexpr int faces[20][3]={{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},{3,9,4},{3,4,2},{3,2,6},
        {3,6,8},{3,8,9},{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}};
    std::vector<FacetVertex> mesh;
    mesh.reserve(std::size_t(60)<<(2*subdivisions));
    auto triangle=[&](auto&& self,mars::Vec3 a,mars::Vec3 b,mars::Vec3 c,int level,std::uint32_t seed)->void {
        if(level>0) {
            auto ab=mars::normalized(a+b),bc=mars::normalized(b+c),ca=mars::normalized(c+a);
            self(self,a,ab,ca,level-1,seed*4); self(self,ab,b,bc,level-1,seed*4+1);
            self(self,ca,bc,c,level-1,seed*4+2); self(self,ab,bc,ca,level-1,seed*4+3);
            return;
        }
        // Source appearance belongs to the face, not interpolated screen pixels.
        // Sample a spherical centroid so both the dateline and poles stay local.
        auto where=mars::location(a+b+c);
        double x=mars::wrapLongitude(where.lon+180)*width/360-.5;
        double y=std::clamp((90-where.lat)*height/180-.5,0.,double(height-1));
        int left=int(std::floor(x)),top=int(y),bottom=std::min(top+1,height-1);
        double tx=x-std::floor(x),ty=y-top;
        auto sample=[&](int row,int col,int channel) { col=(col+width)%width; return rgb[(row*width+col)*3+channel]/255.; };
        seed^=seed>>16; seed*=0x7feb352du; seed^=seed>>15; seed*=0x846ca68bu; seed^=seed>>16;
        // A restrained, fixed per-face value variation makes the dense tessellation
        // legible. This is color styling only, never inferred elevation.
        double variation=.95+.10*double(seed&255)/255;
        std::array<unsigned char,4> color{{0,0,0,255}};
        for(int channel=0;channel<3;++channel) {
            double north=sample(top,left,channel)*(1-tx)+sample(top,left+1,channel)*tx;
            double south=sample(bottom,left,channel)*(1-tx)+sample(bottom,left+1,channel)*tx;
            double value=std::clamp((north*(1-ty)+south*ty)*variation,0.,1.);
            // DS-inspired 5-bit channels, with more geometric detail than the
            // original hardware. Preserve the supplied map palette.
            color[channel]=static_cast<unsigned char>(std::lround(std::round(value*31)/31*255));
        }
        for(auto p:{a,b,c}) mesh.push_back({p,color});
    };
    for(int i=0;i<20;++i) triangle(triangle,points[faces[i][0]],points[faces[i][1]],points[faces[i][2]],subdivisions,std::uint32_t(i+1));
    return mesh;
}
}
