// Original rules fixture: human treasure choices, level-up and sequential turns.
#include "server_authority.h"
#include "server_terminal.h"
#include "server_projection.h"
#include "server_encounters.h"
#include "server_market.h"
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
#include <sstream>
#include <unistd.h>
#include <execinfo.h>
#include <csignal>
bool portableLoadGameData();void portableInitVars();void aiInitialize();void sRand(int);
namespace {void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
int main(int argc,char** argv){std::signal(SIGSEGV,[](int){void* frames[64];int n=backtrace(frames,64);backtrace_symbols_fd(frames,n,2);_Exit(139);});try{
    if(argc!=3||chdir(argv[1]))return 2;initMainClasses();sRand(42);g_noSound=1;
    g_config.m_soundVolume=0;g_config.m_musicVolume=0;g_config.m_blackoutComputer=1;
    ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);
    int error=0;check(ResourceManager::Open(true,true,&error)&&portableLoadGameData(),"rule resources");
    g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();g_numHumanPlayers=2;
    {TGzFile file(argv[2],"rb");g_game->initNewGame(1,-1,nullptr,&file);}
    for(int i=0;i<8;++i){auto& p=g_game->m_players[i];p.m_isLocal=0;p.m_isHuman=g_game->m_setup.m_playerPos[i]>=0&&g_game->m_setup.m_playerPos[i]<8;g_game->m_setup.m_startingHero[i]=-1;g_newMapStartingBonus[i]=0;}
    g_game->setupOrigData();{TGzFile file(argv[2],"rb");g_game->newMap(&file,nullptr,nullptr,2);}
    int decisions=0;
    Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){++decisions;return Port::Authority::Reply{c.options.front(),0,0};});
    Port::Authority::start();const int first=Port::Authority::activeActor();
    Port::Authority::apply(0,first,0,0,0);
    int second=-1,ai=-1;for(int i=0;i<8;++i){if(i!=first&&g_game->isHuman(i)&&!g_game->m_playerDisabled[i])second=i;else if(!g_game->isHuman(i)&&!g_game->m_playerDisabled[i])ai=i;}
    check(second>=0,"two human seats");
    if(ai<0){for(int i=0;i<8;++i)if(i!=first&&i!=second){ai=i;break;}g_game->m_playerDisabled[ai]=0;g_game->m_players[ai].m_isHuman=0;g_game->m_players[ai].m_numHeroes=1;g_game->m_players[ai].m_numTowns=1;}
    auto& loss=g_game->m_mapHeader.m_lossCondition;loss.m_type=LOSS_CONDITION_LOSE_HERO;loss.m_heroId=g_game->m_players[first].m_heroes[0];loss.m_playerLoser=first;loss.m_gameLost=1;
    const int before=decisions;checkEndGame(0);
    check(g_game->m_playerDisabled[first]&&!g_game->m_playerDisabled[second]&&!g_gameOver,"one special loss must not end surviving human plus AI match");
    check(Port::Authority::outcome(first)==-1&&Port::Authority::outcome(second)==0&&decisions==before,"loss notice cannot block worker on eliminated human");
    std::ostringstream projection;Port::Authority::writeProjection(projection,first,Port::Authority::revision());
    check(projection.str().find("\"eliminated\":true")!=std::string::npos&&projection.str().find("\"outcome\":-1")!=std::string::npos,"eliminated player retains valid private final projection");
    Port::Authority::Choice acknowledgement;acknowledgement.actor=first;acknowledgement.kind="normal";acknowledgement.options={DIALOG_RETURN_OK};
    check(Port::Authority::choose(acknowledgement).value==DIALOG_RETURN_OK,"disabled actor may acknowledge informational original dialog");
    acknowledgement.options.push_back(DIALOG_RETURN_CANCEL);bool denied=false;try{Port::Authority::choose(acknowledgement);}catch(const std::runtime_error&){denied=true;}check(denied,"disabled actor cannot make a mutable choice");
    loss.m_playerLoser=second;loss.m_heroId=g_game->m_players[second].m_heroes[0];checkEndGame(0);
    check(g_gameOver&&Port::Authority::outcome(second)==-1,"all humans eliminated finishes worker even with AI alive");
    std::cout<<"{\"passed\":true,\"checks\":6}\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
