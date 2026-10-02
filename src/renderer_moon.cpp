#include "renderer.hpp"
#include <cstdio>
#include <filesystem>
namespace mars {
void Renderer::uploadMoonSlopes(const std::vector<float>& slopes) {
    if(slopes.size()!=std::size_t(moon::polarSize)*moon::polarSize*6) throw std::runtime_error("Invalid polar slope dimensions");
    gl_.DeleteTextures(1,&moonSlopes_); gl_.GenTextures(1,&moonSlopes_);
    gl_.ActiveTexture(GL_TEXTURE1); gl_.BindTexture(GL_TEXTURE_2D,moonSlopes_);
    gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,moon::polarSize,moon::polarSize*2,0,GL_RGB,GL_FLOAT,slopes.data());
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl_.ActiveTexture(GL_TEXTURE0); check();
}
void Renderer::initMoonColor() {
    const char* executable=SDL_GetBasePath();
    std::filesystem::path base=executable?executable:".";
    for(auto root:{std::filesystem::path("assets"),base/"assets",base.parent_path()/"assets"}) {
        auto path=root/"moon2019.rgb";
        if(!std::filesystem::exists(path)) continue;
        try {
            auto image=moon::Image::load(path);
            gl_.ActiveTexture(GL_TEXTURE0); gl_.GenTextures(1,&moonSource_); gl_.BindTexture(GL_TEXTURE_2D,moonSource_);
            gl_.PixelStorei(GL_UNPACK_ALIGNMENT,1);
            gl_.TexImage2D(GL_TEXTURE_2D,0,GL_RGB8,moon::Image::width,moon::Image::height,0,GL_RGB,GL_UNSIGNED_BYTE,image.rgb.data());
            gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
            gl_.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            gl_.GenerateMipmap(GL_TEXTURE_2D); check();
        } catch(const std::exception& e) {
            gl_.DeleteTextures(1,&moonSource_); moonSource_=0;
            SDL_Log("Moon appearance unavailable: %s",e.what());
        }
        return;
    }
    SDL_Log("Moon appearance unavailable: missing assets/moon2019.rgb");
}

void Renderer::moonHud(const Camera& c,const moon::HeightField& field,const Probe& p,float exaggeration,
                         int selected,int page,float mx,float my,float fps,bool fullscreen,bool heightColors,int meshPpd) {
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f};
    constexpr Vec3 paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    ui_.clear();
    int uw=layout_.width(),uh=layout_.height();
    auto panel=[&](Rect r) {
        rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.78f);
        rect(float(r.x),float(r.y),float(r.w),1,line,.7f);
        rect(float(r.x),float(r.y+r.h-1),float(r.w),1,line,.35f);
    };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        bool hover=r.contains(mx,my);
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.94f:hover?.95f:.70f);
        if(hover) border(r,paper,.7f);
        buttonLabel(r.x+9,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"MOON",paper,3);
    panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / LOCATIONS",muted);
    for(int row=0;row<moon::pageSize;++row) {
        int i=page*moon::pageSize+row;
        if(i<moon::landmarkCount) button(layout_.landmarkButton(row),std::to_string(row+1)+"  "+moon::landmarks[i].name,i==selected);
    }
    button(layout_.locationPrev(),"<");
    button(layout_.locationNext(),">");
    text(62,layout_.locationPrev().y+7,"PAGE "+std::to_string(page+1)+" / "+std::to_string(moon::pageCount)+"    , . / CHANGE",muted);
    button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN");
    char buf[120]; std::snprintf(buf,sizeof(buf),"E / RELIEF %.0fX",double(exaggeration));
    button(layout_.reliefButton(),buf); button(layout_.homeButton(),"HOME / RESET");
    auto sc=layout_.scalePanel(); panel(sc);
    bool fitted=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
    std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fitted?" FIT":"");
    text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
    button(layout_.environmentButton(),heightColors?"C / PALETTE: ELEVATION":moonAppearance()?"C / PALETTE: LROC":"C / PALETTE: GRAY");
    rect(float(uw-226),148,206,36,ink,.70f);
    text(uw-217,157,heightColors?"HEIGHT TINTS / LOLA":moonAppearance()?"LROC + LDAM / NASA 2019":"APPEARANCE MAP UNAVAILABLE",muted);
    text(uw-217,171,"CAMERA-RELATIVE STUDY LIGHT",muted);
    auto r=layout_.readout(); panel(r);
    text(r.x+10,r.y+9,p.underPointer?"02 / SURFACE PROBE":"02 / VIEW CENTRE",accent);
    std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(p.location.lat),p.location.lat>=0?'N':'S',p.location.lon);
    text(r.x+10,r.y+26,buf,paper,2); rect(float(r.x+10),float(r.y+48),302,1,line,.35f);
    if(std::isfinite(p.meters)) std::snprintf(buf,sizeof(buf),"%+.0f M",double(p.meters));
    else std::snprintf(buf,sizeof(buf),"NO DATA");
    text(r.x+10,r.y+57,buf,paper,2); text(r.x+136,r.y+56,"1737.4 KM SPHERE",muted);
    text(r.x+136,r.y+69,"LOLA / INTERPOLATED",muted);
    auto l=layout_.legend(); panel(l); text(l.x+10,l.y+9,heightColors?"03 / ELEVATION":"03 / MEASURED RELIEF",accent);
    if(heightColors) {
    for(int i=0;i<6;++i) {
        float t=std::floor(float(i)/5*9)/9;
        Vec3 low{.20f,.29f,.36f},middle{.57f,.62f,.55f},high{.91f,.82f,.62f};
        Vec3 color=t<.5f?low*(1-t*2)+middle*(t*2):middle*(2-t*2)+high*(t*2-1);
        rect(float(l.x+10+31*i),float(l.y+26),30,6,color);
    }
    text(l.x+10,l.y+39,"-9.1 KM",muted); text(l.x+151,l.y+39,"+10.8 KM",muted);
    } else { text(l.x+10,l.y+29,moonAppearance()?"8K APPEARANCE / FACET LIGHT":"GRAY SURFACE / FACET LIGHT",muted); text(l.x+10,l.y+42,"LANDERS BELOW GRID SCALE",muted); }
    text(l.x+10,l.y+55,"SOURCE 64 PPD / 474 M GRID",muted);
    std::snprintf(buf,sizeof(buf),"MESH %d PPD%s",meshPpd,c.distance<1.086f?" / ZOOM LIMIT":" / REGIONAL RELIEF");
    text(l.x+10,l.y+69,buf,muted);
    rect(20,float(uh-21),float(uw-40),21,ink,.78f); rect(20,float(uh-21),float(uw-40),1,line,.4f);
    text(20,uh-13,"DRAG / ORBIT   WHEEL / ZOOM   1-4 / PAGE LOCATION   ALT+LEFT / UP",muted);
    std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(uw-74,uh-13,buf,muted);
    if(p.underPointer) crosshair(int(mx),int(my),paper); else crosshair(uw/2,uh/2,muted);
    // The markers use the rendered depth to avoid showing through foreground relief.
    float aspect=float(layout_.pixelWidth)/layout_.pixelHeight;
    for(int i=page*moon::pageSize;i<std::min((page+1)*moon::pageSize,moon::landmarkCount);++i) {
        auto loc=moon::landmarks[i]; float meters=field.sample(loc.lat,loc.lon);
        if(!std::isfinite(meters)) continue;
        Vec3 pos=direction(loc.lat,loc.lon)*(1+meters*exaggeration/moon::radiusMeters),relative=pos-c.position();
        float z=-dot(relative,c.backward());
        if(z<=Camera::nearPlane||dot(pos,c.position()-pos)<=0) continue;
        float nx=dot(relative,c.right())/(z*c.lens*aspect),ny=dot(relative,c.up())/(z*c.lens);
        int px=int((nx+1)*.5f*layout_.pixelWidth),py=int((1-ny)*.5f*layout_.pixelHeight);
        int x=int(px/layout_.scale),y=int(py/layout_.scale);
        if(x<8||y<8||x>=uw-14||y>=uh-8||layout_.overHud(float(x),float(y))) continue;
        float depth=1; gl_.ReadPixels(px,layout_.pixelHeight-1-py,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);
        float visibleZ=2*Camera::nearPlane*Camera::farPlane/(Camera::farPlane+Camera::nearPlane-(2*depth-1)*(Camera::farPlane-Camera::nearPlane));
        if(depth<1&&z>visibleZ+.004f) continue;
        border({x-3,y-3,7,7},i==selected?paper:accent); text(x+7,y-3,std::to_string(i%moon::pageSize+1),accent);
    }
    flushUi();
}
}
