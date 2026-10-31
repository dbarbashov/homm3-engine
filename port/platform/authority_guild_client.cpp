#include "authority_guild_client.h"
#include "authority_client.h"
#include "authority_choices.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "townmgr.h"
#include "creaturetype.h"
#include <algorithm>
#include <cstring>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace Port { namespace AuthorityGuildClient {
namespace {
bool showing=false;
Guild::View report;
std::array<hero,8> portraits;
}
bool active(){return showing;}
void install(const Guild::View& view){
    report=view;
    for(int who=0;who<8;++who){
        const auto& p=report.players[who];auto& h=portraits[who];
        if(p.hero>=game::HERO_COUNT||p.portrait>=game::HERO_COUNT||p.creature>g_creatureTypeLast)throw std::runtime_error("Guild identity exceeds original tables");
        h.m_id=static_cast<HeroId>(p.hero);h.m_owner=who;h.m_portrait=p.portrait;
        const size_t size=std::min(sizeof(h.m_name)-1,p.name.size());std::memcpy(h.m_name,p.name.data(),size);h.m_name[size]=0;
        for(int skill=0;skill<4;++skill)h.setPrimarySkill(skill,p.stats[skill]);
    }
    showing=true;
}
void clear(){showing=false;report=Guild::View();}
int tier(){return report.tier;}
int playerCount(){return showing?report.count:g_game->m_numPlayers;}
bool disabled(int player){return showing?!(report.mask&(1<<player)):g_game->m_playerDisabled[player]!=0;}
void rankStats(int category,long* values,signed char* players){
    if(category<0||category>=int(report.ranks.size()))throw std::runtime_error("Guild category unavailable");
    const auto& row=report.ranks[category];
    for(int i=0;i<report.count;++i){players[i]=static_cast<signed char>(row.players[i]);values[i]=row.groups[i]<0?-1:8-row.groups[i];}
}
hero* bestHero(int player){return report.players[player].hero>=0?&portraits[player]:nullptr;}
const char* heroName(int column){
    for(int who=0;who<8;++who)if(report.mask&(1<<who)){if(!column--)return report.players[who].name.c_str();}
    return "";
}
int personality(int player){return showing?report.players[player].personality:g_game->m_players[player].m_personality;}
int creature(int player){return report.players[player].creature;}
void creatureDisplay(int player,armyGroup& army,int& slot){
    const auto& p=report.players[player];army.initialize();slot=0;
    if(p.creature<0)return;
    if(player==report.actor && p.armySlot>=0){
        if(p.armySource>=0 && p.armySource<game::HERO_COUNT && g_game->m_heroes[p.armySource].m_owner==player){army=g_game->m_heroes[p.armySource].m_army;slot=p.armySlot;return;}
        const int town=-p.armySource-1;
        if(p.armySource<0&&town<int(g_game->m_towns.size())&&g_game->m_towns[town].m_owner==player){army=g_game->m_towns[town].getArmy();slot=p.armySlot;return;}
    }
    // Enemy right-click is disabled by the original owner check. Only its
    // public creature identity is needed for the portrait and rollover name.
    army.add(p.creature,1,-1);
}
void showPending(){
#ifdef __EMSCRIPTEN__
    const int length=EM_ASM_INT({return window.homm3Authority.guildWordsLength();});
    if(length<5||length>Guild::MaxWords){EM_ASM({window.homm3Authority.fail('Invalid guild report length');});return;}
    std::vector<int32_t> words(length);EM_ASM({window.homm3Authority.copyGuildWords(HEAP32,$0>>2);},words.data());
    try {
        install(Guild::decode(words.data(),words.size(),g_game->getLocalPlayerGamePos()));
        {TThievesGuildWindow window(report.tier);window.doModal(false);}
        clear();
    }catch(const std::exception&){clear();EM_ASM({window.homm3Authority.fail('Invalid filtered guild report');});}
#endif
}
void requestTown(int townId){AuthorityClient::request(39,townId);}
void requestTavern(){
    if(AuthorityClient::mapTavernChoiceActive()){AuthorityClient::chooseMapTavernGuild();return;}
    if(g_townManager->m_townToView)requestTown(g_townManager->m_townToView->m_id);
}
} }
