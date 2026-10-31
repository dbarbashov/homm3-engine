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
    int checks=0,levelChoices=0;
    auto dispatchService=[&](TAdventureObjectType type,type_point point){
        NewmapCell cell;cell.m_type=type;cell.m_objectTypeIndex=-1;cell.m_extraInfo=0;
        g_advManager->doEvent(&cell,point);
    };
    for(int r=0;r<7;++r)player.m_resources[r]=100000;
    // Level one is free; higher levels discount only gold, using retail table.
    h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,10,-1);h.m_army.add(TCreatureType(12),2,-1);
    long fullCost[7];getUpgradeCost(TCreatureType(12),g_game->upgradedCreatureType(TCreatureType(12)),2,fullCost);
    const int beforeGold=player.m_resources[GOLD];int calls=0;
    setChoiceHandler([&](const Choice& c){
        check(c.kind=="hill_fort","hill-fort kind");
        if(calls++==0){check(std::find(c.options.begin(),c.options.end(),7)!=c.options.end(),"upgrade all offered");return Reply{7,0,0};}
        check(c.options.size()==1,"upgraded army cannot buy again");return Reply{-1,0,0};
    });dispatchService(HILL_FORT,h.getLocation());
    check(h.m_army.m_armies[0]==CREATURE_HALBERDIER&&h.m_army.m_armies[1]==13,"hill-fort canonical upgrade");
    check(player.m_resources[GOLD]==beforeGold-int(fullCost[6]*g_afUpgradeCostFactor[6]),"hill-fort server price");++checks;
    h.m_army.m_armies[0]=2;player.m_resources[GOLD]=0;
    setChoiceHandler([&](const Choice& c){check(c.options.size()==1,"unaffordable upgrade excluded");return Reply{-1,0,0};});dispatchService(HILL_FORT,h.getLocation());++checks;
    // Evil class may sacrifice creatures; stage is nonmutating and commit exact.
    h.m_heroClass=classDeathKnight;h.m_skillLevel[eSecSkillLearning]=0;
    h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,20,-1);
    const int startingXp=h.m_experience;calls=0;
    setChoiceHandler([&](const Choice& c){
        if(c.kind=="level_up"){++levelChoices;return Reply{c.options.front(),0,0};}
        check(c.kind=="sacrifice"&&bool(c.validate),"altar typed choice");
        if(calls==0){check(!c.validate({1,0,20})&&!c.validate({1,7,1})&&!c.validate({1,0,-1}),"last troop and bounds rejected");++calls;return Reply{1,0,7};}
        if(calls==1){check(h.m_army.m_numTroops[0]==20&&h.m_experience==startingXp,"staging unchanged");++calls;return Reply{3,0,0};}
        return Reply{0,0,0};
    });dispatchService(ALTAR_OF_SACRIFICE,h.getLocation());
    check(h.m_army.m_numTroops[0]==13&&h.m_experience==startingXp+int((akCreatureTypeTraits[CREATURE_PIKEMAN].AI_value/40*5)*7*h.getExperienceBonusFactor()),"altar exact count and experience");++checks;
    calls=0;setChoiceHandler([&](const Choice& c){return calls++?Reply{0,0,0}:Reply{1,0,5};});dispatchService(ALTAR_OF_SACRIFICE,h.getLocation());check(h.m_army.m_numTroops[0]==13,"cancel restores uncommitted offerings");++checks;
    h.m_heroClass=classKnight;
    h.equipArtifact(type_artifact(ARTIFACT_SWORD_OF_JUDGEMENT),3);type_artifact_offering offering;offering.set(h.getArtifact(TArtifactSlot(3)),TArtifactSlot(3),&h);
    const int artifactXp=h.m_experience;calls=0;
    setChoiceHandler([&](const Choice& c){
        if(c.kind=="level_up"){++levelChoices;return Reply{c.options.front(),0,0};}
        check(std::find(c.options.begin(),c.options.end(),1)==c.options.end(),"good hero cannot sacrifice creatures");
        if(calls==0){check(!c.validate({2,83,1})&&!c.validate({2,17,1}),"artifact bounds and special artifacts");++calls;return Reply{2,3,1};}
        if(calls==1){check(h.getArtifact(TArtifactSlot(3)).m_artifactId==ARTIFACT_SWORD_OF_JUDGEMENT,"artifact unchanged before commit");++calls;return Reply{3,0,0};}
        return Reply{0,0,0};
    });dispatchService(ALTAR_OF_SACRIFICE,h.getLocation());
    check(h.getArtifact(TArtifactSlot(3)).m_artifactId==ARTIFACT_NONE&&h.m_experience==artifactXp+offering.m_value,"artifact consumed and exact original reward");check(levelChoices>0,"artifact experience opens original level choices");++checks;
    // Native hire consumes the canonical recruit, gold, roster and map placement.
    const int offered=player.m_recruits[0];check(offered>=0,"native recruit fixture");
    const int oldHeroes=player.m_numHeroes;player.m_resources[GOLD]=g_heroGoldCost+123;
    type_point hirePoint=h.getLocation();hirePoint.m_x+=2;
    setChoiceHandler([&](const Choice& c){check(c.kind=="map_tavern"&&std::find(c.options.begin(),c.options.end(),offered)!=c.options.end(),"native tavern offers");return Reply{offered,0,0};});
    dispatchService(TAVERN,hirePoint);
    check(player.m_resources[GOLD]==123&&player.m_numHeroes==oldHeroes+1&&g_game->m_heroes[offered].m_owner==actor&&g_game->m_heroes[offered].getLocation()==hirePoint,"native hire result");++checks;
    setChoiceHandler([&](const Choice& c){check(c.options.size()==2&&c.options[0]==-1&&c.options[1]==-2,"unaffordable map tavern hire excluded");return Reply{-1,0,0};});dispatchService(TAVERN,hirePoint);++checks;
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"levelChoices\":"<<levelChoices<<"}\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
