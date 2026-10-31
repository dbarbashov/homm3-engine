#include "parity_wait.h"
#include "authority_bootstrap.h"
#include "authority_client.h"
#include "server_projection.h"
#include "fast_game.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "advmgr.h"
#include "gzfile.h"
#include "kb.h"
#include "remote.h"
#include "newgame.h"
#include "netgame.h"
#include <string>
#include <vector>
#include <stdexcept>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
void portableAdventureLoop(bool newGame);
namespace Port { namespace AuthorityBootstrap {
void run(){
#ifdef __EMSCRIPTEN__
    char mapName[512]={};int settings[42]={};
    const bool valid=EM_ASM_INT({
        const r=window.homm3AuthorityRestore;
        if(!r || typeof r.map!=='string' || !r.map.length || r.map.length>250 ||
           !Array.isArray(r.setup) || r.setup.length!==41 || !r.setup.every(Number.isInteger) ||
           !Number.isInteger(r.actor) || r.actor<0 || r.actor>7)return 0;
        stringToUTF8(r.map,$0,512);HEAP32.set([r.actor,...r.setup],$1>>2);return 1;
    },mapName,settings);
    if(!valid)throw std::runtime_error("Missing authoritative restore metadata");
    const int actor=settings[0];const int* setup=settings+1;
    std::string map=mapName;
    if(map.compare(0,5,"maps/")==0)map.erase(0,5);
    if(map.empty()||map=="."||map==".."||map.find('/')!=std::string::npos||map.find('\\')!=std::string::npos||map.find(':')!=std::string::npos)
        throw std::runtime_error("Invalid restore map name");
    std::string path="maps/"+map;
    int humans=0;for(int i=0;i<8;++i)if(setup[1+i*5]>=0&&setup[1+i*5]<8)++humans;
    if(humans<2||humans>8||setup[0]<0||setup[0]>4)throw std::runtime_error("Invalid restore setup");
    // This instance loads only the original map's presentation catalogues.
    // Authority state arrives independently and replaces every disclosed live
    // pool before the original adventure window is opened.
    g_numHumanPlayers=humans;g_mpNetProtocol=MP_TCP;g_remoteOn=0;g_goSolo=0;g_inCampaign=0;g_game->m_isTutorial=0;
    {TGzFile input(path.c_str(),"rb");g_game->initNewGame(setup[0],-1,nullptr,&input);}
    std::strncpy(g_game->m_setup.m_filename,map.c_str(),sizeof(g_game->m_setup.m_filename)-1);
    std::strcpy(g_game->m_setup.m_path,"maps/");
    for(int color=0;color<8;++color){const int* value=setup+1+color*5;
        g_game->m_setup.m_playerPos[color]=value[0];
        if(value[1]>=0&&value[1]<9)g_game->m_setup.m_alignment[color]=static_cast<TTownType>(value[1]);
        g_game->m_setup.m_startingHero[color]=value[2];g_newMapStartingBonus[color]=value[3];g_game->m_setup.m_startingBonus[color]=value[3];g_game->m_setup.m_handicap[color]=value[4];
        g_game->m_players[color].m_isHuman=value[0]>=0&&value[0]<8;g_game->m_players[color].m_isLocal=color==actor;
    }
    g_game->m_setup.m_difficulty=setup[0];g_game->setupOrigData();
    {TGzFile input(path.c_str(),"rb");g_game->newMap(&input,nullptr,nullptr,2);}
    for(int color=0;color<8;++color){
        const int seat=setup[1+color*5];if(seat<0||seat>=8)continue;
        auto& player=g_game->m_players[color];
        player.m_dpid=EM_ASM_INT({return window.homm3Network?.players[$0]?.dpid||0;},seat);
        EM_ASM({const p=window.homm3Network?.players[$0];if(!p)return;const codes=Array.from(p.name).slice(0,20).map(c=>{const n=c.charCodeAt(0);return n<128?n:n>=0x410&&n<=0x44f?n-0x410+0xc0:n===0x401?0xa8:n===0x451?0xb8:63;});HEAPU8.set(codes,$1);HEAPU8[$1+codes.length]=0;},seat,player.m_name);
    }
    // Test fixture must precede projection installation, including restore.
    Port::Harness::loadFixture();
    const double deadline=emscripten_get_now()+30000;
    int length=0;
    while(!(length=EM_ASM_INT({return window.homm3Authority.pendingLength();}))){
        if(emscripten_get_now()>deadline)throw std::runtime_error("No authoritative restore projection");
        Port::Harness::wait(8);
    }
    if(length<0||length>Authority::ProjectionMaxWords)throw std::runtime_error("Restore projection limit");
    std::vector<int32_t> words(length);
    EM_ASM({window.homm3Authority.copyPending(HEAP32,$0>>2);},words.data());
    if(words[1]!=actor || !AuthorityProjectionAccess::read(words.data(),words.size(),false) || !AuthorityProjectionAccess::read(words.data(),words.size(),true))
        throw std::runtime_error("Restore projection does not match selected map");
    EM_ASM({window.homm3Authority.applied();});
    g_localGamePos=actor;g_netLocalGamePos=actor;g_playerTurn=words[2];g_currentPlayer=&g_game->m_players[actor];g_curPlayerBit=1<<actor;g_curWatchPlayer=actor;g_mapVisibilityBit=1<<actor;
    g_currentPlayer->m_isLocal=1;g_currentPlayer->m_isHuman=1;g_thisNetGotAdventureControl=1;
    g_currentPlayer->m_currHeroId=g_currentPlayer->m_numHeroes?g_currentPlayer->m_heroes[0]:static_cast<HeroId>(-1);
    g_currentPlayer->m_currTownId=g_currentPlayer->m_numTowns?g_currentPlayer->m_townIds[0]:-1;
    Fast::state().ready=g_playerTurn!=actor;
    g_gameCommand=-1;g_gameOver=0;
    portableAdventureLoop(false);
#else
    throw std::runtime_error("Authority browser restore requires WebAssembly");
#endif
}
} }
