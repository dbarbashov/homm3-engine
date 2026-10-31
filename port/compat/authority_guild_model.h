#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
namespace Port { namespace Guild {
constexpr int MaxWords=2048;
inline int categories(int tier){return tier>=5?9:tier==4?8:tier==3?6:tier==2?4:2;}
struct Rank {std::array<int,8> players{},groups{};};
struct Player {
    int hero=-1,portrait=-1;std::string name;std::array<int,4> stats{};
    int personality=-1,creature=-1,armySource=-1,armySlot=-1;
};
struct View {
    int actor=-1,tier=0,count=0,mask=0;
    std::vector<Rank> ranks;
    std::array<Player,8> players;
};
inline std::vector<int32_t> encode(const View& view){
    std::vector<int32_t> out={1,view.actor,view.tier,view.count,view.mask};
    for(const auto& row:view.ranks)for(int i=0;i<view.count;++i){out.push_back(row.players[i]);out.push_back(row.groups[i]);}
    for(int i=0;i<8;++i)if(view.mask&(1<<i)){
        const auto& p=view.players[i];
        if(view.tier>=1){
            out.push_back(p.hero);
            if(p.hero>=0){out.push_back(p.portrait);out.push_back(p.name.size());for(unsigned char c:p.name)out.push_back(c);
                if(view.tier>=2)for(auto stat:p.stats)out.push_back(stat);}
        }
        if(view.tier>=3)out.push_back(p.personality);
        if(view.tier>=4){out.push_back(p.creature);if(i==view.actor){out.push_back(p.armySource);out.push_back(p.armySlot);}}
    }
    return out;
}
inline View decode(const int32_t* words,size_t length,int actor){
    if(!words||length<5||length>MaxWords)throw std::runtime_error("Invalid guild payload size");
    size_t cursor=0;
    auto take=[&](int low,int high){if(cursor==length)throw std::runtime_error("Truncated guild payload");int n=words[cursor++];if(n<low||n>high)throw std::runtime_error("Invalid guild field");return n;};
    take(1,1);View view;view.actor=take(0,7);if(view.actor!=actor)throw std::runtime_error("Wrong guild recipient");
    view.tier=take(0,5);view.count=take(1,8);view.mask=take(1,255);
    if(!(view.mask&(1<<actor)) || (view.mask>>view.count))throw std::runtime_error("Invalid guild roster");
    for(int category=0;category<categories(view.tier);++category){
        Rank row;int seen=0,previous=-1;
        for(int i=0;i<view.count;++i){const int id=take(0,view.count-1),rank=take(-1,7);if(seen&(1<<id))throw std::runtime_error("Duplicate guild rank player");seen|=1<<id;
            if((view.mask&(1<<id))?(rank<0||rank<previous):(rank!=-1))throw std::runtime_error("Invalid guild rank grouping");
            if(rank>=0)previous=rank;row.players[i]=id;row.groups[i]=rank;}
        view.ranks.push_back(row);
    }
    for(int i=0;i<8;++i)if(view.mask&(1<<i)){
        auto& p=view.players[i];
        if(view.tier>=1){p.hero=take(-1,255);if(p.hero>=0){p.portrait=take(0,255);const int size=take(0,63);for(int n=0;n<size;++n)p.name.push_back(char(take(1,255)));
            if(view.tier>=2)for(auto& stat:p.stats)stat=take(0,99);}}
        if(view.tier>=3)p.personality=take(0,3);
        if(view.tier>=4){p.creature=take(-1,255);if(i==view.actor){p.armySource=take(-10000,255);p.armySlot=take(-1,6);}}
    }
    if(cursor!=length)throw std::runtime_error("Unexpected guild fields");
    return view;
}
} }
