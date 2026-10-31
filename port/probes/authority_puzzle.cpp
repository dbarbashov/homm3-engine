// Native semantic-action fixtures over the real original tables and map.
#include "server_authority.h"
#include "server_exchange.h"
#include "server_puzzle.h"
#include "server_projection.h"
#include <sstream>
#include "server_encounters.h"
#include "worker_memory.h"
#include "puzzlewindow.h"
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
#include "castle.h"
#include "recruit.h"
#include "creaturetype.h"
#include <iostream>
#include <unistd.h>
bool portableLoadGameData();void portableInitVars();void aiInitialize();void sRand(int);
namespace {int checks=0;void check(bool value,const char* why){if(!value)throw std::runtime_error(why);++checks;}}
int main(int argc,char** argv){try{
    if((argc!=3&&argc!=4)||chdir(argv[1]))return 2;initMainClasses();sRand(42);g_noSound=1;
    g_config.m_soundVolume=0;g_config.m_musicVolume=0;g_config.m_blackoutComputer=1;
    ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);
    int error=0;check(ResourceManager::Open(true,true,&error)&&portableLoadGameData(),"rule resources");
    g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();g_numHumanPlayers=2;
    {TGzFile file(argv[2],"rb");g_game->initNewGame(1,-1,nullptr,&file);}
    for(int i=0;i<8;++i){auto& p=g_game->m_players[i];p.m_isLocal=0;p.m_isHuman=g_game->m_setup.m_playerPos[i]>=0&&g_game->m_setup.m_playerPos[i]<8;g_game->m_setup.m_startingHero[i]=-1;g_newMapStartingBonus[i]=0;}
    g_game->setupOrigData();{TGzFile file(argv[2],"rb");g_game->newMap(&file,nullptr,nullptr,2);}

    auto acknowledge=[](const Port::Authority::Choice& c){return Port::Authority::Reply{c.options.front(),0,0};};
    Port::Authority::setChoiceHandler(acknowledge);Port::Authority::start();const int actor=Port::Authority::activeActor();Port::Authority::apply(0,actor,0,0,0);
    const auto before=Port::Authority::memory().rss;
    if(argc==4){
        g_game->m_numObelisks=4;for(int i=0;i<4;++i)g_game->m_obeliskFlags[i]=1<<actor;g_game->m_ultimateArtifactX=16;g_game->m_ultimateArtifactY=18;g_game->m_ultimateArtifactZ=0;
        const auto beforePeak=Port::Authority::memory().peakRss;uint64_t pending=0;size_t wire=0;
        Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){pending=Port::Authority::memory().rss;wire=c.metadata.size();return Port::Authority::Reply{0,0,0};});
        Port::Authority::puzzleEncounter(actor);const auto after=Port::Authority::memory();
        std::cout<<"{\"passed\":true,\"rssBefore\":"<<before<<",\"rssPending\":"<<pending<<",\"rssAfter\":"<<after.rss<<",\"peakBefore\":"<<beforePeak<<",\"peakRss\":"<<after.peakRss<<",\"choiceBytes\":"<<wire<<"}\n";return 0;
    }

    auto* oldScreen=g_windowManager->m_screenBitmap;const int oldPuzzle=g_drawingPuzzle;
    const auto oldPieces=g_puzzlePiecesRemoved;
    g_game->m_numObelisks=4;for(auto& flag:g_game->m_obeliskFlags)flag=0;
    g_game->m_ultimateArtifactX=16;g_game->m_ultimateArtifactY=18;g_game->m_ultimateArtifactZ=0;
    {
    auto visitWords=[](int viewer){std::ostringstream out;Port::Authority::writeProjection(out,viewer,0);const auto text=out.str();auto start=text.find("\"words\":[")+9;std::istringstream in(text.substr(start,text.find(']',start)-start));std::vector<int> values;int n;char comma;while(in>>n){values.push_back(n);in>>comma;}return std::vector<int>(values.end()-48,values.end());};
    g_game->m_obeliskFlags[0]=1;g_game->m_obeliskFlags[1]=2;
    const auto redVisits=visitWords(0),blueVisits=visitWords(1);
    check(redVisits[0]==1&&redVisits[1]==0&&blueVisits[0]==0&&blueVisits[1]==1,"obelisk projection contains only viewer visitation bits");
    for(auto& flag:g_game->m_obeliskFlags)flag=0;
    const auto hidden=Port::Authority::puzzleProjection(actor);
    check(hidden.size()<900000&&hidden.find("\"width\":592")!=std::string::npos,"bounded RGB565 puzzle");
    check(hidden.find("origin")==std::string::npos&&hidden.find("grail")==std::string::npos,"no canonical coordinates");
    g_game->m_ultimateArtifactX=25;g_game->m_ultimateArtifactY=29;
    const auto otherHidden=Port::Authority::puzzleProjection(actor);
    check(hidden==otherHidden,"opaque pieces conceal every secret pixel");
    g_game->m_obeliskFlags[0]=1<<actor;
    g_game->m_numObelisks=0;check(Port::Authority::puzzleProjection(actor)==hidden,"zero obelisks reveal no terrain");g_game->m_numObelisks=4;
    const auto partial=Port::Authority::puzzleProjection(actor);check(partial!=hidden,"visited obelisk reveals original pixels");
    check(hidden.find("\"revealed\":0")!=std::string::npos&&partial.find("\"revealed\":7")!=std::string::npos,"puzzle animation uses actual reveal count");
    check(Port::Authority::puzzleProjection(actor)==partial,"puzzle deterministic");
    for(int i=0;i<4;++i)g_game->m_obeliskFlags[i]=1<<actor;
    const auto full=Port::Authority::puzzleProjection(actor);check(full!=partial,"full puzzle reveal");
    check(g_windowManager->m_screenBitmap==oldScreen&&g_drawingPuzzle==oldPuzzle&&g_puzzlePiecesRemoved==oldPieces,"transient presentation state restored");
    int choices=0;Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){check(c.kind=="puzzle"&&c.actor==actor&&c.options==std::vector<int>{0},"typed puzzle decision");++choices;return Port::Authority::Reply{0,0,0};});
    const auto revision=Port::Authority::revision();
    Port::Authority::apply(40,actor,0,0,0);check(choices==1&&Port::Authority::revision()==revision+1,"original stack continues after one committed puzzle intent");
    bool malformed=false;try{Port::Authority::apply(40,actor,0,1,0);}catch(const Port::Authority::Rejected&){malformed=true;}check(malformed&&Port::Authority::revision()==revision+1,"malformed puzzle command cannot advance revision");
    const auto savedAlignment=g_game->m_setup.m_alignment[actor];for(auto& flag:g_game->m_obeliskFlags)flag=0;
    for(int alignment=0;alignment<9;++alignment){g_game->m_setup.m_alignment[actor]=static_cast<TTownType>(alignment);g_game->m_ultimateArtifactX=16;const auto first=Port::Authority::puzzleProjection(actor);g_game->m_ultimateArtifactX=25;check(first==Port::Authority::puzzleProjection(actor),"all nine faction masks conceal secret terrain");}
    g_game->m_setup.m_alignment[actor]=savedAlignment;
    sRand(12345);const int expectedRandom=rand();sRand(12345);
    Port::Authority::setChoiceHandler([](const Port::Authority::Choice&){return Port::Authority::Reply{0,0,0};});
    hero& viewer=g_game->m_heroes[g_game->m_players[actor].m_heroes[0]];
    Port::Authority::worldViewEncounter(viewer,SPELL_VIEW_EARTH,eMasteryBasic);check(rand()==expectedRandom,"world view without puzzle does not reseed gameplay RNG");
    int nested=0;uint64_t lastToken=0;
    Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){check(c.token>lastToken,"nested puzzle tokens advance");lastToken=c.token;++nested;
        if(nested==1){check(c.kind=="world_view"&&c.options==std::vector<int>({0,1})&&c.metadata.find("pixels")==std::string::npos,"world view offers lazy puzzle without rendering it");return Port::Authority::Reply{1,0,0};}
        check(c.kind==(nested==2?"puzzle":"world_view"),"nested puzzle then same world view");return Port::Authority::Reply{0,0,0};});
    Port::Authority::worldViewEncounter(viewer,SPELL_VIEW_AIR,eMasteryAdvanced);check(nested==3,"world view resumes after puzzle acknowledgement");
    bool rejected=false;try{Port::Authority::puzzleProjection(-1);}catch(const Port::Authority::Rejected&){rejected=true;}check(rejected,"invalid viewer rejected");
    }
    const auto after=Port::Authority::memory();
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"rssBefore\":"<<before<<",\"rssAfter\":"<<after.rss<<",\"peakRss\":"<<after.peakRss<<"}\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
