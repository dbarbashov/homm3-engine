// Original rules fixture: human treasure choices, level-up and sequential turns.
#include "server_authority.h"
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
    int checks=0;type_point point=h.getLocation();point.m_x+=2;
    auto* water=g_game->getCell(point);water->m_groundSet=eTerrainWater;
    const int id=g_game->createBoat(point.m_x,point.m_y,point.m_z,actor,false,0);check(id>=0,"boat created");
    auto* vessel=g_game->getBoat(id);h.m_movePoints=1000;h.m_flags&=~0x1040000;
    NewmapCell cell;cell.m_type=BOAT;cell.m_extraInfo=id;cell.m_objectTypeIndex=-1;
    g_advManager->doEvent(&cell,point);
    check(!g_advManager->m_advWindow&&(h.m_flags&0x40000)&&h.m_movePoints==0,"headless embark consumes movement");
    check(vessel->m_occupied&&vessel->m_occupyingHero==h.m_id&&vessel->m_playerOwner==actor,"canonical boat occupancy");++checks;
    h.m_movePoints=1000;cell.m_type=ANCHOR_POINT;
    g_advManager->doEvent(&cell,h.getLocation());
    check(!(h.m_flags&0x40000)&&h.m_movePoints==0,"headless disembark consumes movement");++checks;
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
