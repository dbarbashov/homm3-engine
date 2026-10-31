// Original rules fixture: human treasure choices, level-up and sequential turns.
#include "server_authority.h"
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
    int choices=0,levels=0;bool experience=false;uint64_t token=0;
    auto originalDecisions=[&](const Port::Authority::Choice& c){
        check(Port::Authority::phase()==Port::Authority::Phase::Choice,"stack waits in choice phase");
        check(!Port::Fast::enabled()&&!Port::Fast::executing(),"ordinary never uses Fast policy");
        check(!g_advManager->m_advWindow,"no server adventure UI");
        check(c.token>token,"choice tokens increase");token=c.token;++choices;
        if(c.kind=="level_up")++levels;
        Port::Authority::Reply reply;reply.value=c.options.front();
        if(c.kind=="normal"&&c.options.size()==2&&c.options[0]==DIALOG_RETURN_CHOICE_1)
            reply.value=experience?DIALOG_RETURN_CHOICE_2:DIALOG_RETURN_CHOICE_1;
        return reply;
    };Port::Authority::setChoiceHandler(originalDecisions);
    Port::Authority::start();int first=Port::Authority::activeActor();
    bool rejected=false;try{Port::Authority::apply(5,first,0,0,0);}catch(const Port::Authority::Rejected&){rejected=true;}check(rejected&&Port::Authority::revision()==0,"bootstrap required");
    Port::Authority::apply(0,first,0,0,0);check(Port::Authority::revision()==1,"bootstrap commits once");
    int id=g_game->m_players[first].m_heroes[0];hero& h=g_game->m_heroes[id];g_advManager->setHeroContext(id,0,false,false);
    long gold=g_game->m_players[first].m_resources[GOLD];int xp=h.m_experience;int before=choices;
    NewmapCell chest;chest.m_type=TREASURE_CHEST;chest.m_objectTypeIndex=-1;chest.m_extraInfo=0;chest.m_treasureInfo.m_gold=2;
    g_advManager->doEvent(&chest,h.getLocation());
    check(choices==before+1&&g_game->m_players[first].m_resources[GOLD]==gold+1000&&h.m_experience==xp,"human chest gold decision");
    experience=true;gold=g_game->m_players[first].m_resources[GOLD];
    h.m_experience=hero::getExperience(h.m_level+1)-1;
    g_advManager->doEvent(&chest,h.getLocation());h.checkLevel();
    check(g_game->m_players[first].m_resources[GOLD]==gold&&levels>0,"experience decision opens explicit original level choice");
    {
        // A suspended dwelling owns its population pointer and prices. Invalid
        // quantities are rejected before any canonical state is touched.
        int creature=h.m_army.m_armies[0],cost[7];getMonsterCost(creature,cost);
        for(int r=0;r<7;++r)g_game->m_players[first].m_resources[r]+=10000;
        short population=5;int creatures[4]={creature,-1,-1,-1};short* available[4]={&population,0,0,0};
        const long previousCount=h.m_army.getCreatureTotal(TCreatureType(creature));const int previousGold=g_game->m_players[first].m_resources[6];int calls=0;
        Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){
            check(c.kind=="recruit"&&bool(c.validate),"typed recruitment callback");
            if(calls++>0)return Port::Authority::Reply{-1,0,0};
            check(!c.validate({creature,6,0})&&!c.validate({creature,-1,0})&&!c.validate({creature,1,1}),"recruit bounds checked without mutation");
            check(population==5&&h.m_army.getCreatureTotal(TCreatureType(creature))==previousCount,"invalid recruit did not change world");
            return Port::Authority::Reply{creature,2,0};
        });
        Port::Authority::recruitEncounter(h,creatures,available);
        check(population==3&&h.m_army.getCreatureTotal(TCreatureType(creature))==previousCount+2&&g_game->m_players[first].m_resources[6]==previousGold-cost[6]*2,"recruit canonical price, army and population");
        armyGroup reward;reward.initialize();reward.add(creature,3,-1);calls=0;
        Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){
            check(c.kind=="army_exchange"&&bool(c.validate),"typed joining army callback");
            if(calls++>0)return Port::Authority::Reply{0,0,0};
            check(!c.validate({2,4,0})&&!c.validate({2,1,49}),"joining army count and slot bounds");
            return Port::Authority::Reply{2,3,0};
        });
        Port::Authority::armyEncounter(h,reward,true,true);
        check(!reward.hasCreatures()&&h.m_army.getCreatureTotal(TCreatureType(creature))==previousCount+5,"joining stack transferred canonically");
        const int marketGold=g_game->m_players[first].m_resources[GOLD],marketWood=g_game->m_players[first].m_resources[WOOD];calls=0;
        Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){
            check(c.kind=="market"&&bool(c.validate),"typed market callback");
            if(calls++>0)return Port::Authority::Reply{0,0,0};
            check(!c.validate({1,-1,42})&&!c.validate({1,1,48})&&!c.validate({3,0,0}),"market rejects negative, self-trade and unavailable artifact tab");
            return Port::Authority::Reply{1,2,42}; // Two wood, original trading-post efficiency five.
        });
        Port::Authority::marketEncounter(&h,1,5,0,nullptr);
        check(g_game->m_players[first].m_resources[WOOD]==marketWood+2&&g_game->m_players[first].m_resources[GOLD]==marketGold-1666,"market exact original float rounding and server price");
        hero* hidden=nullptr;for(auto& other:g_game->m_heroes)if(other.m_x<g_mapWidth&&other.m_y<g_mapHeight&&other.m_z<g_game->getNumMapLevels()&&other.m_owner>=0&&other.m_owner!=first){hidden=&other;break;}
        check(hidden!=nullptr,"fixture has an enemy hero");const int hx=hidden->m_x,hy=hidden->m_y,hz=hidden->m_z;auto* visibility=getMapExtraPtr(hx,hy,hz);auto oldVisibility=*visibility;*visibility&=~(1<<first);
        bool containsHidden=false;
        Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){
            check(c.kind=="world_view","typed world-view callback");
            const auto start=c.metadata.find("\"markers\":[");check(start!=std::string::npos,"world marker payload");
            std::string values=c.metadata.substr(start+11);for(char& ch:values)if(ch==','||ch==']'||ch=='}')ch=' ';
            std::istringstream input(values);int x,y,z,type,index,owner;containsHidden=false;
            while(input>>x>>y>>z>>type>>index>>owner)if(x==hx&&y==hy&&z==hz&&type==HERO)containsHidden=true;
            return Port::Authority::Reply{0,0,0};
        });
        Port::Authority::worldViewEncounter(h,SPELL_VIEW_AIR,eMasteryBasic);check(!containsHidden,"basic View Air does not reveal enemy hero location");
        Port::Authority::worldViewEncounter(h,SPELL_VIEW_AIR,eMasteryAdvanced);check(containsHidden,"advanced View Air reveals allowed hero symbol");*visibility=oldVisibility;
        Port::Authority::setChoiceHandler(originalDecisions);
    }
    g_advManager->demobilizeCurrHero(false,false);
    int day=Port::Authority::day();Port::Authority::apply(5,first,0,0,0);int second=Port::Authority::activeActor();
    check(second!=first&&Port::Authority::day()==day,"end turn transfers to next human without advancing day");
    auto rev=Port::Authority::revision();rejected=false;try{Port::Authority::apply(5,first,0,0,0);}catch(const Port::Authority::Rejected&){rejected=true;}
    check(rejected&&Port::Authority::revision()==rev&&Port::Authority::activeActor()==second,"inactive actor cannot end another turn");
    Port::Authority::apply(5,second,0,0,0);
    check(Port::Authority::activeActor()==first&&Port::Authority::day()==day+1,"full turn cycle advances original day once");
    std::cout<<"{\"passed\":true,\"choices\":"<<choices<<",\"levelChoices\":"<<levels<<",\"revision\":"<<Port::Authority::revision()<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<"authority-session fixture: "<<e.what()<<'\n';return 1;}}
