#include "combat_display.h"
// Synthetic ordinary battle fixture linked against the exact native server build.
#include "game.h"
#include "resourcemanager.h"
#include "kb.h"
#include "hero.h"
#include "soundmgr.h"
#include "prefs.h"
#include "server_authority.h"
#include "server_combat.h"
#include "cmbtmgr.h"
#include "town.h"
#include <unistd.h>
#include <iostream>
#include <regex>
#include <stdexcept>
bool portableLoadGameData();void portableInitVars();void aiInitialize();
struct EmptyMapStream:TAbstractFile {int read(void* data,int size)override{memset(data,0,size);return size;}int write(const void*,int)override{return 0;}};
namespace Probe {
using namespace Port::Authority;
int requests=0,resultEvents=0,autoSteps=0;bool autoStopped=false;bool castChosen=false;int selectedExtra=-1;bool tacticsSeen=false;
void gridConsistent(){
 for(int hex=0;hex<COMBAT_GRID_CELLS;++hex){
  const auto& cell=g_combatManager->m_cells[hex];if(cell.m_armySide<0)continue;
  if(cell.m_armySide>1 || cell.m_armySlot<0 || cell.m_armySlot>=20)throw std::runtime_error("invalid grid occupant");
  const auto& stack=g_combatManager->m_armies[cell.m_armySide][cell.m_armySlot];
  if(hex!=stack.m_gridIndex && (!stack.is(creatureDoubleWide) || hex!=stack.getSecondGridIndex()))throw std::runtime_error("ghost grid occupant");
 }
}
Reply answer(const Choice& c){
 static uint64_t token=0;
 if(c.token<=token || phase()!=Phase::Choice)throw std::runtime_error("choice token/stack phase");token=c.token;
 gridConsistent();
 if(c.kind!="combat" && c.kind!="combat_spell"){
  if(c.kind=="level_up" && resultEvents!=(getenv("AI")?1:2))throw std::runtime_error("level choice preceded combat results");
  return Reply{c.options.front(),0,0};
 }
 for(int side=0;side<2;++side)for(int slot=0;slot<g_combatManager->m_numArmies[side];++slot){
  const auto& original=g_combatManager->m_armies[side][slot];army display;
  display.m_creatureType=original.m_creatureType;display.m_monInfo=original.m_monInfo;
  std::copy_n(original.m_spellInfluence,81,display.m_spellInfluence);
  applyCombatModifiers(display,combatModifiers(original));
  if(display.getSpeed()!=original.getSpeed() || display.getAverageDamage()!=original.getAverageDamage() || display.getAdjustedAttack(nullptr,false)!=original.getAdjustedAttack(nullptr,false) || display.getAdjustedAttack(nullptr,true)!=original.getAdjustedAttack(nullptr,true))throw std::runtime_error("spell display differs from original values");
 }
 const auto observer=combatProjection(1-c.actor);
 if(observer.find("\"canAct\":false")==std::string::npos || observer.find("\"actions\":[]")==std::string::npos || !combatProjection(2).empty())throw std::runtime_error("observer action/privacy boundary");
 if(c.actor<0 || c.actor>1 || !g_game->isHuman(c.actor))throw std::runtime_error("invalid combat actor");
 if(g_combatManager->isQuickCombat() || g_combatManager->m_combatWindow)throw std::runtime_error("manual combat policy or presentation violation");
 if(++requests>500)throw std::runtime_error("too many requests");
 if(c.metadata.find("\"tactics\":true")!=std::string::npos)tacticsSeen=true;
 std::regex r("\\{\\\"id\\\":([0-9]+),\\\"action\\\":([0-9]+),\\\"hex\\\":(-?[0-9]+),\\\"extra\\\":(-?[0-9]+),\\\"hex2\\\":(-?[0-9]+)\\}");
 if(getenv("SPELL")&&!castChosen){for(std::sregex_iterator i(c.metadata.begin(),c.metadata.end(),r),end;i!=end;++i)if(std::stoi((*i)[2])==101){castChosen=true;return Reply{std::stoi((*i)[1]),0,0};}}
 int selected=-1,best=10000;
 for(std::sregex_iterator i(c.metadata.begin(),c.metadata.end(),r),end;i!=end;++i){
  int id=std::stoi((*i)[1]),action=std::stoi((*i)[2]),hex=std::stoi((*i)[3]);
  if(!getenv("AI") && (action==103 || action==104))throw std::runtime_error("PvP auto must be unavailable");
  if(getenv("AUTO") && !autoStopped) {
   const bool on=c.metadata.find("\"autoCombat\":true")!=std::string::npos;
   const int desired=on&&getenv("AUTO_STOP")?104:103;
   if(action==desired){selected=id;if(on)++autoSteps;if(action==104)autoStopped=true;break;}
  }
  if(const char* desired=getenv("ACTION"))if(action==atoi(desired)){selected=id;selectedExtra=std::stoi((*i)[4]);break;}
  if(action==100){selected=id;break;}
  if(action==101 && !castChosen){castChosen=true;selected=id;break;}
  if(action==1){selected=id;break;}
  if(action==6||action==7){selected=id;break;}
  if(action==2){int score=abs(hex%17-8)+abs(hex/17-5);if(score<best){best=score;selected=id;}}
 }
 if(selected<0)selected=0;
 if(getenv("INVALID"))return Reply{999999,0,0};
 if(getenv("INVALID_ARGUMENTS"))return Reply{selected,2147483647,-2147483647};
 return Reply{selected,0,0};
}
}
int main(int argc,char** argv){try{
 if(argc!=2||chdir(argv[1]))return 2;
 initMainClasses();g_noSound=1;g_config.m_soundVolume=0;g_config.m_musicVolume=0;
 ResourceManager::SetPath("data/");ResourceManager::SetPixelFormat(0xf800,0x07e0,0x001f);
 int err=0;if(!ResourceManager::Open(true,true,&err)||!portableLoadGameData())throw std::runtime_error("resources");
 g_game->m_worldMap.loadObjectTypeTemplates();aiInitialize();portableInitVars();
 if(getenv("AUTO"))g_config.m_combatAutoCreatures=1;
 g_mapWidth=g_mapHeight=36;g_game->m_mapHeader.m_size=36;g_game->m_mapHeader.m_hasTwoLayers=0;
 EmptyMapStream empty;g_game->m_worldMap.read(&empty,36,0,0x1c);
 for(int y=0;y<36;++y)for(int x=0;x<36;++x){auto* cell=g_game->getCell(type_point(x,y,0));cell->m_groundSet=2;cell->m_passable=1;}
 for(int i=0;i<8;++i)g_game->m_playerDisabled[i]=1;
 for(int side=0;side<2;++side){auto& p=g_game->m_players[side];p.m_isHuman=!(side && getenv("AI"));p.m_isLocal=0;p.m_resources[6]=10000;p.m_numHeroes=0;p.m_numTowns=0;p.m_recruits[0]=p.m_recruits[1]=heroIdNone;g_game->m_playerDisabled[side]=0;
  auto& h=g_game->m_heroes[side];h.m_owner=side;h.m_id=HeroId(side);h.m_army.initialize();h.m_army.add(static_cast<TCreatureType>(getenv("CREATURE") && !side?atoi(getenv("CREATURE")):0),side?1:20,-1);h.m_experience=0;h.m_level=1;
 }
 auto& left=g_game->m_heroes[0];auto& right=g_game->m_heroes[1];
 if(const char* spell=getenv("SPELL")){left.equipArtifact(type_artifact(ARTIFACT_SPELLBOOK),17);left.addSpell(atoi(spell));left.m_mana=100;left.setPrimarySkill(2,1);}

 if(getenv("TACTICS")){left.m_skillLevel[eSecSkillBattleTactics]=3;left.m_formation|=2;}
 town* defending=nullptr;
 if(getenv("SIEGE")){g_game->m_towns.resize(1);defending=&g_game->m_towns[0];defending->m_id=0;defending->m_owner=1;defending->m_type=0;defending->setLegalBuildings(0);defending->setMask(g_bitNumber[CASTLE_CASTLE_ID]);}
 Port::Authority::setCombatResultHandler([](int actor,const std::string& data){
  if(actor!=Probe::resultEvents || Port::Authority::phase()!=Port::Authority::Phase::Combat)throw std::runtime_error("result actor/order/phase");
  if(data.find("\"canAct\":false")==std::string::npos || data.find("\"actions\":[]")==std::string::npos || data.find("\"winner\":"+std::to_string(g_combatManager->m_winner))==std::string::npos)throw std::runtime_error("result projection");
  if(g_combatManager->m_winner==actor && g_game->m_heroes[actor].m_experience!=0)throw std::runtime_error("result must precede human experience/level choice");
  if(data.find("\"retreated\":"+std::string(getenv("ACTION") && atoi(getenv("ACTION"))==4?"true":"false"))==std::string::npos || data.find("\"surrendered\":"+std::string(getenv("ACTION") && atoi(getenv("ACTION"))==5?"true":"false"))==std::string::npos)throw std::runtime_error("result outcome flags");
  std::cerr<<"RESULT "<<actor<<" "<<data<<"\n";
  ++Probe::resultEvents;
 });
 Port::Authority::setChoiceHandler(Probe::answer);
 Port::Authority::start();
 // Invalid/inactive seat cannot even reach the decision callback.
 {Port::Authority::Choice bad;bad.actor=8;bad.kind="combat";bad.options={0};bool rejected=false;
  try{Port::Authority::choose(bad);}catch(const std::runtime_error&){rejected=true;}
  if(!rejected || Probe::requests)throw std::runtime_error("unauthorized choice actor accepted");}
 g_combatManager->setupCombat(type_point(10,10,0),&left,&left.m_army,1,defending,&right,&right.m_army,10,10,42,false);
 Port::Authority::runCombat(*g_combatManager);
 Probe::gridConsistent();
 if(Probe::resultEvents!=(getenv("AI")?1:2))throw std::runtime_error("missing private result events");
 if(getenv("ACTION")){
  if(g_combatManager->m_winner!=1)throw std::runtime_error("wrong retreat outcome");
  if(atoi(getenv("ACTION"))==5 && (g_game->m_players[0].m_resources[6]!=10000-Probe::selectedExtra || g_game->m_players[1].m_resources[6]!=10000+Probe::selectedExtra))throw std::runtime_error("surrender price mismatch");
 }else if(g_combatManager->m_winner!=0 || !left.m_army.hasCreatures() || right.m_army.hasCreatures())throw std::runtime_error("wrong outcome");
 if(getenv("TACTICS")&&!Probe::tacticsSeen)throw std::runtime_error("tactics missing");
 if(getenv("SPELL") && (!Probe::castChosen || left.m_mana>=100))throw std::runtime_error("spell not offered");
 if(getenv("AUTO") && (Probe::autoSteps<1 || (getenv("AUTO_STOP") && !Probe::autoStopped)))throw std::runtime_error("authoritative auto handshake missing");
 std::cout<<"{\"passed\":true,\"requests\":"<<Probe::requests<<",\"winner\":"<<g_combatManager->m_winner<<",\"xp\":"<<left.m_experience<<"}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
