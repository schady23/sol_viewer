#include "saturn.hpp"
#include <fstream>
#include <charconv>
#include <stdexcept>
#include <string>
namespace saturn {
double centricLatitude(double graphic) { return std::atan(axisRatio*axisRatio*std::tan(graphic*mars::radians))/mars::radians; }
double graphicLatitude(double centric) { return std::atan(std::tan(centric*mars::radians)/(axisRatio*axisRatio))/mars::radians; }
double jetSpeed(double latitude) {
    latitude=std::clamp(latitude,-90.0,90.0);
    double x=std::abs(latitude);
    return (330*std::exp(-x*x/180.)+75*std::sin(latitude*mars::radians*12))*std::pow(std::cos(latitude*mars::radians),2);
}
void Clock::advance(double dt) { if(!paused&&std::isfinite(dt)&&dt>0) seconds+=std::min(dt,.25)*rates[speed]; }
void Clock::setRate(double rate) {
    for(int i=0;i<int(rates.size());++i) if(rate==rates[i]) { speed=i; return; }
    throw std::runtime_error("Saturn rate must be 1, 60, 600 or 3600");
}
void Clock::cycleRate(int delta) { speed=(speed+delta%int(rates.size())+int(rates.size()))%int(rates.size()); }
void Clock::reset() { seconds=0; ++revision; }
double Clock::rotation() const { return std::fmod(seconds/rotationSeconds,1.)*360; }
double Clock::loopFraction() const { return std::fmod(seconds,loopSeconds)/loopSeconds; }
namespace {
std::vector<unsigned char> loadChecked(const std::filesystem::path& path,const char* expectedMagic,int width,int height,int channels) {
    std::ifstream input(path,std::ios::binary);
    std::string magic,hash; int version=0,w=0,h=0;
    if(!(input>>magic>>version>>w>>h>>hash)||magic!=expectedMagic||version!=1||w!=width||h!=height||hash.size()!=16||input.get()!='\n')
        throw std::runtime_error("Invalid Saturn asset header: "+path.string());
    std::uint64_t expected=0;
    auto parsed=std::from_chars(hash.data(),hash.data()+hash.size(),expected,16);
    if(parsed.ec!=std::errc{}||parsed.ptr!=hash.data()+hash.size()) throw std::runtime_error("Invalid Saturn asset checksum");
    std::vector<unsigned char> bytes(std::size_t(w)*h*channels);
    if(!input.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))||input.peek()!=EOF)
        throw std::runtime_error("Saturn asset length mismatch");
    std::uint64_t hashValue=14695981039346656037ULL;
    for(auto b:bytes) { hashValue^=b; hashValue*=1099511628211ULL; }
    if(hashValue!=expected) throw std::runtime_error("Saturn asset checksum mismatch");
    return bytes;
}
}
Image Image::load(const std::filesystem::path& path) { return {2048,1024,loadChecked(path,"SATURN_RGB",2048,1024,3)}; }
RingImage RingImage::load(const std::filesystem::path& path) { return {loadChecked(path,"SATURN_RINGS",4096,1,4)}; }
std::vector<mars::Vec3> makeRings(int segments) {
    if(segments<32||segments>4096) throw std::runtime_error("Invalid Saturn ring tessellation");
    std::vector<mars::Vec3> points; points.reserve(std::size_t(segments)*6);
    float inner=float(ringInnerKm/equatorialKm),outer=float(ringOuterKm/equatorialKm);
    for(int i=0;i<segments;++i) {
        auto a=mars::direction(0,i*360.0/segments),b=mars::direction(0,(i+1)*360.0/segments);
        points.insert(points.end(),{a*inner,a*outer,b*inner,a*outer,b*outer,b*inner});
    }
    return points;
}
}
