// Original monster encounter: last hero retreats, remaining town is selected.
#include "server_authority.h"
#include "server_combat.h"
#include <regex>
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
    int decisions=0,results=0;
    setChoiceHandler([&](const Choice& c){
        if(c.kind!="combat")return Reply{c.options.front(),0,0};
        const int sequence[]={3,103,104,4};
        check(decisions<4,"unexpected combat decision");
        const int desired=sequence[decisions++];
        std::regex pattern("\\{\\\"id\\\":([0-9]+),\\\"action\\\":([0-9]+),");
        for(std::sregex_iterator i(c.metadata.begin(),c.metadata.end(),pattern),end;i!=end;++i)
            if(std::stoi((*i)[2])==desired)return Reply{std::stoi((*i)[1]),0,0};
        throw std::runtime_error("expected defend/auto/stop/retreat unavailable");
    });
    setCombatResultHandler([&](int actor,const std::string& data){
        check(actor==activeActor()&&data.find("\"retreated\":true")!=std::string::npos,"private retreat result");++results;
    });
    start();const int actor=activeActor();apply(0,actor,0,0,0);
    auto& player=g_game->m_players[actor];
    check(player.m_numHeroes==1&&player.m_numTowns>0,"fixture needs one hero and a remaining town");
    hero& h=g_game->m_heroes[player.m_heroes[0]];const int heroId=h.m_id;
    h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,200,-1);
    h.m_skillLevel[eSecSkillBattleTactics]=0;h.m_formation&=~2;
    g_config.m_combatAutoCreatures=1;
    g_advManager->setHeroContext(heroId,0,false,false);
    NewmapCell cell;cell.m_type=MONSTER;cell.m_objectIndex=CREATURE_PIKEMAN;cell.m_monsterInfo.m_qty=50;
    type_point point=h.getLocation();point.m_x+=2;
    int monsters=50;
    g_advManager->combatMonsterEvent(&h,CREATURE_PIKEMAN,&monsters,&cell,point,CREATURE_NONE,0,0,CREATURE_NONE,0,0);
    check(decisions==4&&results==1,"manual decisions and result delivered");
    check(player.m_numHeroes==0&&h.m_owner<0,"last hero removed from map");
    check(player.m_recruits[0]==heroId||player.m_recruits[1]==heroId,"retreated hero remains recruitable");
    check(player.m_currHeroId==-1&&player.m_currTownId>=0,"remaining town selected");
    check(player.m_townIds[0]==player.m_currTownId&&!g_advManager->m_advWindow,"headless town context");
    check(!g_game->m_playerDisabled[actor]&&phase()==Phase::Adventure,"player remains active with town");
    std::cout<<"{\"passed\":true,\"checks\":6,\"combatDecisions\":"<<decisions<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
