#include "venus.hpp"
#include "terrain.hpp"
#include <charconv>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
namespace venus {
namespace {
std::vector<unsigned char> readAsset(const std::filesystem::path& path,const char* expected,int ver,int w,int h,int bytes) {
    std::ifstream input(path,std::ios::binary);
    if(!input) throw std::runtime_error("Cannot open Venus asset: "+path.string());
    std::string magic,hash; int version=0,width=0,height=0;
    if(!(input>>magic>>version>>width>>height>>hash)||magic!=expected||version!=ver||
       width!=w||height!=h||hash.size()!=16||input.get()!='\n')
        throw std::runtime_error("Invalid Venus asset: "+path.string());
    std::uint64_t checksum=0;
    auto parsed=std::from_chars(hash.data(),hash.data()+hash.size(),checksum,16);
    if(parsed.ec!=std::errc{}||parsed.ptr!=hash.data()+hash.size())
        throw std::runtime_error("Invalid Venus asset checksum: "+path.string());
    std::vector<unsigned char> data(std::size_t(w)*h*bytes);
    if(!input.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size()))||input.peek()!=EOF)
        throw std::runtime_error("Venus asset length mismatch: "+path.string());
    std::uint64_t value=14695981039346656037ULL;
    for(auto byte:data) { value^=byte; value*=1099511628211ULL; }
    if(value!=checksum) throw std::runtime_error("Venus asset checksum mismatch: "+path.string());
    return data;
}
}
SurfaceMap SurfaceMap::load(const std::filesystem::path& path) {
    return {readAsset(path,"VENUS_RGB",1,width,height,3)};
}
HeightField HeightField::load(const std::filesystem::path& path) {
    auto bytes=readAsset(path,"VENUS_DEM",1,width,height,2);
    HeightField f; f.meters.resize(width*height);
    for(std::size_t i=0;i<f.meters.size();++i) {
        int raw=int(bytes[2*i])+(int(bytes[2*i+1])<<8),value=raw>=32768?raw-65536:raw;
        if(value!=-32768&&(value<-4000||value>12000)) throw std::runtime_error("Invalid or missing Venus elevation");
        f.meters[i]=std::int16_t(value);
    }
    double north=0,south=0; int nc=0,sc=0;
    for(int x=0;x<width;++x) {
        if(f.meters[x]!=-32768) { north+=f.meters[x]; ++nc; }
        if(f.meters[(height-1)*width+x]!=-32768) { south+=f.meters[(height-1)*width+x]; ++sc; }
    }
    f.north=nc?float(north/nc):std::numeric_limits<float>::quiet_NaN();
    f.south=sc?float(south/sc):std::numeric_limits<float>::quiet_NaN();
    return f;
}
float HeightField::sample(double lat,double lon) const {
    if(meters.empty()||!std::isfinite(lat)||!std::isfinite(lon)) return std::numeric_limits<float>::quiet_NaN();
    double x=mars::wrapLongitude(lon+180)*width/360-.5;
    double y=(90-std::clamp(lat,-90.,90.))*height/180-.5;
    int x0=int(std::floor(x)),x1=(x0+1)%width; x0=(x0+width)%width;
    double tx=x-std::floor(x);
    // A missing neighbour only invalidates a sample when it contributes weight.
    // In particular, NaN * 0 must not erase valid cell centres or the pole mean.
    auto mix=[](float a,float b,double t) {
        if(t<=0) return a;
        if(t>=1) return b;
        return float(a*(1-t)+b*t);
    };
    auto cell=[&](int r,int c) {
        auto h=meters[r*width+c];
        return h==-32768?std::numeric_limits<float>::quiet_NaN():float(h);
    };
    auto row=[&](int r) { return mix(cell(r,x0),cell(r,x1),tx); };
    if(y<0) return mix(north,row(0),(y+.5)*2);
    if(y>height-1) return mix(south,row(height-1),(height-.5-y)*2);
    int y0=int(y),y1=std::min(y0+1,height-1); double ty=y-y0;
    return mix(row(y0),row(y1),ty);
}
std::vector<FacetVertex> makeFacetedGlobe(const HeightField& field,int columns) {
    auto mesh=mars::makeSampledGlobe(columns,[&](double lat,double lon) { float h=field.sample(lat,lon); return std::isfinite(h)?h:-32768.f; });
    std::vector<FacetVertex> result; result.reserve(mesh.indices.size());
    for(std::size_t i=0;i<mesh.indices.size();i+=3) {
        float h=0; bool valid=true;
        for(int j=0;j<3;++j) { float v=mesh.vertices[mesh.indices[i+j]].meters; valid=valid&&v!=-32768; h+=v/3; }
        for(int j=0;j<3;++j) {
            auto& v=mesh.vertices[mesh.indices[i+j]];
            result.push_back({v.direction,v.meters==-32768?0:v.meters,valid?h:-32768.f});
        }
    }
    return result;
}
}
