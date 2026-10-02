#pragma once
#include "catalog.hpp"
#include "faceted.hpp"
#include "guide.hpp"

namespace satellites {
inline constexpr int count=25;
inline constexpr std::array<solar::BodyId,count> ids{{solar::BodyId::Phobos,solar::BodyId::Deimos,solar::BodyId::Europa,solar::BodyId::Ganymede,solar::BodyId::Mimas,solar::BodyId::Enceladus,solar::BodyId::Tethys,solar::BodyId::Dione,solar::BodyId::Rhea,solar::BodyId::Titan,solar::BodyId::Iapetus,solar::BodyId::Uranus,solar::BodyId::Neptune,solar::BodyId::Miranda,solar::BodyId::Ariel,solar::BodyId::Umbriel,solar::BodyId::Titania,solar::BodyId::Oberon,solar::BodyId::Triton,solar::BodyId::Proteus,solar::BodyId::Ceres,solar::BodyId::Pluto,solar::BodyId::Haumea,solar::BodyId::Makemake,solar::BodyId::Eris}};
struct Region { const char* name; double lat,lon; float distance; const char* notes; };
struct Definition {
    solar::BodyId id;
    const char* asset;
    const char* magic;
    bool model;
    std::array<Region,4> regions;
    const char* description;
    const char* source;
};
std::vector<faceted::FacetVertex> makeCloudShell(solar::BodyId id);
bool supports(solar::BodyId id);
int slot(solar::BodyId id);
const Definition& definition(solar::BodyId id);
std::vector<faceted::FacetVertex> loadSurface(solar::BodyId id,const std::filesystem::path& path,int subdivisions=8);
const solar::Article& guide(solar::BodyId id);
const solar::Article& locationGuide(solar::BodyId id,int index);
}
