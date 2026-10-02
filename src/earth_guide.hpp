#pragma once
#include "earth.hpp"
#include "guide.hpp"
#include <stdexcept>
namespace earth {
inline const solar::Article& guide() {
    static const solar::Article article={"EARTH / GEOGRAPHY",{
        "Mount Everest rises above the Himalayan mountain belt. The Mariana Trench descends beneath the western Pacific, with Challenger Deep at its southern end. Together they show the great contrast between Earth's highest mountain summit and its deepest ocean floor.",
        "The Grand Canyon exposes layered rock cut by the Colorado River and its tributaries. Mauna Loa forms a broad volcanic shield on the island of Hawaii, with flanks that continue far below the ocean surface."
    },{
        {"MOUNT EVEREST","HIMALAYAS / MOUNTAIN SUMMIT"},
        {"MARIANA TRENCH","WESTERN PACIFIC / DEEP-OCEAN TRENCH"},
        {"GRAND CANYON","ARIZONA / RIVER-CUT CANYON"},
        {"MAUNA LOA","HAWAII / SHIELD VOLCANO"}
    },"Choose a landmark with 1-4 or the location list to explore it and read its geography notes.",
    "https://science.nasa.gov/earth/earth-observatory/the-eight-thousanders/","GEOGRAPHIC LANDMARKS"};
    return article;
}
inline const solar::Article& locationGuide(int index) {
    static const solar::Article articles[]={
        {landmarks[0].name,{
            "Mount Everest is the highest mountain summit above sea level, rising about 8.8 kilometres in the Himalayas. The collision of the Indian and Eurasian plates raises this mountain belt, while glaciers cut its ridges and valleys. Everest's pyramid-shaped summit joins the nearby peak of Lhotse through the high saddle called the South Col."
        },{{"LANDFORM","MOUNTAIN SUMMIT"},{"RANGE","HIMALAYAS"},{"SUMMIT HEIGHT","ABOUT 8.8 KM ABOVE SEA LEVEL"}},
        "The map shows regional Himalayan relief; its elevation sample does not resolve the surveyed summit height.",
        "https://science.nasa.gov/earth/earth-observatory/the-eight-thousanders/","GEOGRAPHY"},
        {landmarks[1].name,{
            "The Mariana Trench curves across the western Pacific Ocean. Near its southern end, Challenger Deep is the deepest known part of the ocean, approximately 10,935 metres below the surface. This view centres on the Challenger Deep area, where the narrow seafloor depression contrasts with the surrounding ocean basin."
        },{{"LANDFORM","DEEP-OCEAN TRENCH"},{"REGION","WESTERN PACIFIC"},{"CHALLENGER DEEP","ABOUT 10,935 M BELOW SEA LEVEL"}},
        "Ocean-free topography reveals the trench. The regional map sample is not a measurement of the deepest surveyed point.",
        "https://oceanservice.noaa.gov/facts/oceandepth.html","GEOGRAPHY"},
        {landmarks[2].name,{
            "The Grand Canyon cuts across the Colorado Plateau in northern Arizona. The Colorado River carved downward through uplifted rock, and tributaries and erosion widened the canyon. Its cliffs expose layers deposited long before the canyon itself formed, with older crystalline rocks visible deep in the inner gorge."
        },{{"LANDFORM","RIVER-CUT CANYON"},{"REGION","NORTHERN ARIZONA / COLORADO PLATEAU"},{"RIVER","COLORADO"}},
        "This view locates the canyon and surrounding plateau; individual trails and narrow cliff faces are below the map's resolution.",
        "https://www.nps.gov/grca/learn/nature/grca-geology.htm","GEOGRAPHY"},
        {landmarks[3].name,{
            "Mauna Loa is a broad shield volcano on the island of Hawaii. Repeated basalt lava flows have built gentle slopes rising to 4,169 metres above sea level. Much of the volcano lies underwater: its submarine flanks extend about another five kilometres down to the ocean floor. Its large footprint contrasts with the sharp ridges of mountain ranges such as the Himalayas."
        },{{"LANDFORM","SHIELD VOLCANO"},{"REGION","ISLAND OF HAWAII / PACIFIC"},{"SUMMIT HEIGHT","4,169 M ABOVE SEA LEVEL"}},
        "Use ocean-free topography to compare the island's slopes with its submerged volcanic flanks.",
        "https://www.usgs.gov/volcanoes/mauna-loa","GEOGRAPHY"}
    };
    if(index<0||index>=int(landmarks.size())) throw std::runtime_error("Invalid Earth guide location");
    return articles[index];
}
}
