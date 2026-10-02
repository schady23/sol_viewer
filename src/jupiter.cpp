#include "jupiter.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
namespace jupiter {
double centricLatitude(double graphic) { return std::atan(axisRatio*axisRatio*std::tan(graphic*mars::radians))/mars::radians; }
double graphicLatitude(double centric) { return std::atan(std::tan(centric*mars::radians)/(axisRatio*axisRatio))/mars::radians; }
double jetSpeed(double latitude) {
    // Rounded, deliberately sparse approximation of the OPAL 2015 Fig. 2 profile.
    // Beyond the plotted +/-50 degrees, taper the model to zero at the poles.
    constexpr double lat[]={-90,-70,-50,-45,-40,-36,-32,-27,-23,-20,-16,-12,-7,-2,2,7,12,17,20,24,28,31,35,39,43,47,50,70,90};
    constexpr double vel[]={0,8,0,42,0,38,-18,48,8,-55,-12,45,145,85,78,115,45,-20,25,153,25,-28,35,-18,22,-4,0,8,0};
    latitude=std::clamp(latitude,-90.0,90.0);
    for(std::size_t i=1;i<std::size(lat);++i) if(latitude<=lat[i]) {
        double t=(latitude-lat[i-1])/(lat[i]-lat[i-1]); t=t*t*(3-2*t);
        return vel[i-1]+(vel[i]-vel[i-1])*t;
    }
    return 0;
}
void Clock::advance(double dt) { if(!paused&&std::isfinite(dt)&&dt>0) seconds+=std::min(dt,.25)*rates[speed]; }
void Clock::setRate(double rate) {
    for(int i=0;i<int(rates.size());++i) if(rate==rates[i]) { speed=i; return; }
    throw std::runtime_error("Jupiter rate must be 1, 60, 600 or 3600");
}
void Clock::cycleRate(int delta) { speed=(speed+delta%int(rates.size())+int(rates.size()))%int(rates.size()); }
void Clock::reset() { seconds=0; ++revision; }
double Clock::rotation() const { return std::fmod(seconds/rotationSeconds,1.)*360; }
double Clock::loopFraction() const { return std::fmod(seconds,loopSeconds)/loopSeconds; }
Image Image::load(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary); Image image;
    std::string magic,hash; int version=0;
    if(!(input>>magic>>version>>image.width>>image.height>>hash)||magic!="JUPITER_RGB"||(version!=1&&version!=2)||
       (version==1&&(image.width!=2048||image.height!=1024))||
       (version==2&&(image.width!=3600||image.height!=1800))||hash.size()!=16||input.get()!='\n')
        throw std::runtime_error("Invalid Jupiter cloud asset: "+path.string());
    image.rgb.resize(std::size_t(image.width)*image.height*3);
    if(!input.read(reinterpret_cast<char*>(image.rgb.data()),std::streamsize(image.rgb.size()))||input.peek()!=EOF)
        throw std::runtime_error("Jupiter cloud asset length mismatch");
    std::uint64_t value=14695981039346656037ULL;
    for(auto byte:image.rgb) { value^=byte; value*=1099511628211ULL; }
    std::size_t used=0; auto expected=std::stoull(hash,&used,16);
    if(used!=hash.size()||value!=expected) throw std::runtime_error("Jupiter cloud asset checksum mismatch");
    return image;
}
}
