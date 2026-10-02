#include "renderer.hpp"
#include <cstdio>
namespace mars {
void Renderer::guideHud(const solar::Guide& guide,float mx,float my,bool scanAvailable) {
    if(!solar::hasGuide(guide.body)) return;
    constexpr Vec3 ink{.028f,.032f,.031f},line{.40f,.43f,.40f},muted{.66f,.70f,.65f};
    constexpr Vec3 paper{.92f,.94f,.88f},accent{.84f,.91f,.30f};
    constexpr Vec3 labelInk{.88f,.73f,.43f},valueInk{.64f,.85f,.96f},card{.065f,.083f,.095f};
    ui_.clear();
    auto drawLine=[&](int x,int y,int width,const solar::GuideLine& row,int pitch) {
        using Style=solar::GuideStyle;
        Vec3 color=paper;
        if(row.style==Style::Heading) color=accent;
        if(row.style==Style::Label) color=labelInk;
        if(row.style==Style::Value) color=valueInk;
        if(row.style==Style::Note) color=muted;
        if(row.style==Style::Value||row.style==Style::Label)
            rect(float(x-4),float(y-3),float(width),float(pitch),card);
        if(row.style==Style::Heading) rect(float(x),float(y-5),float(width),1,line,.6f);
        text(x,y,row.text,color,row.scale);
    };
    auto button=[&](Rect r,const std::string& label,bool active=false) {
        bool hover=r.contains(mx,my);
        rect(float(r.x),float(r.y),float(r.w),float(r.h),active?accent:ink,active?.95f:.90f);
        border(r,hover?paper:line,hover?.9f:.5f);
        buttonLabel(r.x+7,r.y+(r.h-7)/2,label,active?ink:paper);
    };
    button(layout_.guideToggle(),guide.open?"I / FIELD GUIDE   -":"I / FIELD GUIDE   +",guide.open);
    if(scanAvailable) button(layout_.scanButton(),"R / SCAN");
    // Location notes belong to the destination list, independently of the planet guide.
    if(layout_.locationAvailable) {
        const auto& article=solar::locationArticle(guide.body,guide.location);
        button(layout_.locationToggle(),std::string("ABOUT / ")+article.title+(guide.locationOpen?"   -":"   +"));
        if(guide.locationOpen) {
            auto r=layout_.locationPanel();
            rect(float(r.x),float(r.y),float(r.w),float(r.h),ink,.94f);
            rect(float(r.x),float(r.y),2,float(r.h),accent);
            auto lines=solar::styledArticleLines(article,layout_.locationColumns(),1.25f);
            int rows=layout_.locationRows(),scroll=std::clamp(guide.locationScroll,0,std::max(0,int(lines.size())-rows));
            for(int row=0;row<rows&&scroll+row<int(lines.size());++row)
                drawLine(r.x+10,r.y+5+row*13,r.w-24,lines[scroll+row],13);
            button(layout_.locationSource(),"SOURCE / OPEN WEB");
            char count[64]; std::snprintf(count,sizeof(count),"%d-%d / %zu",scroll+1,std::min(scroll+rows,int(lines.size())),lines.size());
            text(r.x+162,r.y+r.h-16,count,muted);
            if(int(lines.size())>rows) { button(layout_.locationUp(),"UP"); button(layout_.locationDown(),"DN"); }
        }
    }
    if(!guide.open) { flushUi(); return; }
    auto r=layout_.guidePanel();
    rect(float(r.x),float(r.y),float(r.w),float(r.h),ink); border(r,line,.8f);
    rect(float(r.x),float(r.y),3,float(r.h),accent);
    text(r.x+12,r.y+12,"FIELD GUIDE / "+std::string(solar::body(guide.body).name),accent);
    text(r.x+12,r.y+30,guide.body==solar::BodyId::Earth?"GEOGRAPHIC LANDMARKS":"EXPLORE / UNDERSTAND",paper,2);
    button(layout_.guideClose(),"X");
    rect(float(r.x+12),float(r.y+54),float(r.w-24),1,line,.6f);
    auto lines=solar::styledArticleLines(guide.article(),layout_.guideColumns());
    int rows=layout_.guideRows(),maximum=std::max(0,int(lines.size())-rows),scroll=std::clamp(guide.scroll,0,maximum);
    for(int row=0;row<rows&&scroll+row<int(lines.size());++row) {
        drawLine(r.x+16,r.y+66+row*18,r.w-36,lines[scroll+row],18);
    }
    if(maximum>0) {
        float height=float(rows*18),thumb=std::max(14.f,height*rows/lines.size());
        rect(float(r.x+r.w-8),float(r.y+66),2,height,line,.4f);
        rect(float(r.x+r.w-8),float(r.y+66)+(height-thumb)*scroll/maximum,2,thumb,accent);
    }
    button(layout_.guideSource(),"SOURCE / OPEN WEB");
    button(layout_.guideUp(),"UP"); button(layout_.guideDown(),"DN");
    char buf[120]; std::snprintf(buf,sizeof(buf),"%d-%d / %zu   WHEEL / SCROLL   ESC / CLOSE",scroll+1,std::min(scroll+rows,int(lines.size())),lines.size());
    text(r.x+12,r.y+r.h-20,buf,muted);
    flushUi();
}
}
