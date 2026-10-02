#include "guide.hpp"
#include "earth.hpp"
#include "preview.hpp"
#include "scan.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* message) { if(!good) throw std::runtime_error(message); }
template<class F> void rejects(F fn) { bool caught=false; try { fn(); } catch(...) { caught=true; } check(caught,"Bad preview accepted"); }
struct Scratch {
    std::filesystem::path path=std::filesystem::temp_directory_path()/("solar-preview-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~Scratch() { std::error_code e; std::filesystem::remove(path,e); }
};
}
int main(int argc,char** argv) {
    try {
        using namespace solar;
        for(auto body:{BodyId::Mars,BodyId::Mercury,BodyId::Jupiter,BodyId::Moon}) {
            check(planetArticle(body).paragraphs.size()==2,"Planet paragraphs missing");
            check(planetArticle(body).facts.size()>=13,"Planet facts missing");
            for(int index=0;index<locationCount(body);++index) {
                auto& article=locationArticle(body,index);
                check(article.paragraphs.size()==1&&std::string(article.paragraphs[0]).size()>200,"Location article missing");
                check(std::string(article.source).find("https://")==0,"Article source missing");
            }
        }
        for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout l; l.resize(800,500,scale); l.marsView=false; l.moonView=true;
            l.guideAvailable=l.locationAvailable=l.locationOpen=true;
            check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Moon notes cover readout");
            check(l.destinations().contains(float(l.locationNext().x),float(l.locationNext().y)),"Pager outside click panel");
            check(l.landmarkButton(3).y+22<=l.locationPrev().y,"Pager overlaps location");
            Guide moon; moon.sync(BodyId::Moon,15);
            check(moon.location==15&&moon.locationOpen,"Final lunar location lost");
            rejects([&] { locationArticle(BodyId::Moon,16); });
        }
        check(hasGuide(BodyId::Earth)&&locationCount(BodyId::Earth)==4,"Earth geography guide routing");
        const auto& geography=planetArticle(BodyId::Earth);
        check(std::string(geography.factHeading)=="GEOGRAPHIC LANDMARKS","Earth guide is not geography-focused");
        for(int i=0;i<locationCount(BodyId::Earth);++i) {
            const auto& article=locationArticle(BodyId::Earth,i);
            check(std::string(article.title)==earth::landmarks[i].name,"Earth guide and map destinations disagree");
            check(!article.paragraphs.empty()&&!article.facts.empty(),"Earth landmark has no geography notes");
            check(std::string(article.source).find("https://")==0,"Earth landmark has no reference");
            Guide notes; notes.sync(BodyId::Earth,i);
            check(notes.locationOpen&&notes.location==i,"Earth selection did not open location notes");
            for(float scale:{.75f,1.f,2.f}) {
                mars::ViewLayout l; l.marsView=false; l.earthView=true;
                l.guideAvailable=l.locationAvailable=l.locationOpen=true; l.resize(800,500,scale);
                check(l.locationPanel().y+l.locationPanel().h<l.readout().y,"Earth notes overlap coordinate readout");
                for(const auto& row:styledArticleLines(article,l.locationColumns(),1.25f))
                    check(row.text.size()*6*row.scale<=l.locationColumns()*7.5f+.01f,"Earth geography text overflows");
                notes.moveLocation(10000,l);
                check(notes.locationScroll==notes.maximumLocationScroll(l),"Earth notes cannot reach final geography fact");
            }
        }
        rejects([&] { locationArticle(BodyId::Earth,-1); });
        rejects([&] { locationArticle(BodyId::Earth,4); });
        check(hasGuide(BodyId::Sun)&&locationCount(BodyId::Sun)==0,"Sun guide routing");
        check(planetArticle(BodyId::Sun).facts.size()>=7,"Sun scale and activity facts missing");
        rejects([&] { locationArticle(BodyId::Sun,0); });
        Guide guide; guide.sync(BodyId::Mars); guide.toggle(); check(guide.open,"Guide toggle failed");
        for(int w:{800,1242,2560}) for(int h:{500,1000,1600}) for(float scale:{.75f,1.f,2.f}) {
            mars::ViewLayout layout; layout.resize(w,h,scale); layout.guideAvailable=true; layout.guideOpen=true;
            auto r=layout.guidePanel();
            check(r.x>=0&&r.y>=0&&r.x+r.w<=layout.width()&&r.y+r.h<=layout.height(),"Guide out of bounds");
            check(layout.guideRows()>0&&layout.guideColumns()>20,"Guide not readable");
            check(layout.overHud(float(r.x+5),float(r.y+5)),"Guide did not capture pointer");
            check(r.x==20&&layout.guideToggle().x==20,"Guide is not docked left");
            guide.sync(BodyId::Mars); guide.move(10000,layout);
            check(guide.scroll==guide.maximumScroll(layout),"Cannot reach final facts");
            guide.move(-10000,layout); check(guide.scroll==0,"Guide scroll underflow");
            auto styled=styledArticleLines(guide.article(),layout.guideColumns());
            bool label=false,value=false,heading=false,note=false;
            for(const auto& line:styled) {
                check(line.text.size()*6*line.scale<=layout.guideColumns()*9+.01f,"Styled guide text overflow");
                label|=line.style==GuideStyle::Label; value|=line.style==GuideStyle::Value;
                heading|=line.style==GuideStyle::Heading; note|=line.style==GuideStyle::Note;
            }
            check(label&&value&&heading&&note,"Guide lost semantic text styles");
            check(guide.maximumScroll(layout)==std::max(0,int(styled.size())-layout.guideRows()),"Guide scroll disagrees with styled rows");
            for(auto line:articleLines(guide.article(),layout.guideColumns())) check(int(line.size())<=layout.guideColumns(),"Text overflow");
            for(bool mercury:{false,true}) {
                layout.marsView=!mercury; layout.mercuryView=mercury; layout.guideOpen=false;
                layout.locationAvailable=true; layout.locationOpen=true;
                guide.sync(mercury?BodyId::Mercury:BodyId::Mars,0);
                auto notes=layout.locationPanel(),list=layout.destinations();
                check(notes.x==list.x&&notes.w==list.w&&notes.y>list.y+list.h,"Notes are not beneath destinations");
                check(notes.y+notes.h<layout.readout().y,"Notes overlap elevation readout");
                check(layout.locationRows()>=2,"Compact notes cannot show two lines");
                check(layout.overHud(float(notes.x+6),float(notes.y+6)),"Notes did not capture pointer");
                if(!mercury) check(layout.orbitButton().y>=notes.y+notes.h&&layout.orbitButton().y+26<layout.readout().y,"Notes overlap satellite control");
                guide.moveLocation(10000,layout);
                check(guide.locationScroll==guide.maximumLocationScroll(layout),"Cannot reach final location lines");
                for(auto line:articleLines(locationArticle(guide.body,0),layout.locationColumns()))
                    check(int(line.size())<=layout.locationColumns(),"Location text overflow");
                layout.locationOpen=false;
                if(!mercury) check(layout.orbitButton().y<notes.y+notes.h,"Collapsed notes reserve unused space");
            }
            layout.marsView=false; layout.mercuryView=false;
            check(layout.openBody().y+layout.openBody().h<layout.readout().y,"Solar list overlaps readout with left guide");
            auto before=layout.bodyButton(1); layout.guideAvailable=false;
            auto after=layout.bodyButton(1);
            check(before.x==after.x&&before.y==after.y&&before.h==after.h,"Selecting a planet shifts double-click targets");
            layout.guideAvailable=true;
            layout.marsView=true; layout.satellite=true;
            check(layout.orbitButton().x>=layout.guideToggle().x+layout.guideToggle().w,"Satellite exit overlaps guide toggle");
            check(layout.orbitPanel().y+layout.orbitPanel().h<layout.readout().y,"Guide displaced orbit controls into readout");
        }
        guide.sync(BodyId::Mercury,2); check(guide.location==2&&guide.scroll==0,"Selected location did not follow");
        guide.locationOpen=false; guide.sync(BodyId::Mercury,2); check(!guide.locationOpen,"Collapsed notes reopened without selection");
        guide.sync(BodyId::Mercury,1); check(guide.location==1&&guide.locationOpen&&guide.locationScroll==0,"New destination did not expand notes");
        guide.sync(BodyId::Mercury,-1); check(guide.location==-1&&!guide.locationOpen,"Unselected location still has notes");
        guide.sync(BodyId::Venus); check(!guide.open&&!hasGuide(guide.body),"Unknown planet has an active guide");
        std::string longWord(130,'A'); auto wrapped=wrapText(longWord,30); check(wrapped.size()==5&&wrapped.back().size()==10,"Long word wrap");
        SurfaceScan scan; scan.update(.1,true); check(scan.strength()==1,"Loading scan invisible");
        for(int i=0;i<100;++i) scan.update(.1,true);
        check(scan.phase()>=0&&scan.phase()<1,"Scan phase outside loop");
        scan.update(.1,false); check(scan.strength()>0&&scan.strength()<1,"Scan completion did not fade");
        for(int i=0;i<20;++i) scan.update(.1,false);
        check(scan.strength()==0,"Scan did not finish");
        scan.replay(); scan.update(.1,false); check(scan.strength()==1,"Scan replay failed");
        Scratch scratch;
        writePreview(scratch.path,false,[](double lat,double lon) { return float(lat*2+lon/20); });
        auto mesh=loadPreview(scratch.path,false); check(mesh.indices.size()/3==65536,"Preview mesh cost changed");
        for(std::size_t index=1;index+1<mesh.vertices.size();++index) {
            auto v=mesh.vertices[index]; auto loc=mars::location(v.direction);
            float expected=float(loc.lat*2+loc.lon/20);
            check(std::abs(v.meters-expected)<.07f,"Preview changed sample coordinates or units");
        }
        rejects([&] { loadPreview(scratch.path,true); }); // A Mars file cannot be interpreted as Mercury.
        { std::ofstream out(scratch.path); out<<"SOLAR_PREVIEW 1 MARS 256 128 3396000\n1 2 3\n"; }
        rejects([&] { loadPreview(scratch.path,false); });
        if(argc==2) {
            for(bool mercury:{false,true}) {
                auto real=loadPreview(std::filesystem::path(argv[1])/(mercury?"mercury.preview":"mars.preview"),mercury);
                check(real.vertices.size()==32770,"Bundled preview dimensions");
                check(real.indices.size()==196608,"Bundled preview topology");
            }
        }
        std::cout<<"PASS: articles, wrapping, scaled overlay, scrolling, destination follow, scan lifecycle, preview coordinates/validation\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
