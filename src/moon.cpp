#include "moon.hpp"
#include <array>
#include <fstream>
#include <map>
#include <regex>
#include <stdexcept>
namespace moon {
namespace {
constexpr std::size_t sampleCount=std::size_t(360*sourcePpd)*(180*sourcePpd);
constexpr float absent=std::numeric_limits<float>::quiet_NaN();
float meters(std::int16_t dn) { return float(dn)*.5f; }
float mix(float a,float b,double f) {
    // Snap negligible interpolation weights to exact sample centres.
    if(f<=1e-10) return a;
    if(f>=1-1e-10) return b;
    return float(a*(1-f)+b*f);
}
void put32(std::uint8_t* p,std::uint32_t n) { for(int i=0;i<4;++i) p[i]=std::uint8_t(n>>(8*i)); }
std::uint32_t get32(const std::uint8_t* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
void hashByte(std::uint32_t& hash,std::uint8_t b) { hash=(hash^b)*16777619u; }
std::vector<std::int16_t> readSamples(const std::filesystem::path& path,bool prepared) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f) throw std::runtime_error("Cannot open "+path.string());
    if(f.tellg()!=std::streamoff(2*sampleCount+(prepared?32:0))) throw std::runtime_error("Wrong Moon data file size");
    f.seekg(0); std::uint32_t expected=0;
    if(prepared) {
        std::array<std::uint8_t,32> h{};
        f.read(reinterpret_cast<char*>(h.data()),h.size());
        if(std::string(h.begin(),h.begin()+8)!="MOONDEM1" || get32(h.data()+8)!=1 ||
           get32(h.data()+12)!=23040 || get32(h.data()+16)!=11520 || get32(h.data()+20)!=64 || get32(h.data()+28)!=0)
            throw std::runtime_error("Unsupported Moon data header; reconvert LOLA V3.1");
        expected=get32(h.data()+24);
    }
    std::vector<std::int16_t> samples(sampleCount);
    std::array<std::uint8_t,65536> buffer{};
    std::uint32_t hash=2166136261u;
    for(std::size_t offset=0;offset<sampleCount;) {
        auto count=std::min(buffer.size()/2,sampleCount-offset);
        if(!f.read(reinterpret_cast<char*>(buffer.data()),std::streamsize(count*2))) throw std::runtime_error("Moon data read failed");
        for(std::size_t i=0;i<count;++i) {
            auto lo=buffer[2*i],hi=buffer[2*i+1];
            samples[offset+i]=mars::decodeSigned16(hi,lo);
            hashByte(hash,lo); hashByte(hash,hi);
        }
        offset+=count;
    }
    if(prepared&&hash!=expected) throw std::runtime_error("Moon checksum mismatch; reconvert source");
    return samples;
}
}
void validateLabel(const std::string& text) {
    std::map<std::string,std::string> values;
    std::regex assignment(R"((?:^|\n)[ \t]*([\^A-Z_]+)[ \t]*=[ \t]*([^\r\n]*))");
    for(auto i=std::sregex_iterator(text.begin(),text.end(),assignment);i!=std::sregex_iterator();++i) {
        std::string key=(*i)[1],value=(*i)[2];
        if(key=="OBJECT"||key=="END_OBJECT"||key=="DESCRIPTION") continue;
        value.erase(value.find_last_not_of(" \t")+1);
        if(!values.emplace(key,value).second) throw std::runtime_error("Duplicate Moon label key: "+key);
    }
    const std::pair<const char*,const char*> expected[]={
        {"PDS_VERSION_ID","\"PDS3\""},
        {"PRODUCT_VERSION_ID","\"V3.1\""},
        {"DATA_SET_ID","\"LRO-L-LOLA-4-GDR-V1.0\""},
        {"PRODUCT_ID","\"LDEM_64\""},
        {"TARGET_NAME","MOON"},
        {"FILE_NAME","\"LDEM_64.IMG\""},
        {"RECORD_TYPE","FIXED_LENGTH"},
        {"FILE_RECORDS","11520"},
        {"RECORD_BYTES","46080"},
        {"^IMAGE","\"LDEM_64.IMG\""},
        {"NAME","HEIGHT"},
        {"LINES","11520"},
        {"LINE_SAMPLES","23040"},
        {"MAXIMUM","21546"},
        {"MINIMUM","-18251"},
        {"SAMPLE_TYPE","LSB_INTEGER"},
        {"SAMPLE_BITS","16"},
        {"UNIT","METER"},
        {"SCALING_FACTOR","0.5"},
        {"OFFSET","1737400."},
        {"MAP_PROJECTION_TYPE","\"SIMPLE CYLINDRICAL\""},
        {"MAP_RESOLUTION","64 <pix/deg>"},
        {"A_AXIS_RADIUS","1737.4 <km>"},
        {"B_AXIS_RADIUS","1737.4 <km>"},
        {"C_AXIS_RADIUS","1737.4 <km>"},
        {"POSITIVE_LONGITUDE_DIRECTION","\"EAST\""},
        {"CENTER_LATITUDE","0 <deg>"},
        {"CENTER_LONGITUDE","180 <deg>"},
        {"LINE_FIRST_PIXEL","1"},
        {"LINE_LAST_PIXEL","11520"},
        {"SAMPLE_FIRST_PIXEL","1"},
        {"SAMPLE_LAST_PIXEL","23040"},
        {"MAP_PROJECTION_ROTATION","0.0"},
        {"MAP_SCALE","473.802 <m/pix>"},
        {"MAXIMUM_LATITUDE","90 <deg>"},
        {"MINIMUM_LATITUDE","-90 <deg>"},
        {"WESTERNMOST_LONGITUDE","0 <deg>"},
        {"EASTERNMOST_LONGITUDE","360 <deg>"},
        {"LINE_PROJECTION_OFFSET","5759.5 <pix>"},
        {"SAMPLE_PROJECTION_OFFSET","11519.5 <pix>"},
        {"COORDINATE_SYSTEM_TYPE","\"BODY-FIXED ROTATING\""},
        {"COORDINATE_SYSTEM_NAME","\"MEAN EARTH/POLAR AXIS OF DE421\""},
    };
    for(auto [key,want]:expected) if(values[key]!=want) throw std::runtime_error(std::string("Unsupported Moon label ")+key+": "+values[key]);
    for(const auto& [key,value]:values) {
        (void)value;
        if(key.find("NULL")!=std::string::npos||key.find("INVALID")!=std::string::npos||key.find("SATURATION")!=std::string::npos||
           key=="LINE_PREFIX_BYTES"||key=="LINE_SUFFIX_BYTES"||key.find("MISSING")!=std::string::npos)
            throw std::runtime_error("Unsupported Moon padding or special value: "+key);
    }
}
HeightField::HeightField(int ppd,std::vector<std::int16_t> samples):
    ppd_(ppd),width_(ppd>=1&&ppd<=64?360*ppd:0),height_(ppd>=1&&ppd<=64?180*ppd:0),samples_(std::move(samples)),north_(absent),south_(absent) {
    if(ppd<1||ppd>64||samples_.size()!=std::size_t(width_)*height_) throw std::runtime_error("Invalid Moon grid dimensions");
    for(auto dn:samples_) {
        if(dn < -18251 || dn > 21546) throw std::runtime_error("Moon height outside LOLA V3.1 range; check product/byte order");
    }
    auto pole=[&](int row) {
        double sum=0; int count=0;
        for(int x=0;x<width_;++x) {
            auto dn=samples_[std::size_t(row)*width_+x];
            sum+=meters(dn); ++count;
        }
        return count?float(sum/count):absent;
    };
    north_=pole(0); south_=pole(height_-1);
}
HeightField HeightField::importPds(const std::filesystem::path& label,const std::filesystem::path& image) {
    validateLabel(mars::readText(label));
    return HeightField(sourcePpd,readSamples(image,false));
}
HeightField HeightField::load(const std::filesystem::path& path) { return HeightField(sourcePpd,readSamples(path,true)); }
void HeightField::save(const std::filesystem::path& path) const {
    if(ppd_!=sourcePpd) throw std::runtime_error("Only the complete 64 ppd Moon product can be saved");
    if(std::filesystem::exists(path)) throw std::runtime_error("Output already exists; choose a new path");
    std::ofstream f(path,std::ios::binary);
    std::array<std::uint8_t,32> header{};
    const std::string magic="MOONDEM1"; std::copy(magic.begin(),magic.end(),header.begin());
    put32(header.data()+8,1); put32(header.data()+12,23040); put32(header.data()+16,11520); put32(header.data()+20,64);
    f.write(reinterpret_cast<const char*>(header.data()),header.size());
    std::array<std::uint8_t,65536> buffer{}; std::uint32_t hash=2166136261u;
    for(std::size_t offset=0;offset<samples_.size();) {
        auto count=std::min(buffer.size()/2,samples_.size()-offset);
        for(std::size_t i=0;i<count;++i) {
            auto u=static_cast<std::uint16_t>(samples_[offset+i]);
            buffer[2*i]=std::uint8_t(u); buffer[2*i+1]=std::uint8_t(u>>8);
            hashByte(hash,buffer[2*i]); hashByte(hash,buffer[2*i+1]);
        }
        f.write(reinterpret_cast<const char*>(buffer.data()),std::streamsize(2*count)); offset+=count;
    }
    put32(header.data()+24,hash); f.seekp(0); f.write(reinterpret_cast<const char*>(header.data()),header.size());
    f.close(); if(!f) throw std::runtime_error("Cannot finish Moon output: "+path.string());
}
float HeightField::sample(double lat,double lon) const {
    if(!std::isfinite(lat)||!std::isfinite(lon)) throw std::runtime_error("Non-finite Moon coordinates");
    lat=std::clamp(lat,-90.0,90.0);
    double x=mars::wrapLongitude(lon)*ppd_-.5,y=(90-lat)*ppd_-.5;
    int left=int(std::floor(x)); double fx=x-std::floor(x);
    int x0=(left+width_)%width_,x1=(x0+1)%width_;
    auto row=[&](int yy) { return mix(meters(samples_[std::size_t(yy)*width_+x0]),meters(samples_[std::size_t(yy)*width_+x1]),fx); };
    if(y<0) return mix(north_,row(0),(y+.5)*2);
    if(y>height_-1) return mix(row(height_-1),south_,(y-height_+1)*2);
    int y0=int(std::floor(y)),y1=std::min(y0+1,height_-1);
    return mix(row(y0),row(y1),y-y0);
}
Image Image::load(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary); Image image;
    std::string magic,hash; int version=0,w=0,h=0;
    if(!(input>>magic>>version>>w>>h>>hash)||magic!="MOON_RGB"||version!=1||
       w!=width||h!=height||hash.size()!=16||input.get()!='\n')
        throw std::runtime_error("Invalid lunar appearance map: "+path.string());
    image.rgb.resize(std::size_t(width)*height*3);
    if(!input.read(reinterpret_cast<char*>(image.rgb.data()),std::streamsize(image.rgb.size()))||input.peek()!=EOF)
        throw std::runtime_error("Lunar appearance map length mismatch");
    std::uint64_t value=14695981039346656037ULL;
    for(auto byte:image.rgb) { value^=byte; value*=1099511628211ULL; }
    std::size_t used=0; auto expected=std::stoull(hash,&used,16);
    if(used!=hash.size()||value!=expected) throw std::runtime_error("Lunar appearance map checksum mismatch");
    return image;
}
std::vector<float> polarSlopes(const HeightField& field) {
    std::vector<float> result(std::size_t(polarSize)*polarSize*2*3);
    constexpr double step=mars::radians/16; // Match default mesh's regional spacing.
    auto height=[&](mars::Vec3 p) { auto ll=mars::location(p); return field.sample(ll.lat,ll.lon); };
    for(int pole=0;pole<2;++pole) for(int y=0;y<polarSize;++y) for(int x=0;x<polarSize;++x) {
        float px=((x+.5f)/polarSize*2-1)*polarExtent,pz=((y+.5f)/polarSize*2-1)*polarExtent;
        mars::Vec3 p{px,(pole?-1.f:1.f)*std::sqrt(1-px*px-pz*pz),pz};
        auto east=mars::normalized(mars::Vec3{p.y,-p.x,0});
        auto north=mars::cross(p,east);
        auto slope=[&](mars::Vec3 axis) {
            return (height(p*float(std::cos(step))+axis*float(std::sin(step)))-
                    height(p*float(std::cos(step))-axis*float(std::sin(step))))/float(2*step*radiusMeters);
        };
        auto gradient=east*slope(east)+north*slope(north);
        auto offset=(std::size_t(pole*polarSize+y)*polarSize+x)*3;
        result[offset]=gradient.x; result[offset+1]=gradient.y; result[offset+2]=gradient.z;
    }
    return result;
}
}
