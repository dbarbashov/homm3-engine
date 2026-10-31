// Original rules fixture: human treasure choices, level-up and sequential turns.
#include "server_authority.h"
#include "server_guild.h"
#include "server_map_services.h"
#include "creaturetype.h"
#include "herotraits.h"
#include "sacrifice_window.h"
#include "hillfortwindow.h"
#include <algorithm>
#include "game.h"
#include "hero.h"
#include "town.h"
#include "advmgr.h"
#include "gzfile.h"
#include "resourcemanager.h"
#include "kb.h"
#include "prefs.h"
#include "soundmgr.h"
#include "winmgr.h"
#include "fast_game.h"
#include "recruit.h"
#include <iostream>
#include <unistd.h>
void getCategoryStats(int,long*,signed char*);void sortStats(long*,signed char*);
bool portableLoadGameData();void portableInitVars();void aiInitialize();void sRand(int);
namespace {void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
int main(int argc,char** argv){try{
    if(argc!=3||chdir(argv[1]))return 2;initMainClasses();sRand(42);g_noSound=1;
    g_config.m_soundVolume=0;g_config.m_musicVolume=0;g_config.m_blackoutComputer=1;
    ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);
    int error=0;check(ResourceManager::Open(true,true,&error)&&portableLoadGameData(),"rule resources");
    g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();g_numHumanPlayers=2;
    {TGzFile file(argv[2],"rb");g_game->initNewGame(1,-1,nullptr,&file);}
    for(int i=0;i<8;++i){auto& p=g_game->m_players[i];p.m_isLocal=0;p.m_isHuman=g_game->m_setup.m_playerPos[i]>=0&&g_game->m_setup.m_playerPos[i]<8;g_game->m_setup.m_startingHero[i]=-1;g_newMapStartingBonus[i]=0;}
    g_game->setupOrigData();{TGzFile file(argv[2],"rb");g_game->newMap(&file,nullptr,nullptr,2);}
    using namespace Port::Authority;
    setChoiceHandler([](const Choice& c){return Reply{c.options.front(),0,0};});
    start();const int actor=activeActor();apply(0,actor,0,0,0);
    auto& player=g_game->m_players[actor];hero& h=g_game->m_heroes[player.m_heroes[0]];
    g_advManager->setHeroContext(h.m_id,0,false,false);
    int enemy=-1;for(int i=0;i<8;++i)if(i!=actor&&!g_game->m_playerDisabled[i]){enemy=i;break;}check(enemy>=0,"guild opponent fixture");
    auto& opponent=g_game->m_players[enemy];check(opponent.m_numHeroes>0,"opponent hero fixture");
    hero& hidden=g_game->m_heroes[opponent.m_heroes[0]];
    opponent.m_resources[GOLD]=135791;hidden.m_army.initialize();hidden.m_army.add(CREATURE_ARCHANGEL,246802,-1);
    for(int skill=0;skill<4;++skill)hidden.setPrimarySkill(skill,41+skill);
    strcpy(hidden.m_name,"SecretName");opponent.m_personality=2;
    int checks=0;
    for(int tier=0;tier<=5;++tier){
        const auto view=guildView(actor,tier);const auto words=Port::Guild::encode(view);
        const auto decoded=Port::Guild::decode(words.data(),words.size(),actor);
        check(decoded.tier==tier&&int(decoded.ranks.size())==Port::Guild::categories(tier),"canonical guild information ladder");
        check(std::find(words.begin(),words.end(),135791)==words.end()&&std::find(words.begin(),words.end(),246802)==words.end(),"exact enemy resources and troop counts absent");
        const auto& shown=decoded.players[enemy];
        check(shown.armySource==-1&&shown.armySlot==-1,"enemy army location absent");
        if(tier==0)check(shown.hero==-1&&shown.name.empty(),"tier0 hides heroes");
        else check(shown.hero==hidden.m_id&&shown.name=="SecretName"&&shown.portrait==hidden.m_portrait,"strongest hero portrait identity");
        for(int skill=0;skill<4;++skill)check(shown.stats[skill]==(tier>=2?41+skill:0),"primary skills tier2 only");
        check(shown.personality==(tier>=3?2:-1),"personality tier3 only");
        check(shown.creature==(tier>=4?CREATURE_ARCHANGEL:-1),"strongest creature tier4 only");
        // Verify rank order and ties against the original canonical helpers.
        for(size_t category=0;category<view.ranks.size();++category){
            long values[8]={};signed char ids[8]={};getCategoryStats(category,values,ids);sortStats(values,ids);
            for(int i=0;i<view.count;++i){check(decoded.ranks[category].players[i]==ids[i],"original descending rank order");
                if(i>0&&values[i]>=0&&values[i-1]>=0)check((decoded.ranks[category].groups[i]==decoded.ranks[category].groups[i-1])==(values[i]==values[i-1]),"original ties retained");}
        }
        auto malformed=words;malformed.push_back(0);bool rejected=false;try{Port::Guild::decode(malformed.data(),malformed.size(),actor);}catch(const std::runtime_error&){rejected=true;}check(rejected,"extra guild fields rejected");
        rejected=false;try{Port::Guild::decode(words.data(),words.size(),enemy);}catch(const std::runtime_error&){rejected=true;}check(rejected,"guild recipient cannot change");++checks;
    }
    check(opponent.m_resources[GOLD]==135791&&hidden.m_army.m_numTroops[0]==246802,"guild inspection does not mutate world");
    int guildChoices=0;setChoiceHandler([&](const Choice& c){if(c.kind=="guild"){++guildChoices;check(c.actor==actor&&c.options==std::vector<int>{0},"guild view has only acknowledgement");check(c.metadata.find("135791")==std::string::npos&&c.metadata.find("246802")==std::string::npos,"wire metadata filters totals");}return Reply{c.options.front(),0,0};});
    guildEncounter(actor,true);check(guildChoices==1,"den guild boundary");++checks;
    const int ownTown=player.m_townIds[0];townGuildEncounter(actor,ownTown);check(guildChoices==2,"town guild boundary");++checks;
    bool rejected=false;try{townGuildEncounter(enemy,ownTown);}catch(const Rejected&){rejected=true;}check(rejected,"enemy town access denied");++checks;
    int tavernChoices=0;setChoiceHandler([&](const Choice& c){if(c.kind=="guild"){++guildChoices;return Reply{0,0,0};}check(c.kind=="map_tavern","nested guild preserves tavern");return tavernChoices++==0?Reply{-2,0,0}:Reply{-1,0,0};});
    mapTavernEncounter(h,h.getLocation());check(tavernChoices==2&&guildChoices==3,"map tavern guild returns to suspended chooser");++checks;
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"guildChoices\":"<<guildChoices<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
