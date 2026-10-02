#include "renderer.hpp"
#include <cstdio>
namespace mars {
void Renderer::mercuryHud(const Camera& c,const mercury::HeightField& field,const Probe& p,float exaggeration,
                         int selected,float mx,float my,float fps,bool fullscreen,bool heightColors,int meshPpd) {
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
    panel(layout_.title()); rect(20,20,3,58,accent); text(32,28,"MERCURY",paper,3);
    panel(layout_.destinations()); text(29,layout_.destinations().y+7,"01 / LOCATIONS",muted);
    for(int i=0;i<4;++i) button(layout_.landmarkButton(i),"0"+std::to_string(i+1)+"  "+mercury::landmarks[i].name,i==selected);
    button(layout_.fullscreenButton(),fullscreen?"F11 / EXIT FULLSCREEN":"F11 / FULLSCREEN");
    char buf[120]; std::snprintf(buf,sizeof(buf),"E / RELIEF %.0fX",double(exaggeration));
    button(layout_.reliefButton(),buf); button(layout_.homeButton(),"HOME / RESET");
    auto sc=layout_.scalePanel(); panel(sc);
    bool fitted=std::abs(layout_.scale-layout_.baseScale()*layout_.requestedScale)>.001f;
    std::snprintf(buf,sizeof(buf),"UI / %.0f%%%s",double(layout_.requestedScale*100),fitted?" FIT":"");
    text(sc.x+9,sc.y+11,buf,muted); button(layout_.scaleDown(),"-"); button(layout_.scaleUp(),"+");
    button(layout_.environmentButton(),heightColors?"C / PALETTE: ELEVATION":"C / PALETTE: STONE");
    rect(float(uw-226),148,206,36,ink,.70f);
    text(uw-217,157,"HEIGHT TINTS / NOT TRUE COLOR",muted);
    text(uw-217,171,"CAMERA-RELATIVE STUDY LIGHT",muted);
    auto r=layout_.readout(); panel(r);
    text(r.x+10,r.y+9,p.underPointer?"02 / SURFACE PROBE":"02 / VIEW CENTRE",accent);
    std::snprintf(buf,sizeof(buf),"%05.2f %c / %06.2f E",std::abs(p.location.lat),p.location.lat>=0?'N':'S',p.location.lon);
    text(r.x+10,r.y+26,buf,paper,2); rect(float(r.x+10),float(r.y+48),302,1,line,.35f);
    if(std::isfinite(p.meters)) std::snprintf(buf,sizeof(buf),"%+.0f M",double(p.meters));
    else std::snprintf(buf,sizeof(buf),"NO DATA");
    text(r.x+10,r.y+57,buf,paper,2); text(r.x+136,r.y+56,"2439.4 KM SPHERE",muted);
    text(r.x+136,r.y+69,"MDIS / INTERPOLATED",muted);
    auto l=layout_.legend(); panel(l); text(l.x+10,l.y+9,"03 / ELEVATION",accent);
    const Vec3 colors[]={{.21f,.31f,.38f},{.31f,.43f,.46f},{.49f,.54f,.44f},{.70f,.62f,.40f},{.81f,.54f,.35f},{.92f,.77f,.56f}};
    for(int i=0;i<6;++i) {
        float t=float(i)/5;
        Vec3 stone={.40f+(.73f-.40f)*t,.38f+(.70f-.38f)*t,.35f+(.62f-.35f)*t};
        rect(float(l.x+10+31*i),float(l.y+26),30,6,heightColors?colors[i]:stone);
    }
    text(l.x+10,l.y+39,"-5.4 KM",muted); text(l.x+151,l.y+39,"+4.5 KM",muted);
    text(l.x+10,l.y+55,"SOURCE 64 PPD / 665 M GRID",muted);
    std::snprintf(buf,sizeof(buf),"MESH %d PPD%s",meshPpd,c.distance<1.04f?" / ZOOM LIMIT":" / REGIONAL RELIEF");
    text(l.x+10,l.y+69,buf,muted);
    rect(20,float(uh-21),float(uw-40),21,ink,.78f); rect(20,float(uh-21),float(uw-40),1,line,.4f);
    text(20,uh-13,"DRAG / ORBIT   WHEEL / ZOOM   1-4 / LOCATION   ALT+LEFT / UP",muted);
    std::snprintf(buf,sizeof(buf),"%.0f FPS",double(fps)); text(uw-74,uh-13,buf,muted);
    if(p.underPointer) crosshair(int(mx),int(my),paper); else crosshair(uw/2,uh/2,muted);
    // The markers use the rendered depth to avoid showing through foreground relief.
    float aspect=float(layout_.pixelWidth)/layout_.pixelHeight;
    for(int i=0;i<4;++i) {
        auto loc=mercury::landmarks[i]; float meters=field.sample(loc.lat,loc.lon);
        if(!std::isfinite(meters)) continue;
        Vec3 pos=direction(loc.lat,loc.lon)*(1+meters*exaggeration/mercury::radiusMeters),relative=pos-c.position();
        float z=-dot(relative,c.backward());
        if(z<=Camera::nearPlane||dot(pos,c.position()-pos)<=0) continue;
        float nx=dot(relative,c.right())/(z*c.lens*aspect),ny=dot(relative,c.up())/(z*c.lens);
        int px=int((nx+1)*.5f*layout_.pixelWidth),py=int((1-ny)*.5f*layout_.pixelHeight);
        int x=int(px/layout_.scale),y=int(py/layout_.scale);
        if(x<8||y<8||x>=uw-14||y>=uh-8||layout_.overHud(float(x),float(y))) continue;
        float depth=1; gl_.ReadPixels(px,layout_.pixelHeight-1-py,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&depth);
        float visibleZ=2*Camera::nearPlane*Camera::farPlane/(Camera::farPlane+Camera::nearPlane-(2*depth-1)*(Camera::farPlane-Camera::nearPlane));
        if(depth<1&&z>visibleZ+.004f) continue;
        border({x-3,y-3,7,7},i==selected?paper:accent); text(x+7,y-3,std::to_string(i+1),accent);
    }
    flushUi();
}
}
