#include "earth.hpp"
#include "terrain.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
namespace earth {
mars::Mesh makeGlobe(const HeightField& field,int columns) {
    if(field.meters.size()!=std::size_t(HeightField::width)*HeightField::height)
        throw std::runtime_error("Invalid Earth elevation grid");
    return mars::makeSampledGlobe(columns,[&](double lat,double lon) { return field.sample(lat,lon); });
}
void Clouds::advance(double dt) {
    if(!paused&&std::isfinite(dt)&&dt>0) seconds=std::fmod(seconds+std::fmod(dt,loopSeconds),loopSeconds);
}
double Clouds::fraction() const { return std::fmod(seconds,loopSeconds)/loopSeconds; }
mars::Vec3 Clouds::phase() const {
    double angle=fraction()*2*mars::pi;
    return {float(std::cos(angle)),float(std::sin(angle)),0};
}
std::vector<unsigned char> cloudNoise() {
    constexpr int size=128,period=16;
    auto hash=[](int x,int y,int z) {
        std::uint32_t h=std::uint32_t(x)*0x8da6b343u^std::uint32_t(y)*0xd8163841u^std::uint32_t(z)*0xcb1ab31fu;
        h^=h>>16; h*=0x7feb352du; h^=h>>15; h*=0x846ca68bu; return h^(h>>16);
    };
    auto gradient=[&](int x,int y,int z,float dx,float dy,float dz) {
        unsigned h=hash(x&(period-1),y&(period-1),z&(period-1))&15;
        float u=h<8?dx:dy,v=h<4?dy:(h==12||h==14?dx:dz);
        return ((h&1)?-u:u)+((h&2)?-v:v);
    };
    auto fade=[](float t) { return t*t*t*(t*(t*6-15)+10); };
    auto mix=[](float a,float b,float t) { return a+(b-a)*t; };
    std::vector<unsigned char> result(size*size*size);
    for(int z=0;z<size;++z) for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
        float px=float(x)/8,py=float(y)/8,pz=float(z)/8;
        int ix=int(px),iy=int(py),iz=int(pz); float fx=px-ix,fy=py-iy,fz=pz-iz;
        float u=fade(fx),v=fade(fy),w=fade(fz),plane[2]{};
        for(int k=0;k<2;++k) {
            float row[2]{};
            for(int j=0;j<2;++j) row[j]=mix(gradient(ix,iy+j,iz+k,fx,fy-j,fz-k),
                                                          gradient(ix+1,iy+j,iz+k,fx-1,fy-j,fz-k),u);
            plane[k]=mix(row[0],row[1],v);
        }
        result[(z*size+y)*size+x]=static_cast<unsigned char>(std::clamp(128+mix(plane[0],plane[1],w)*145,0.f,255.f));
    }
    return result;
}
namespace {
std::vector<unsigned char> readAsset(const std::filesystem::path& path,const char* expected,int ver,int w,int h,int bytes) {
    std::ifstream input(path,std::ios::binary);
    std::string magic,hash; int version=0,width=0,height=0;
    if(!(input>>magic>>version>>width>>height>>hash)||magic!=expected||version!=ver||
       width!=w||height!=h||hash.size()!=16||input.get()!='\n')
        throw std::runtime_error("Invalid Earth asset: "+path.string());
    std::vector<unsigned char> data(std::size_t(w)*h*bytes);
    if(!input.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size()))||input.peek()!=EOF)
        throw std::runtime_error("Earth asset length mismatch: "+path.string());
    std::uint64_t value=14695981039346656037ULL;
    for(auto byte:data) { value^=byte; value*=1099511628211ULL; }
    std::size_t used=0; auto checksum=std::stoull(hash,&used,16);
    if(used!=hash.size()||value!=checksum) throw std::runtime_error("Earth asset checksum mismatch: "+path.string());
    return data;
}
}
Image Image::load(const std::filesystem::path& path) {
    return {4096,2048,readAsset(path,"EARTH_RGB",2,4096,2048,3)};
}
Image Image::loadClouds(const std::filesystem::path& path) {
    return {2048,1024,readAsset(path,"EARTH_CLOUDS",1,2048,1024,1)};
}
HeightField HeightField::load(const std::filesystem::path& path) {
    auto bytes=readAsset(path,"EARTH_DEM",1,width,height,2);
    HeightField f; f.meters.resize(width*height);
    for(std::size_t i=0;i<f.meters.size();++i) {
        int raw=int(bytes[2*i])+(int(bytes[2*i+1])<<8),value=raw>=32768?raw-65536:raw;
        if(value<-12000||value>9000) throw std::runtime_error("Invalid or missing Earth elevation");
        f.meters[i]=std::int16_t(value);
    }
    double north=0,south=0;
    for(int x=0;x<width;++x) { north+=f.meters[x]; south+=f.meters[(height-1)*width+x]; }
    f.north=float(north/width); f.south=float(south/width);
    return f;
}
float HeightField::sample(double lat,double lon) const {
    if(meters.empty()||!std::isfinite(lat)||!std::isfinite(lon)) return std::numeric_limits<float>::quiet_NaN();
    double x=mars::wrapLongitude(lon+180)*width/360-.5;
    double y=(90-std::clamp(lat,-90.,90.))*height/180-.5;
    int x0=int(std::floor(x)),x1=(x0+1)%width; x0=(x0+width)%width;
    double tx=x-std::floor(x);
    auto row=[&](int r) { return float(meters[r*width+x0]*(1-tx)+meters[r*width+x1]*tx); };
    if(y<0) return float(north+(row(0)-north)*(y+.5)*2);
    if(y>height-1) return float(south+(row(height-1)-south)*(height-.5-y)*2);
    int y0=int(y),y1=std::min(y0+1,height-1); double ty=y-y0;
    return float(row(y0)*(1-ty)+row(y1)*ty);
}
}
