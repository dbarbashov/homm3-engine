// Verify the actual writer/reader against original object fragments, without
// exposing hidden cells or relying on screenshots and animation timing.
#include "server_authority.h"
#include "server_projection.h"
#include "game.h"
#include "hero.h"
#include "advmgr.h"
#include "gzfile.h"
#include "resourcemanager.h"
#include "kb.h"
#include "prefs.h"
#include "soundmgr.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <array>
#include <unistd.h>
bool portableLoadGameData();void portableInitVars();void aiInitialize();void sRand(int);
namespace {
void check(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
using Ref=std::array<int,3>;
std::vector<Ref> refs(const NewmapCell& c){std::vector<Ref> out;for(const auto& r:c.m_objects)out.push_back({int(r.m_objectIndex),int(r.m_offsets),int(r.m_layer)});return out;}
}
int main(int argc,char** argv){try{
    if(argc<3||chdir(argv[1]))return 2;
    initMainClasses();sRand(42);g_noSound=1;g_config.m_soundVolume=0;g_config.m_musicVolume=0;g_config.m_blackoutComputer=1;
    ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);
    int error=0;check(ResourceManager::Open(true,true,&error)&&portableLoadGameData(),"resources");
    g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();g_numHumanPlayers=2;
    {TGzFile file(argv[2],"rb");g_game->initNewGame(1,-1,nullptr,&file);}
    for(int i=0;i<8;++i){auto& p=g_game->m_players[i];p.m_isLocal=0;p.m_isHuman=g_game->m_setup.m_playerPos[i]>=0&&g_game->m_setup.m_playerPos[i]<8;g_game->m_setup.m_startingHero[i]=-1;g_newMapStartingBonus[i]=0;}
    g_game->setupOrigData();{TGzFile file(argv[2],"rb");g_game->newMap(&file,nullptr,nullptr,2);}
    using namespace Port::Authority;
    setChoiceHandler([](const Choice& c){return Reply{c.options.front(),0,0};});start();const int actor=activeActor();apply(0,actor,0,0,0);
    const int bit=1<<actor,levels=g_game->getNumMapLevels();auto& map=g_game->m_worldMap;
    auto visible=[&](int x,int y,int z){return x>=0&&y>=0&&x<g_mapWidth&&y<g_mapHeight&&(*getMapExtraPtr(x,y,z)&bit);};
    std::vector<bool> known(map.m_objects.size(),false),knownHeroes(game::HERO_COUNT,false);
    for(int z=0;z<levels;++z)for(int y=0;y<g_mapHeight;++y)for(int x=0;x<g_mapWidth;++x)if(visible(x,y,z))for(const auto& r:map.cell(x,y,z)->m_objects)known[r.m_objectIndex]=true;
    const auto& player=g_game->m_players[actor];
    for(const auto& h:g_game->m_heroes)knownHeroes[h.m_id]=h.m_owner==actor||h.m_id==player.m_recruits[0]||h.m_id==player.m_recruits[1]||(h.isOnMap()&&visible(h.m_x,h.m_y,h.m_z));
    struct Cell {int x,y,z;bool visible;std::vector<Ref> refs;};std::vector<Cell> expected;
    int edgeCells=0,fragments=0,hidden=0,hiddenHeroes=0;
    for(int z=0;z<levels;++z)for(int y=0;y<g_mapHeight;++y)for(int x=0;x<g_mapWidth;++x){
        Cell c{x,y,z,visible(x,y,z)!=0,{}};bool fringe=false;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)fringe|=visible(x+dx,y+dy,z)!=0;
        for(auto r:refs(*map.cell(x,y,z)))if(c.visible||(fringe&&known[r[0]]))c.refs.push_back(r);
        if(!c.visible){++hidden;if(!c.refs.empty()){++edgeCells;fragments+=c.refs.size();}}
        expected.push_back(c);
    }
    check(edgeCells>0&&fragments>0&&hidden>0,"fixture must cross scenery at fog boundary");
    std::ostringstream output;writeProjection(output,actor,1);const auto json=output.str();
    const auto start=json.find("\"words\":[");check(start!=std::string::npos,"projection words");
    const auto end=json.find(']',start);std::istringstream input(json.substr(start+9,end-start-9));std::vector<int32_t> words;long long value;
    while(input>>value){words.push_back(static_cast<int32_t>(value));if(input.peek()==',')input.get();}
    check(AuthorityProjectionAccess::read(words.data(),words.size(),false),"validate projection");
    check(AuthorityProjectionAccess::read(words.data(),words.size(),true),"apply projection");
    // Fault injection proves the original missing-fragment bug fails this gate.
    if(argc>3)for(const auto& c:expected)if(!c.visible)map.cell(c.x,c.y,c.z)->m_objects.clear();
    for(const auto& c:expected){const auto& actual=*map.cell(c.x,c.y,c.z);
        check(visible(c.x,c.y,c.z)==c.visible,"fog visibility changed");
        check(refs(actual)==c.refs,"scenery fragments differ at fog edge");
        if(!c.visible)check(actual.m_type==NOTHING&&actual.m_extraInfo==0&&actual.m_objectIndex==-1&&actual.m_objectTypeIndex==-1&&!actual.m_isTrigger,"hidden gameplay cell leaked");
    }
    for(size_t i=0;i<known.size();++i)if(!known[i])check(map.m_objects[i].m_typeIndex==65535&&map.m_objects[i].m_extraInfo==0,"unknown object leaked");
    for(const auto& h:g_game->m_heroes)if(!knownHeroes[h.m_id]){++hiddenHeroes;check(!h.isOnMap()&&h.m_owner==-1&&h.m_x==-1&&h.m_y==-1,"hidden hero leaked");}
    std::cout<<"{\"passed\":true,\"cells\":"<<expected.size()<<",\"hiddenCells\":"<<hidden<<",\"edgeCells\":"<<edgeCells<<",\"fragments\":"<<fragments<<",\"hiddenHeroes\":"<<hiddenHeroes<<"}\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
