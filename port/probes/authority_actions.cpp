// Native semantic-action fixtures over the real original tables and map.
#include "server_authority.h"
#include "server_exchange.h"
#include "server_market.h"
#include "tradpost.h"
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
    if(argc!=3||chdir(argv[1]))return 2;initMainClasses();sRand(42);g_noSound=1;
    g_config.m_soundVolume=0;g_config.m_musicVolume=0;g_config.m_blackoutComputer=1;
    ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);
    int error=0;check(ResourceManager::Open(true,true,&error)&&portableLoadGameData(),"rule resources");
    g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();g_numHumanPlayers=2;
    {TGzFile file(argv[2],"rb");g_game->initNewGame(1,-1,nullptr,&file);}
    for(int i=0;i<8;++i){auto& p=g_game->m_players[i];p.m_isLocal=0;p.m_isHuman=g_game->m_setup.m_playerPos[i]>=0&&g_game->m_setup.m_playerPos[i]<8;g_game->m_setup.m_startingHero[i]=-1;g_newMapStartingBonus[i]=0;}
    g_game->setupOrigData();{TGzFile file(argv[2],"rb");g_game->newMap(&file,nullptr,nullptr,2);}

    auto acknowledge=[](const Port::Authority::Choice& c){return Port::Authority::Reply{c.options.front(),0,0};};
    Port::Authority::setChoiceHandler(acknowledge);Port::Authority::start();const int actor=Port::Authority::activeActor();Port::Authority::apply(0,actor,0,0,0);
    auto& player=g_game->m_players[actor];hero& h=g_game->m_heroes[player.m_heroes[0]];town& t=g_game->m_towns[player.m_townIds[0]];
    for(int r=0;r<7;++r)player.m_resources[r]=1000000;
    auto call=[&](int op,int id,int a=0,int b=0,int c=0,int d=0,int e=0){const auto rev=Port::Authority::revision();Port::Authority::apply(op,actor,id,a,b,c,d,e);check(Port::Authority::revision()==rev+1,"one revision per accepted intent");};
    auto reject=[&](int op,int id,int a=0,int b=0,int c=0,int d=0,int e=0){const auto rev=Port::Authority::revision();bool denied=false;try{Port::Authority::apply(op,actor,id,a,b,c,d,e);}catch(const Port::Authority::Rejected&){denied=true;}check(denied&&Port::Authority::revision()==rev,"rejected intent preserves revision");};
    h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,10,-1);h.m_army.add(CREATURE_ARCHER,5,-1);
    call(16,h.m_id,0,h.m_id,2,3,0);check(h.m_army.m_numTroops[0]==7&&h.m_army.m_numTroops[2]==3,"army split quantity");
    reject(16,h.m_id,0,h.m_id,2,999,0);check(h.m_army.m_numTroops[0]==7,"bad army count did not mutate");
    call(16,h.m_id,2,h.m_id,0,3,0);check(h.m_army.m_numTroops[0]==10&&h.m_army.m_armies[2]<0,"army merge");
    call(25,h.m_id,1);reject(25,h.m_id,0);check(h.m_army.hasCreatures(),"last hero stack retained");
    call(30,h.m_id,1);check(h.m_formation==1,"formation");
    h.giveArtifact(type_artifact(TArtifact(8)),0,0);int weapon=-1;for(int slot=0;slot<19;++slot)if(h.getArtifact(TArtifactSlot(slot)).m_artifactId==8)weapon=slot;check(weapon>=0,"fixture artifact equipped");
    const int pack=h.getNumberInBackpack(1);call(18,h.m_id,weapon,h.m_id,100+pack);check(h.getBackpack(pack).m_artifactId==8,"equipment to backpack");
    call(18,h.m_id,100+pack,h.m_id,weapon);check(h.getArtifact(TArtifactSlot(weapon)).m_artifactId==8,"backpack to equipment");reject(24,h.m_id,weapon,0);
    h.equipArtifact(type_artifact(ARTIFACT_SPELLBOOK),eArtifactSlotSpellbook);reject(18,h.m_id,eArtifactSlotSpellbook,h.m_id,100+h.getNumberInBackpack(1));
    h.m_mana=500;h.m_flightLevel=eMasteryInvalid;h.m_waterWalkLevel=eMasteryInvalid;
    for(int spell=0;spell<10;++spell)h.addSpell(spell);
    int mana=h.m_mana,cost=h.getManaCost(SPELL_FLY);call(15,h.m_id,SPELL_FLY);check(h.isFlying(0)&&h.m_mana==mana-cost,"Fly uses canonical mana and mastery");reject(15,h.m_id,SPELL_FLY);
    h.m_flightLevel=eMasteryInvalid;mana=h.m_mana;cost=h.getManaCost(SPELL_WATER_WALK);call(15,h.m_id,SPELL_WATER_WALK);check(h.canWalkOnWater(0)&&h.m_mana==mana-cost,"Water Walk canonical mana and status");
    h.m_waterWalkLevel=eMasteryInvalid;h.m_movePoints=h.m_maxMovePoints=2000;h.m_dWalkSpellsCast=0;
    type_point target;bool found=false;for(int y=std::max(0,int(h.m_y)-2);y<std::min(g_mapHeight,int(h.m_y)+3)&&!found;++y)for(int x=std::max(0,int(h.m_x)-2);x<std::min(g_mapWidth,int(h.m_x)+3);++x){auto* c=g_game->getCell(type_point(x,y,h.m_z));if(!(c->m_flags0011&0x100)&&!c->m_isTrigger&&c->m_groundSet==g_game->getCell(h.getLocation())->m_groundSet){target=type_point(x,y,h.m_z);found=true;break;}}
    check(found,"Dimension Door fixture target");mana=h.m_mana;cost=h.getManaCost(SPELL_DIMENSION_DOOR);call(15,h.m_id,SPELL_DIMENSION_DOOR,target.m_x,target.m_y,target.m_z);check(h.m_x==target.m_x&&h.m_y==target.m_y&&h.m_mana==mana-cost&&h.m_dWalkSpellsCast==1,"Dimension Door server target and daily count");
    reject(15,h.m_id,SPELL_DIMENSION_DOOR,g_mapWidth,0,h.m_z);
    // Adjacency does not authorize an exchange through blocked terrain or a
    // remote control message. Original meeting scope is required.
    hero* ally=nullptr;for(auto& candidate:g_game->m_heroes)if(candidate.m_owner>=0&&candidate.m_owner!=actor&&g_game->isHuman(candidate.m_owner)){ally=&candidate;break;}
    check(ally!=nullptr,"fixture second human hero");hero originalAlly=*ally;const int otherActor=ally->m_owner;
    ally->m_owner=actor;ally->m_x=h.m_x;ally->m_y=h.m_y;ally->m_z=h.m_z;
    reject(16,h.m_id,0,ally->m_id,0,1,0);reject(18,h.m_id,weapon,ally->m_id,100);reject(32,h.m_id,ally->m_id);*ally=originalAlly;
    const auto savedTeamCount=g_game->m_mapHeader.m_numTeams;const auto savedTeamA=g_game->m_mapHeader.m_teamInfo[actor],savedTeamB=g_game->m_mapHeader.m_teamInfo[otherActor];
    g_game->m_mapHeader.m_numTeams=2;g_game->m_mapHeader.m_teamInfo[actor]=0;g_game->m_mapHeader.m_teamInfo[otherActor]=0;
    h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,10,-1);h.m_army.add(CREATURE_ARCHER,5,-1);ally->m_army.initialize();ally->m_army.add(CREATURE_PIKEMAN,5,-1);
    int giverCalls[8]={},consentCalls=0;
    Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){
        if(c.kind=="normal"){++consentCalls;check(c.actor==otherActor&&!Port::Authority::exchangeCanInspectHero(actor,ally->m_id),"consent precedes any partner disclosure");return Port::Authority::Reply{DIALOG_RETURN_ACCEPT,0,0};}
        check(c.kind=="hero_exchange"&&Port::Authority::exchangeCanInspectHero(actor,ally->m_id)&&Port::Authority::exchangeCanInspectHero(otherActor,h.m_id),"only consented meeting grants private hero inspection");
        if(giverCalls[c.actor]++>0)return Port::Authority::Reply{0,0,0};
        if(c.actor==actor){check(!c.validate({1,1,98})&&!c.validate({1,0,49})&&!c.validate({3,1000,100}),"giver cannot steal recipient army or artifacts");return Port::Authority::Reply{1,2,49};}
        check(c.actor==otherActor&&!c.validate({1,1,49}),"second giver owns only its source");return Port::Authority::Reply{1,1,98};
    });
    Port::Authority::heroExchange(h,*ally);check(consentCalls==1&&giverCalls[actor]==2&&giverCalls[otherActor]==2,"two authenticated sequential donor passes");
    check(h.m_army.m_numTroops[0]==9&&ally->m_army.m_numTroops[0]==6,"allied gifts preserve total troops");
    check(!Port::Authority::exchangeCanInspectHero(actor,ally->m_id)&&!Port::Authority::exchangeAllowsPair(h.m_id,ally->m_id),"meeting grants revoked on exit");
    *ally=originalAlly;g_game->m_mapHeader.m_numTeams=savedTeamCount;g_game->m_mapHeader.m_teamInfo[actor]=savedTeamA;g_game->m_mapHeader.m_teamInfo[otherActor]=savedTeamB;Port::Authority::setChoiceHandler(acknowledge);
    // Original town garrison insertion and removal retain the hero roster.
    g_advManager->teleportTo(&h,t.getLocation(),0,0,0,0);t.m_visitingHeroId=h.m_id;t.m_garrisonHeroId=-1;t.getArmy().initialize();const int roster=player.m_numHeroes;
    call(17,t.m_id,0);check(t.m_garrisonHeroId==h.m_id&&player.m_numHeroes==roster-1,"original garrison insertion");reject(1,h.m_id,0);reject(11,h.m_id);reject(15,h.m_id,SPELL_FLY);
    call(17,t.m_id,1);check(t.m_visitingHeroId==h.m_id&&t.m_garrisonHeroId<0&&player.m_numHeroes==roster,"original garrison removal");
    t.m_type=TOWN_CASTLE;t.buildBuilding(DWELLING_0_UPG_ID,false,false);h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,10,-1);long upgradeCost[7];getUpgradeCost(CREATURE_PIKEMAN,CREATURE_HALBERDIER,10,upgradeCost);int gold=player.m_resources[GOLD];
    call(22,h.m_id,0);check(h.m_army.m_armies[0]==CREATURE_HALBERDIER&&player.m_resources[GOLD]==gold-upgradeCost[GOLD],"town upgrade uses original cost");
    t.buildBuilding(BLACKSMITH_ID,false,false);h.removeArtifact(ARTIFACT_BALLISTA);gold=player.m_resources[GOLD];call(34,t.m_id,h.m_id,TOWN_CASTLE);check(h.hasArtifact(ARTIFACT_BALLISTA)&&player.m_resources[GOLD]==gold-akCreatureTypeTraits[CREATURE_BALLISTA].cost[GOLD],"blacksmith purchase canonical artifact/cost");reject(34,t.m_id,h.m_id,TOWN_CASTLE);
    t.m_type=TOWN_NECROPOLIS;t.buildBuilding(EXTRA_1_ID,false,false);call(36,h.m_id,1);check(h.m_army.m_armies[0]==CREATURE_SKELETON,"transformer original creature table");reject(36,h.m_id,1);
    t.m_type=TOWN_CONFLUX;t.buildBuilding(EXTRA_0_ID,false,false);h.m_skillCount=0;for(int i=0;i<28;++i){h.m_skillLevel[i]=0;h.m_skillOrder[i]=0;}int tuitionCalls=0,learned=-1;gold=player.m_resources[GOLD];
    Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){check(c.kind=="university","university original modal decision");if(tuitionCalls++==0){check(c.options.size()>1,"university has learnable school");learned=c.options[1];return Port::Authority::Reply{learned,0,0};}return Port::Authority::Reply{-1,0,0};});
    call(35,t.m_id,h.m_id);check(learned>=0&&h.m_skillLevel[learned]==1&&player.m_resources[GOLD]==gold-2000,"university skill and tuition");Port::Authority::setChoiceHandler(acknowledge);
    t.m_type=TOWN_DUNGEON;t.buildBuilding(EXTRA_1_ID,false,false);t.m_summoningType=CREATURE_PIKEMAN;t.m_summoningPopulation=5;auto& townArmy=t.getArmy();townArmy.initialize();int unitCost[7];getMonsterCost(CREATURE_PIKEMAN,unitCost);gold=player.m_resources[GOLD];call(38,t.m_id,2);check(t.m_summoningPopulation==3&&townArmy.getCreatureTotal(CREATURE_PIKEMAN)==2&&player.m_resources[GOLD]==gold-unitCost[GOLD]*2,"Portal of Summoning stock/army/cost");reject(38,t.m_id,6);
    // A real artifact shop must award the artifact constructor, never a spell
    // scroll made accidentally from the integer overload, and debit/credit
    // only the actor's canonical resources through repeat decisions.
    TArtifact offers[7]={TArtifact(9),ARTIFACT_NONE,ARTIFACT_NONE,ARTIFACT_NONE,ARTIFACT_NONE,ARTIFACT_NONE,ARTIFACT_NONE};
    check(!h.hasArtifact(TArtifact(9)),"artifact merchant fixture owns no offered item");
    const int shopGold=player.m_resources[GOLD],price=int(float(akArtifactTraits[9].m_cost)/g_artifactPurchaseEfficency[4]+0.5f);
    int merchantCalls=0;
    Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){
        check(c.kind=="market"&&c.actor==actor,"artifact merchant actor-private decision");
        if(merchantCalls++==0){check(!c.validate({3,1,6})&&!c.validate({3,0,55}),"artifact purchase rejects quantity and nonexistent inventory slot");return Port::Authority::Reply{3,0,6};}
        if(merchantCalls==2){
            check(h.hasArtifact(TArtifact(9))&&offers[0]==ARTIFACT_NONE&&player.m_resources[GOLD]==shopGold-price,"artifact merchant canonical item, stock and price");
            check(!c.validate({4,0,int(eArtifactSlotSpellbook)*7+GOLD}),"merchant cannot sell immutable spellbook");
            int sold=-1;for(int i=0;i<13;++i)if(h.getArtifact(TArtifactSlot(i)).m_artifactId==9)sold=i;
            for(int i=0;i<h.getNumberInBackpack(1);++i)if(h.getBackpack(i).m_artifactId==9)sold=18+i;
            check(sold>=0,"purchased item remains in legal equipment or backpack slot");return Port::Authority::Reply{4,0,sold*7+GOLD};
        }
        check(!h.hasArtifact(TArtifact(9))&&player.m_resources[GOLD]>shopGold-price&&player.m_resources[GOLD]<shopGold,"artifact sale removes exact item and preserves merchant spread");return Port::Authority::Reply{0,0,0};
    });
    Port::Authority::marketEncounter(&h,2,4,2,offers);check(merchantCalls==3,"artifact merchant repeated original decision flow");Port::Authority::setChoiceHandler(acknowledge);
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"revision\":"<<Port::Authority::revision()<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<"authority-actions fixture: "<<e.what()<<'\n';return 1;}}
