// Nine-faction town operations and boundaries over original rules, no browser UI.
#include "server_authority.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "advmgr.h"
#include "gzfile.h"
#include "resourcemanager.h"
#include "kb.h"
#include "prefs.h"
#include "soundmgr.h"
#include "recruit.h"
#include "creaturetype.h"
#include "artifact.h"
#include "castle.h"
#include "winmgr.h"
#include <iostream>
#include <array>
#include <unistd.h>
bool portableLoadGameData();void portableInitVars();void aiInitialize();void sRand(int);
namespace {int checks=0;void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);++checks;}}
int main(int argc,char** argv){try{
 if(argc!=3||chdir(argv[1]))return 2;const int faction=std::atoi(argv[2]);
 initMainClasses();sRand(481516);g_noSound=1;g_config.m_soundVolume=g_config.m_musicVolume=0;g_config.m_blackoutComputer=1;
 ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);int error=0;
 check(ResourceManager::Open(true,true,&error)&&portableLoadGameData(),"load rules");g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();
 for(auto& p:g_game->m_players)p.init();g_numHumanPlayers=2;
 {TGzFile f("maps/Arrogance.h3m","rb");g_game->initNewGame(1,-1,nullptr,&f);}
 for(int i=0;i<8;++i){auto& p=g_game->m_players[i];g_game->m_setup.m_playerPos[i]=i<2?i:(i==5||i==6?10:-1);p.m_isHuman=i<2;p.m_isLocal=0;g_game->m_setup.m_startingHero[i]=-1;g_newMapStartingBonus[i]=0;}
 g_game->m_setup.m_alignment[0]=TTownType(faction);g_game->m_setup.m_startingHero[0]=faction*16;g_game->m_setup.m_startingHero[1]=15;
 g_game->setupOrigData();HeroId ids[8];for(int i=0;i<8;++i)ids[i]=g_startingHeroOverrides[i]=HeroId(g_game->m_setup.m_startingHero[i]);
 {TGzFile f("maps/Arrogance.h3m","rb");g_game->newMap(&f,ids,nullptr,2);}
 Port::Authority::setChoiceHandler([](const Port::Authority::Choice& c){return Port::Authority::Reply{c.options.front(),0,0};});
 Port::Authority::start();const int actor=Port::Authority::activeActor();Port::Authority::apply(0,actor,0,0,0);
 auto& p=g_game->m_players[actor];auto& t=g_game->m_towns[p.m_townIds[0]];auto& h=g_game->m_heroes[p.m_heroes[0]];
 check(t.m_type==faction,"requested faction");
 auto call=[&](int op,int id,int a=0,int b=0,int c=0,int d=0,int e=0){auto rev=Port::Authority::revision();Port::Authority::apply(op,actor,id,a,b,c,d,e);check(Port::Authority::revision()==rev+1,"accepted revision");};
 auto reject=[&](int op,int id,int a=0,int b=0,int c=0,int d=0,int e=0){auto rev=Port::Authority::revision();bool rejected=false;try{Port::Authority::apply(op,actor,id,a,b,c,d,e);}catch(const Port::Authority::Rejected&){rejected=true;}check(rejected&&Port::Authority::revision()==rev,"reject without revision");};
 auto fund=[&](){for(auto& r:p.m_resources)r=1000000;};
 fund();reject(2,t.m_id,44);reject(2,t.m_id,HOLY_GRAIL_ID);reject(2,3,HALL_TOWN_ID);
 int buildings=0;bool poorChecked=false;
 for(int pass=0;pass<44;++pass){bool progress=false;t.m_builtThisTurn=0;
  for(int id=0;id<44;++id){if(id==HOLY_GRAIL_ID||id==DOCK_WITH_BOAT_ID||(id>=27&&id<=29)||!(t.getBuildableMask()&g_bitNumber[id]))continue;
   if(!poorChecked){for(auto& r:p.m_resources)r=0;auto mask=t.getBuildingMask();reject(2,t.m_id,id);check(t.getBuildingMask()==mask,"poor build atomic");fund();poorChecked=true;}
   long before[7];for(int r=0;r<7;++r)before[r]=p.m_resources[r];int cost[7];std::copy_n(t.getBuildCostArray(type_building_id(id)),7,cost);
   call(2,t.m_id,id);check(t.hasBuilding(id,true)&&t.m_builtThisTurn,"building and daily marker");
   for(int r=0;r<7;++r)check(p.m_resources[r]==before[r]-cost[r],"exact building cost");
   reject(2,t.m_id,id);for(int next=0;next<44;++next)if((t.getBuildableMask()&g_bitNumber[next])&&next!=26&&next!=20){reject(2,t.m_id,next);break;}
   ++buildings;progress=true;break;
  }if(!progress)break;
 }
 check(buildings>=20&&poorChecked,"all faction construction tiers reached");t.m_builtThisTurn=0;
 int dwellings=0;for(int slot=0;slot<14;++slot){if(!t.hasBuilding(DWELLING_0_ID+slot,true))continue;
  fund();t.getArmy().initialize();t.m_population[slot]=50;const int type=g_dwellingType[t.m_type][slot];int cost[7];getMonsterCost(type,cost);
  call(4,t.m_id,slot,3);check(t.getArmy().getCreatureTotal(TCreatureType(type))==3&&t.m_population[slot]==47,"recruit army and stock");
  for(int r=0;r<7;++r)check(p.m_resources[r]==1000000-cost[r]*3,"recruit exact cost");
  reject(4,t.m_id,slot,0);reject(4,t.m_id,slot,48);for(auto& r:p.m_resources)r=0;reject(4,t.m_id,slot,1);check(t.m_population[slot]==47,"recruit rejection atomic");++dwellings;
 }
 check(dwellings==14,"all 14 creature variants");fund();
 g_advManager->demobilizeCurrHero(false,false);g_advManager->teleportTo(&h,t.getLocation(),0,0,0,0);t.m_visitingHeroId=h.m_id;t.m_garrisonHeroId=-1;
 h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,40,-1);h.m_army.add(CREATURE_ARCHER,20,-1);t.getArmy().initialize();t.getArmy().add(CREATURE_PIKEMAN,10,-1);
 // Entry rewards are canonical, retain original dialogs and cannot be farmed.
 h.m_townSpecialGrantedMask[t.m_id]=0;h.m_flags&=~2;t.m_manaVortexFull=1;h.m_mana=0;
 const int oldXp=h.m_experience,oldMove=h.m_movePoints;
 int oldStats[4];for(int i=0;i<4;++i)oldStats[i]=h.getPrimarySkill(i);
 reject(41,t.m_id,1);reject(41,-1);reject(41,3);
 call(41,t.m_id);
 if(faction==TOWN_CASTLE)check(h.m_movePoints==oldMove+g_stablesMovementBonus,"stables canonical movement");
 if(faction==TOWN_DUNGEON){check(h.m_experience==oldXp+int(h.getExperienceBonusFactor()*1000.0f),"academy canonical experience");check(h.m_mana>0&&!t.m_manaVortexFull,"vortex canonical mana");}
 const int stat=faction==TOWN_TOWER?3:faction==TOWN_INFERNO?2:faction==TOWN_STRONGHOLD?0:faction==TOWN_FORTRESS?1:-1;
 if(stat>=0)check(h.getPrimarySkill(stat)==oldStats[stat]+1,"canonical town primary skill");
 const int xp=h.m_experience,mana=h.m_mana,move=h.m_movePoints;int stats[4];for(int i=0;i<4;++i)stats[i]=h.getPrimarySkill(i);
 call(41,t.m_id);check(h.m_experience==xp&&h.m_mana==mana&&h.m_movePoints==move,"entry rewards not duplicated");
 for(int i=0;i<4;++i)check(h.getPrimarySkill(i)==stats[i],"primary reward not duplicated");
 const int townRef=-int(t.m_id)-1;
 call(16,h.m_id,0,townRef,0,5,0);check(h.m_army.m_numTroops[0]==35&&t.getArmy().m_numTroops[0]==15,"cross-row merge");
 call(16,h.m_id,0,h.m_id,2,7,0);check(h.m_army.m_numTroops[0]==28&&h.m_army.m_numTroops[2]==7,"split");
 call(16,h.m_id,1,townRef,0,0,1);check(h.m_army.m_armies[1]==CREATURE_PIKEMAN&&t.getArmy().m_armies[0]==CREATURE_ARCHER,"cross-row swap different creatures");
 reject(16,h.m_id,0,townRef,0,1,0);reject(16,h.m_id,0,h.m_id,0,1,0);reject(16,h.m_id,0,townRef,7,1,0);reject(16,h.m_id,0,townRef,1,999,0);
 call(25,h.m_id,2);check(h.m_army.m_numTroops[2]==0,"dismiss one stack");
 h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,1,-1);reject(25,h.m_id,0);reject(16,h.m_id,0,townRef,1,1,0);
 t.getArmy().initialize();for(int i=0;i<7;++i)t.getArmy().add(TCreatureType(2*i),10,-1);h.m_army.m_armies[0]=CREATURE_PIXIE;reject(17,t.m_id,0);
 t.getArmy().initialize();h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,40,-1);h.m_army.add(CREATURE_ARCHER,20,-1);const int roster=p.m_numHeroes;
 call(17,t.m_id,0);check(t.m_garrisonHeroId==h.m_id&&t.m_visitingHeroId==-1&&p.m_numHeroes==roster-1,"enter garrison");reject(1,h.m_id,4);
 // Hire a second hero while the first one occupies the garrison.
 const int offer=p.m_recruits[0];long gold=p.m_resources[GOLD];call(3,t.m_id,offer);check(t.m_visitingHeroId==offer&&p.m_resources[GOLD]==gold-g_heroGoldCost,"hire beside garrison");reject(3,t.m_id,p.m_recruits[1]);reject(17,t.m_id,1);
 auto& visitor=g_game->m_heroes[offer];visitor.m_townSpecialGrantedMask[t.m_id]=0;visitor.m_flags&=~2;
 const int visitorXp=visitor.m_experience,visitorMove=visitor.m_movePoints,visitorStat=stat>=0?visitor.getPrimarySkill(stat):0;
 if(faction==TOWN_DUNGEON){t.m_manaVortexFull=1;h.m_mana=0;visitor.m_mana=0;}
 call(41,t.m_id);
 if(faction==TOWN_DUNGEON){check(h.m_mana>0&&visitor.m_mana==0&&!t.m_manaVortexFull,"vortex prioritizes garrison over visitor");check(visitor.m_experience>visitorXp,"second hero academy reward");}
 if(stat>=0)check(visitor.getPrimarySkill(stat)==visitorStat+1,"second hero primary reward");
 if(faction==TOWN_CASTLE)check(visitor.m_movePoints==visitorMove+g_stablesMovementBonus,"second hero stables reward");
 call(17,t.m_id,2);check(t.m_visitingHeroId==h.m_id&&t.m_garrisonHeroId==offer,"swap two heroes");call(17,t.m_id,2);check(t.m_visitingHeroId==offer&&t.m_garrisonHeroId==h.m_id,"swap back");
 call(23,offer);check(t.m_visitingHeroId==-1,"dismiss visitor");call(17,t.m_id,1);check(t.m_visitingHeroId==h.m_id&&t.m_garrisonHeroId==-1&&p.m_numHeroes==roster,"leave garrison");
 fund();h.m_army.initialize();h.m_army.add(g_dwellingType[faction][0],10,-1);call(22,h.m_id,0);check(h.m_army.m_armies[0]==g_dwellingType[faction][7],"upgrade faction creature");reject(22,h.m_id,0);
 fund();long wood=p.m_resources[WOOD],oldGold=p.m_resources[GOLD];call(19,t.m_id,WOOD,GOLD,1);check(p.m_resources[WOOD]<wood&&p.m_resources[GOLD]>oldGold,"resource market");reject(19,t.m_id,GOLD,GOLD,1);reject(19,t.m_id,WOOD,GOLD,0);
 // Full army refuses a new creature without consuming stock or currency.
 fund();t.getArmy().initialize();const int recruitType=g_dwellingType[faction][7];
 for(int i=0,n=0;n<7;++i)if(i!=recruitType)t.getArmy().add(TCreatureType(i),1,-1),++n;
 t.m_population[7]=50;gold=p.m_resources[GOLD];reject(4,t.m_id,7,1);check(t.m_population[7]==50&&p.m_resources[GOLD]==gold,"full recruitment atomic");
 // Eight field heroes prevent the garrison occupant from being released.
 t.getArmy().initialize();call(17,t.m_id,0);const int realCount=p.m_numHeroes;p.m_numHeroes=8;reject(17,t.m_id,1);p.m_numHeroes=realCount;call(17,t.m_id,1);
 // Original Grail construction consumes the carried artifact, including the
 // normal exception to the one-building-per-day rule.
 h.giveArtifact(type_artifact(ARTIFACT_HOLY_GRAIL),1,1);t.m_builtThisTurn=1;
 call(2,t.m_id,HOLY_GRAIL_ID);check(t.hasBuilding(HOLY_GRAIL_ID,true)&&!h.hasArtifact(ARTIFACT_HOLY_GRAIL),"Grail consumed and installed");reject(2,t.m_id,HOLY_GRAIL_ID);
 // Coastal fixture only for factions that actually support a shipyard.
 int boats=0;
 if(g_townEligibleBuildMask[faction]&g_bitNumber[DOCK_ID]){
  fund();t.boatX=t.m_mapX+2;t.boatY=t.m_mapY+2;auto* water=g_game->getCell(type_point(t.boatX,t.boatY,t.m_mapZ));water->m_typeValue=0;water->m_groundSet=eTerrainWater;
  t.setLegalBuildings(0);t.m_builtThisTurn=0;if(!t.hasBuilding(DOCK_ID,true))call(2,t.m_id,DOCK_ID);
  p.m_resources[GOLD]=0;reject(21,t.m_id);fund();gold=p.m_resources[GOLD];wood=p.m_resources[WOOD];call(21,t.m_id);
  check(water->m_type==BOAT&&p.m_resources[GOLD]==gold-1000&&p.m_resources[WOOD]==wood-10,"shipyard creates paid boat");reject(21,t.m_id);++boats;
 }
 // The Inferno gate must move the visitor, release the old entrance, and
 // reject an occupied destination. A second existing town is a fixture only.
 if(faction==TOWN_INFERNO){
  auto& other=g_game->m_towns[t.m_id==0?1:0];town saved=other;const auto savedCell=*g_game->getCell(other.getLocation());
  other.m_type=TOWN_INFERNO;other.m_owner=actor;other.m_visitingHeroId=-1;other.m_garrisonHeroId=-1;other.buildBuilding(EXTRA_1_ID,false,false);
  call(37,t.m_id,other.m_id);check(t.m_visitingHeroId==-1&&other.m_visitingHeroId==h.m_id&&h.getLocation()==other.getLocation(),"castle gate arrival");
  reject(37,other.m_id,other.m_id);t.m_visitingHeroId=offer;reject(37,other.m_id,t.m_id);t.m_visitingHeroId=-1;
  call(37,other.m_id,t.m_id);check(t.m_visitingHeroId==h.m_id&&other.m_visitingHeroId==-1,"castle gate return");
  *g_game->getCell(other.getLocation())=savedCell;other=saved;
 }
 // Original calendar transition, with abundant resources and no AI turns.
 t.m_builtThisTurn=1;g_game->m_day=7;g_game->m_week=1;g_game->m_month=1;
 int population[14];std::copy_n(t.m_population,14,population);gold=p.m_resources[GOLD];const int income=t.getGoldIncome(true);g_game->perDay();
 check(g_game->m_day==1&&g_game->m_week==2&&!t.m_builtThisTurn,"new week resets daily construction");check(p.m_resources[GOLD]>=gold+income,"daily income received");
 for(int i=0;i<14;++i)check(t.m_population[i]>=population[i],"weekly population retained or increased");
 std::cout<<"{\"passed\":true,\"faction\":"<<faction<<",\"checks\":"<<checks<<",\"buildings\":"<<buildings<<",\"dwellings\":"<<dwellings<<",\"boats\":"<<boats<<"}\n";
 return 0;
}catch(const std::exception& e){std::cerr<<"town matrix: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
