#pragma once
#include "catalog.hpp"
#include "ui.hpp"
namespace solar {
struct Fact { const char* label; const char* value; };
struct Article {
    const char* title;
    std::vector<const char*> paragraphs;
    std::vector<Fact> facts;
    const char* note;
    const char* source;
    const char* factHeading="REFERENCE FACTS";
};
bool hasGuide(BodyId body);
int locationCount(BodyId body);
const Article& planetArticle(BodyId body);
const Article& locationArticle(BodyId body,int index);
std::vector<std::string> wrapText(const std::string& text,int columns);
std::vector<std::string> articleLines(const Article& article,int columns);
enum class GuideStyle { Body, Heading, Label, Value, Note, Space };
struct GuideLine { std::string text; GuideStyle style; float scale; };
// Typed rows share wrapping and scrolling between drawing and input handling.
std::vector<GuideLine> styledArticleLines(const Article& article,int columns,float bodyScale=1.5f);
struct Guide {
    BodyId body=BodyId::None;
    bool open=false,locationOpen=false;
    int location=-1,scroll=0,locationScroll=0;
    void sync(BodyId current,int selected=-1);
    void toggle();
    const Article& article() const;
    int maximumScroll(const mars::ViewLayout& layout) const;
    void move(int rows,const mars::ViewLayout& layout);
    int maximumLocationScroll(const mars::ViewLayout& layout) const;
    void moveLocation(int rows,const mars::ViewLayout& layout);
};
}
