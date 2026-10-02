#include "terrain.hpp"
#include <array>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <stdexcept>

namespace mars {
namespace {
constexpr std::size_t count=std::size_t(HeightField::width)*HeightField::height;
std::vector<std::uint8_t> readBytes(const std::filesystem::path& p, std::size_t size) {
    std::ifstream f(p,std::ios::binary|std::ios::ate);
    if(!f) throw std::runtime_error("Cannot open "+p.string());
    if(f.tellg()!=std::streamoff(size)) throw std::runtime_error("Wrong file size: "+p.string());
    f.seekg(0);
    std::vector<std::uint8_t> b(size);
    if(!f.read(reinterpret_cast<char*>(b.data()),std::streamsize(size))) throw std::runtime_error("Read failed: "+p.string());
    return b;
}
std::uint32_t checksum(const std::uint8_t* p, std::size_t n) {
    std::uint32_t h=2166136261u;
    for(std::size_t i=0;i<n;++i) h=(h^p[i])*16777619u;
    return h;
}
void put32(std::uint8_t* p, std::uint32_t n) { for(int i=0;i<4;++i) p[i]=std::uint8_t(n>>(8*i)); }
std::uint32_t get32(const std::uint8_t* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
}
std::int16_t decodeSigned16(std::uint8_t high, std::uint8_t low) {
    int u=int(high)*256+low;
    return static_cast<std::int16_t>(u>=32768 ? u-65536 : u);
}
std::string readText(const std::filesystem::path& p) {
    std::ifstream f(p);
    if(!f) throw std::runtime_error("Cannot open label: "+p.string());
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
void validateMolaLabel(const std::string& text) {
    // Read single-line scalar assignments only. Never infer a sample format from an extension.
    std::map<std::string,std::string> values;
    std::regex assignment(R"((?:^|\n)[ \t]*([\^A-Z_]+)[ \t]*=[ \t]*([^\r\n]*))");
    for(auto i=std::sregex_iterator(text.begin(),text.end(),assignment);i!=std::sregex_iterator();++i) {
        std::string key=(*i)[1], value=(*i)[2];
        if(key=="OBJECT" || key=="END_OBJECT" || key=="DESCRIPTION") continue;
        auto end=value.find_last_not_of(" \t");
        value=end==std::string::npos ? "" : value.substr(0,end+1);
        if(!values.emplace(key,value).second) throw std::runtime_error("Duplicate label key: "+key);
    }
    const std::pair<const char*,const char*> expected[] = {
        {"PDS_VERSION_ID","PDS3"},{"RECORD_TYPE","FIXED_LENGTH"},
        {"^IMAGE","\"MEGT90N000EB.IMG\""},{"PRODUCT_ID","\"MEGT90N000EB.IMG\""},
        {"PRODUCT_VERSION_ID","\"2.0\""},{"DATA_SET_ID","\"MGS-M-MOLA-5-MEGDR-L3-V1.0\""},
        {"NAME","MEDIAN_TOPOGRAPHY"},{"LINES","2880"},{"LINE_SAMPLES","5760"},
        {"FILE_RECORDS","2880"},{"RECORD_BYTES","11520"},{"SAMPLE_BITS","16"},
        {"SAMPLE_TYPE","MSB_INTEGER"},{"UNIT","METER"},
        {"MAP_PROJECTION_TYPE","\"SIMPLE CYLINDRICAL\""},{"MAP_RESOLUTION","16.0 <PIXEL/DEGREE>"},
        {"POSITIVE_LONGITUDE_DIRECTION","\"EAST\""},{"COORDINATE_SYSTEM_NAME","\"PLANETOCENTRIC\""},
        {"COORDINATE_SYSTEM_TYPE","\"BODY-FIXED ROTATING\""},
        {"CENTER_LATITUDE","0.0 <DEGREE>"},{"CENTER_LONGITUDE","180.0 <DEGREE>"},
        {"LINE_PROJECTION_OFFSET","1440.5"},{"SAMPLE_PROJECTION_OFFSET","2880.5"},
        {"MAXIMUM_LATITUDE","90.0 <DEGREE>"},{"MINIMUM_LATITUDE","-90.0 <DEGREE>"},
        {"WESTERNMOST_LONGITUDE","0.0 <DEGREE>"},{"EASTERNMOST_LONGITUDE","360.0 <DEGREE>"},
        {"LINE_FIRST_PIXEL","1"},{"LINE_LAST_PIXEL","2880"},
        {"SAMPLE_FIRST_PIXEL","1"},{"SAMPLE_LAST_PIXEL","5760"},
        {"MAP_PROJECTION_ROTATION","0.0"},{"A_AXIS_RADIUS","3396.0 <KM>"},
        {"B_AXIS_RADIUS","3396.0 <KM>"},{"C_AXIS_RADIUS","3396.0 <KM>"}
    };
    for(auto [key,want]:expected)
        if(values[key]!=want) throw std::runtime_error(std::string("Unsupported label ")+key+": "+values[key]);
    for(const auto& [key,value]:values) {
        (void)value;
        if(key=="SCALING_FACTOR" || key=="OFFSET" || key.find("MISSING")!=std::string::npos ||
           key.find("NULL")!=std::string::npos || key.find("INVALID")!=std::string::npos ||
           key.find("SATURATION")!=std::string::npos || key=="LINE_PREFIX_BYTES" || key=="LINE_SUFFIX_BYTES")
            throw std::runtime_error("Unexpected scaling, padding, or special-value declaration: "+key);
    }
}
HeightField::HeightField(std::vector<std::int16_t> samples):samples_(std::move(samples)) {
    if(samples_.size()!=count) throw std::runtime_error("Expected the complete 16 ppd grid");
    // The label declares no no-data value. Do not invent -32768 as a valid measurement.
    for(auto h:samples_) if(h < -8177 || h > 21171) throw std::runtime_error("Elevation outside documented MOLA range; check product/byte order");
    double n=0,s=0;
    for(std::uint32_t x=0;x<width;++x) { n+=samples_[x]; s+=samples_[(height-1)*width+x]; }
    north_=float(n/width); south_=float(s/width);
}
HeightField HeightField::importMola(const std::filesystem::path& label,const std::filesystem::path& image) {
    validateMolaLabel(readText(label));
    auto b=readBytes(image,count*2);
    std::vector<std::int16_t> heights(count);
    for(std::size_t i=0;i<count;++i) heights[i]=decodeSigned16(b[2*i],b[2*i+1]);
    return HeightField(std::move(heights));
}
void HeightField::save(const std::filesystem::path& path) const {
    std::vector<std::uint8_t> b(32+2*count,0);
    const std::string magic="MARSMOLA";
    std::copy(magic.begin(),magic.end(),b.begin());
    put32(b.data()+8,1); put32(b.data()+12,width); put32(b.data()+16,height); put32(b.data()+20,ppd);
    for(std::size_t i=0;i<count;++i) {
        auto u=static_cast<std::uint16_t>(samples_[i]);
        b[32+2*i]=std::uint8_t(u); b[33+2*i]=std::uint8_t(u>>8);
    }
    put32(b.data()+24,checksum(b.data()+32,2*count));
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    if(!f || !f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size())))
        throw std::runtime_error("Cannot write "+path.string());
    f.close();
    if(!f) throw std::runtime_error("Cannot finish writing "+path.string());
}
HeightField HeightField::load(const std::filesystem::path& path) {
    auto b=readBytes(path,32+2*count);
    if(std::string(b.begin(),b.begin()+8)!="MARSMOLA" || get32(b.data()+8)!=1 ||
       get32(b.data()+12)!=width || get32(b.data()+16)!=height || get32(b.data()+20)!=ppd || get32(b.data()+28)!=0)
        throw std::runtime_error("Unsupported Mars Sim data header");
    if(get32(b.data()+24)!=checksum(b.data()+32,2*count)) throw std::runtime_error("Data checksum mismatch; reconvert the source");
    std::vector<std::int16_t> h(count);
    for(std::size_t i=0;i<count;++i) h[i]=decodeSigned16(b[33+2*i],b[32+2*i]);
    return HeightField(std::move(h));
}
float HeightField::sample(double lat,double lon) const {
    if(!std::isfinite(lat)||!std::isfinite(lon)) throw std::runtime_error("Non-finite coordinates");
    lat=std::clamp(lat,-90.0,90.0);
    double x=wrapLongitude(lon)*ppd-0.5, y=(90-lat)*ppd-0.5;
    int x0=int(std::floor(x)), x1=(x0+1+int(width))%int(width);
    float fx=float(x-std::floor(x)); x0=(x0+int(width))%int(width);
    auto row=[&](int yy) { return samples_[std::size_t(yy)*width+x0]*(1-fx)+samples_[std::size_t(yy)*width+x1]*fx; };
    // Collapse each unobserved half-cell cap to one common pole; continuous in longitude.
    if(y<0) { float t=float((y+0.5)*2); return north_*(1-t)+row(0)*t; }
    if(y>height-1) { float t=float((y-(height-1))*2); return row(height-1)*(1-t)+south_*t; }
    int y0=int(std::floor(y)), y1=std::min(y0+1,int(height)-1);
    float fy=float(y-y0);
    return row(y0)*(1-fy)+row(y1)*fy;
}
Mesh makeGlobe(const HeightField& field,int columns) {
    return makeSampledGlobe(columns,[&](double lat,double lon) {
        if(columns==int(HeightField::width)&&std::abs(lat)<90) {
            auto row=std::size_t(std::llround((90-lat)*HeightField::ppd-.5));
            auto col=std::size_t(std::llround(lon*HeightField::ppd-.5));
            return float(field.samples()[row*HeightField::width+col]);
        }
        return field.sample(lat,lon);
    });
}
Mesh makeSampledGlobe(int columns,const std::function<float(double,double)>& sample) {
    if(columns<8 || columns>5760 || columns%2) throw std::runtime_error("Invalid globe mesh resolution");
    int rows=columns/2;
    double step=360.0/columns;
    Mesh m;
    // Cell-centred rings: at 16 ppd each non-pole vertex is an exact source sample.
    m.vertices.reserve(std::size_t(rows)*columns+2);
    m.vertices.push_back({{0,1,0},sample(90,0)});
    std::vector<Vec3> equator;
    for(int i=0;i<columns;++i) equator.push_back(direction(0,(i+0.5)*step));
    for(int j=0;j<rows;++j) {
        double lat=90.0-(j+0.5)*step;
        float cl=float(std::cos(lat*radians)),sl=float(std::sin(lat*radians));
        for(int i=0;i<columns;++i) {
            float meters=sample(lat,(i+0.5)*step);
            m.vertices.push_back({{equator[i].x*cl,sl,equator[i].z*cl},meters});
        }
    }
    auto south=std::uint32_t(m.vertices.size());
    m.vertices.push_back({{0,-1,0},sample(-90,0)});
    auto ring=[&](int j,int i) { return std::uint32_t(1+j*columns+i%columns); };
    m.indices.reserve(std::size_t(6)*columns*rows);
    // Fixed-resolution ~5-degree draw batches. Adjacent batches share the same
    // vertices, including the wrapped seam and common poles: no LOD edges/skirt.
    int tile=std::max(1,columns/72);
    for(int j=0;j<=rows;j+=tile) for(int i=0;i<columns;i+=tile) {
        int je=std::min(j+tile,rows+1),ie=std::min(i+tile,columns);
        double north=j==0?90:90-(j-0.5)*step;
        double bottom=je>rows?-90:90-(je-0.5)*step;
        TerrainBatch batch;
        batch.first=std::uint32_t(m.indices.size());
        batch.centre=direction((north+bottom)*0.5,((i+ie)*0.5+0.5)*step);
        batch.halfAngle=float(((north-bottom)+(ie-i)*step)*0.5*radians);
        batch.minimum=32767; batch.maximum=-32768;
        auto tri=[&](std::uint32_t a,std::uint32_t b,std::uint32_t c) {
            // Missing observations leave a hole, never a fictitious deep spike or zero height.
            if(!std::isfinite(m.vertices[a].meters)||!std::isfinite(m.vertices[b].meters)||!std::isfinite(m.vertices[c].meters)) return;
            m.indices.insert(m.indices.end(),{a,b,c});
            for(auto idx:{a,b,c}) {
                batch.minimum=std::min(batch.minimum,m.vertices[idx].meters);
                batch.maximum=std::max(batch.maximum,m.vertices[idx].meters);
            }
        };
        for(int y=j;y<je;++y) for(int x=i;x<ie;++x) {
            if(y==0) tri(0,ring(0,x),ring(0,x+1));
            else if(y==rows) tri(ring(rows-1,x),south,ring(rows-1,x+1));
            else {
                tri(ring(y-1,x),ring(y,x),ring(y-1,x+1));
                tri(ring(y-1,x+1),ring(y,x),ring(y,x+1));
            }
        }
        batch.count=std::uint32_t(m.indices.size())-batch.first;
        if(batch.count) m.batches.push_back(batch);
    }
    // Conservative inscribed-sphere correction for straight triangle chords.
    m.chordFactor=float(std::max(0.0,std::cos(2*step*radians)));
    return m;
}
bool batchVisible(const TerrainBatch& b,const Camera& c,float exaggeration,float aspect,float chordFactor,float baseRadius,float minimum) {
    float low=1+b.minimum*exaggeration/baseRadius,high=1+b.maximum*exaggeration/baseRadius;
    float occluder=(1+minimum*exaggeration/baseRadius)*chordFactor;
    float horizon=std::acos(std::clamp(occluder/c.distance,0.0f,1.0f))+
                  std::acos(std::clamp(occluder/high,0.0f,1.0f));
    if(horizon+b.halfAngle<float(pi)&&dot(b.centre,c.outward())<std::cos(horizon+b.halfAngle)) return false;
    Vec3 centre=b.centre*((low+high)*0.5f);
    float radius=2*high*std::sin(b.halfAngle*0.5f)+(high-low)*0.5f+0.00001f;
    Vec3 delta=centre-c.position();
    float z=-dot(delta,c.backward()),x=dot(delta,c.right()),y=dot(delta,c.up());
    float fx=c.lens*aspect,fy=c.lens;
    return z+radius>=Camera::nearPlane && z-radius<=Camera::farPlane &&
           std::abs(x)-z*fx<=radius*std::sqrt(1+fx*fx) &&
           std::abs(y)-z*fy<=radius*std::sqrt(1+fy*fy);
}
} // namespace mars
