// Adventure rewards and repeat-visit boundaries over original rules, no browser UI.
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
 if(argc!=2||chdir(argv[1]))return 2;const int faction=0;
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

 g_advManager->setHeroContext(h.m_id,0,false,false);
 int notices=0,levelChoices=0;bool decline=false;
 Port::Authority::setChoiceHandler([&](const Port::Authority::Choice& c){++notices;if(c.kind=="level_up")++levelChoices;return Port::Authority::Reply{decline&&c.kind=="normal"?DIALOG_RETURN_DECLINE:c.options.front(),0,0};});
 auto event=[&](int type,int extra=0,int subtype=0){NewmapCell cell;cell.m_type=TAdventureObjectType(type);cell.m_extraInfo=extra;cell.m_objectIndex=subtype;cell.m_objectTypeIndex=-1;g_advManager->doEvent(&cell,h.getLocation());};
 // All seven pickups exercise the real human-event path without a server UI.
 for(int type=0;type<7;++type){auto before=p.m_resources[type];Port::Authority::takePresentations();event(RESOURCE,3,type);check(p.m_resources[type]==before+(type==GOLD?300:3),"resource exact gain");auto events=Port::Authority::takePresentations();check(events.size()==1&&events[0].kind=="resource"&&events[0].id==type&&events[0].other==(type==GOLD?300:3)&&!events[0].text.empty(),"original resource message deferred");}
 for(auto pair:std::array<std::pair<int,int>,4>{{{MERC_CAMP,0},{DEFENSE_TOWER,1},{POWER_SCHOOL,2},{GARDEN_OF_REVELATION,3}}}){const int before=h.getPrimarySkill(pair.second);event(pair.first);check(h.getPrimarySkill(pair.second)==before+1,"primary visit +1");event(pair.first);check(h.getPrimarySkill(pair.second)==before+1,"primary visit not farmable");}
 h.m_mana=0;event(MAGIC_WELL);check(h.m_mana==h.getMaxMana(),"well fills mana");h.m_mana=0;event(MAGIC_WELL);check(h.m_mana==0,"well once per day");
 const int movement=h.m_movePoints;event(STABLES);check(h.m_movePoints==movement+g_stablesMovementBonus,"stables movement");event(STABLES);check(h.m_movePoints==movement+g_stablesMovementBonus,"stables once per week");
 auto morale=h.m_moraleBonus;event(TEMPLE);check(h.m_moraleBonus==morale+1,"temple morale");event(TEMPLE);check(h.m_moraleBonus==morale+1,"temple not farmable");
 auto luck=h.m_luckBonus;event(FAERIE_RING);check(h.m_luckBonus==luck+1,"ring luck");event(FAERIE_RING);check(h.m_luckBonus==luck+1,"ring not farmable");
 // Force the paid tree branch, then repeat and decline a distinct tree.
 NewmapCell tree;tree.m_type=TREE_OF_KNOWLEDGE;tree.m_objectTypeIndex=-1;tree.m_extraInfo=0;tree.m_treeInfo.m_price=const_tree_wants_gold;
 fund();auto gold=p.m_resources[GOLD];auto level=h.m_level;g_advManager->doEvent(&tree,h.getLocation());check(h.m_level==level+1&&p.m_resources[GOLD]==gold-2000,"tree gold and level");gold=p.m_resources[GOLD];level=h.m_level;g_advManager->doEvent(&tree,h.getLocation());check(h.m_level==level&&p.m_resources[GOLD]==gold,"tree once per hero");
 tree.m_treeInfo.m_id=1;decline=true;g_advManager->doEvent(&tree,h.getLocation());check(h.m_level==level&&p.m_resources[GOLD]==gold,"declined tree does not charge");decline=false;p.m_resources[GOLD]=0;g_advManager->doEvent(&tree,h.getLocation());check(h.m_level==level&&p.m_resources[GOLD]==0,"poor tree does not charge");
 h.giveArtifact(type_artifact(ARTIFACT_SPELLBOOK),false,false);h.addSpell(SPELL_VISIONS);h.m_mana=100;int beforeNotices=notices;int cost=h.getManaCost(SPELL_VISIONS);call(15,h.m_id,SPELL_VISIONS);check(h.m_mana==100-cost&&notices==beforeNotices+1,"Visions original confirmation and mana");
 h.m_mana=0;reject(15,h.m_id,SPELL_VISIONS);check(h.m_mana==0,"insufficient mana atomic");
 check(levelChoices>0,"manual level decisions exercised");std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"notices\":"<<notices<<",\"levelChoices\":"<<levelChoices<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<"adventure matrix: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
