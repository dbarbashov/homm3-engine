// Only linked into HOMM3_HARNESS builds. Never shipped with the game.
#include "parity_harness.h"
#include "browser_rng.h"
#include "prefs.h"
#include "mousemgr.h"
#include "findpath.h"
#include "bitmap16.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "townmgr.h"
#include "advmgr.h"
#include "cmbtmgr.h"
#include "kb.h"
#include "gzfile.h"
#include "remote.h"
#include "soundmgr.h"
#include "winmgr.h"
#include "window.h"
#include "widget.h"
#include "textwdgt.h"
#include "iconwdgt.h"
#include "border.h"
#include "bitmap816.h"
#include "binkmanager.h"
#include "hd_view.h"
#include <typeinfo>
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_ASYNC_JS(void, parityYield, (), { await window.homm3Harness.yield(); });
#endif
namespace Port { namespace Harness {
namespace {
uint32_t clockMs=0, rng=1;
bool loaded=false,traceRandom=false,browserCombatRng=false;
#ifdef __EMSCRIPTEN__
bool controlled=false;
#endif
std::string quoteBytes(const std::string& value){
    std::string result(1,char(34));const char* hex="0123456789abcdef";
    for(unsigned char c:value){
      if(c==34||c==92){result+=char(92);result+=char(c);}
      else if(c<32||c>=127){result+=char(92);result+="u00";result+=hex[c>>4];result+=hex[c&15];}
      else result+=char(c);
    }
    return result+char(34);
}
void interfaceSnapshot(){
#ifdef __EMSCRIPTEN__
    if(g_game&&g_advManager&&g_game->getLocalPlayer()){
        auto* local=g_game->getLocalPlayer();
        EM_ASM({window.homm3AdventureState=({hero:$0,town:$1,mobile:!!$2,cursor:!!$3,heroes:[]});},int(local->m_currHeroId),local->m_currTownId,g_advManager->m_curHeroMobile,g_advManager->m_drawCursor);
        EM_ASM({window.homm3AdventureState.bottom=({type:$0,resource:$1,quantity:$2,text:new TextDecoder('windows-1251').decode(HEAPU8.subarray($3,$3+$4))});},g_advManager->m_bottomViewType,g_advManager->m_bottomViewResourceType,g_advManager->m_bottomViewResourceQuantity,g_advManager->m_bottomViewMessage.data(),g_advManager->m_bottomViewMessage.size());
        EM_ASM({window.homm3AdventureState.obelisks=[];window.homm3AdventureState.motion=({moving:!!$0,sequence:$1,frame:$2,scroll:[$3,$4],speed:$5,local:!!$6,pointerVisible:!!$7,showRoute:!!$8});window.homm3AdventureState.route=[];},g_advManager->m_heroMoving,g_advManager->m_cursorSequence,g_advManager->m_cursorFrameCount,g_advManager->m_scrollX,g_advManager->m_scrollY,g_config.m_walkSpeed[1],g_currentPlayer->isLocalHuman(),g_mouseManager->isVis(),g_advManager->m_showRoute);
        if(g_advManager->m_routeArray)for(int z=0;z<g_game->getNumMapLevels();++z)for(int y=0;y<g_mapHeight;++y)for(int x=0;x<g_mapWidth;++x){const int value=*g_advManager->getRouteArrayPtr(x,y,z);if(value)EM_ASM({window.homm3AdventureState.route.push([$0,$1,$2,$3]);},x,y,z,value);}
        for(auto flags:g_game->m_obeliskFlags)EM_ASM({window.homm3AdventureState.obelisks.push(!!$0);},flags&(1<<g_localGamePos));
        for(int slot=0;slot<local->m_numHeroes;++slot){
            auto& own=g_game->m_heroes[local->m_heroes[slot]];auto* cell=g_game->getCell(own.getLocation());
            // Original hero records are packed; copy to aligned words before
            // exposing diagnostic arrays to JS typed-array views.
            int army[7],counts[7];for(int i=0;i<7;++i){army[i]=own.m_army.m_armies[i];counts[i]=own.m_army.m_numTroops[i];}
            EM_ASM({window.homm3AdventureState.heroes.push({id:$0,x:$1,y:$2,z:$3,onMap:!!$4,army:Array.from(HEAP32.subarray($5>>2,($5>>2)+7)),counts:Array.from(HEAP32.subarray($6>>2,($6>>2)+7))});},int(own.m_id),int(own.m_x),int(own.m_y),int(own.m_z),cell->m_type==HERO&&cell->m_extraInfo==own.m_id,army,counts);
            EM_ASM({window.homm3AdventureState.heroes.at(-1).target=([$0,$1,$2]);},own.m_pathTargetX,own.m_pathTargetY,own.m_pathTargetZ);
        }
    }
    if(g_combatActive&&g_combatManager){
      auto& c=*g_combatManager;
      EM_ASM({window.homm3Harness.exactCombat=({side:$0,slot:$1,round:$2,processing:!!$3,control:!!$4,mana:[$5,$6],stacks:[]});},c.m_actingSide,c.m_actingSlot,c.m_turnNumber,g_processingCombatAction,c.m_thisNetHasControl,c.m_heroes[0]?c.m_heroes[0]->m_mana:0,c.m_heroes[1]?c.m_heroes[1]->m_mana:0);
      EM_ASM({window.homm3Harness.exactCombat.secondPhase=!!$0;},c.isInSecondPhase());
      EM_ASM({window.homm3Harness.exactCombat.grid=({shade:Array.from(HEAPU8.subarray($0,$0+187)),drawn:Array.from(HEAPU8.subarray($1,$1+187)),hover:[],showGrid:!!$2,shadeLevel:$3,showMouse:!!$4,selector:$5,highlight:$6});},c.m_curDrawGridShade,c.m_lastDrawGridShade,g_config.m_showCombatGrid,g_config.m_combatShadeLevel,g_config.m_showCombatMouseHex,c.m_lastMovedArmy?c.m_lastMovedArmy->m_gridIndex:-1,c.m_highlighterOn?c.m_highlighterIndex:-1);
      EM_ASM({window.homm3Harness.exactCombat.corpses=[];window.homm3Harness.exactCombat.pow=({frame:$0,active:[]});},c.m_powFrameIndex);
      for(int side=0;side<2;++side)for(int slot=0;slot<c.m_numArmies[side];++slot)if(c.m_armies[side][slot].m_showPowEffect)EM_ASM({window.homm3Harness.exactCombat.pow.active.push([$0,$1]);},side,slot);
      for(int hex=0;hex<187;++hex)for(int i=0;i<c.m_cells[hex].m_bodiesInHex;++i)EM_ASM({window.homm3Harness.exactCombat.corpses.push([$0,$1,$2,$3]);},hex,c.m_cells[hex].m_deadArmySide[i],c.m_cells[hex].m_deadArmySlot[i],c.m_cells[hex].m_deadPartOfDouble[i]);
      for(int i=0;i<187;++i)if(c.m_cells[i].m_backgroundOffset>=0)EM_ASM({window.homm3Harness.exactCombat.grid.hover.push($0);},i);
      EM_ASM({window.homm3Harness.exactCombat.battlefield=({large:$0,terrain:$1,obstacles:[]});},c.m_largeObstacleId,c.m_terrainType);
      for(const auto& o:c.m_obstacles)EM_ASM({window.homm3Harness.exactCombat.battlefield.obstacles.push({hex:$0,sprite:UTF8ToString($1),owner:$2});},o.m_hex,o.m_shape->m_spriteName,o.m_owner);
      for(int side=0;side<2;++side)for(int slot=0;slot<c.m_numArmies[side];++slot){const auto& a=c.m_armies[side][slot];
        EM_ASM({window.homm3Harness.exactCombat.stacks.push({side:$0,slot:$1,hex:$2,count:$3,frame:$4,sequence:$5,effects:$6,speed:$7});},side,slot,a.m_gridIndex,a.m_numTroops,a.m_currFrameIndex,a.m_currFrameType,a.m_numSpellInfluences,a.m_monInfo.speed);
        EM_ASM({Object.assign(window.homm3Harness.exactCombat.stacks.at(-1),{creature:$0,morale:$1,luck:$2,shots:$3,flags:$4});},a.m_creatureType,a.getMorale(0),a.getLuck(0),a.m_monInfo.numShots,a.m_monInfo.attributes);
      }
    }else EM_ASM({window.homm3Harness.exactCombat=null;});
    EM_ASM({window.homm3Harness.video=({paused:!!$0,active:!!$1,id:$2});},BinkManager::g_playingBink.m_paused,BinkManager::g_playingBink.m_bink!=nullptr,BinkManager::g_playingBink.m_id);
    auto* win=g_windowManager?g_windowManager->topWindow():nullptr;
    if(!win){EM_ASM({window.homm3Harness.interfaceState=null;});return;}
    std::ostringstream s;s<<"{\"type\":"<<quoteBytes(typeid(*win).name())<<",\"widgets\":[";bool comma=false;
    unsigned count=0;
    for(auto* w=win->m_headWidget;w;w=w->m_nextWidget){
      if(++count>4096)throw std::runtime_error("Invalid widget chain");
      s<<(comma?",":"")<<"{\"id\":"<<w->m_id<<",\"status\":"<<w->m_status<<",\"x\":"<<win->m_x+w->m_x+HD::overlayOffsetX()<<",\"y\":"<<win->m_y+w->m_y+HD::overlayOffsetY()<<",\"width\":"<<w->m_width<<",\"height\":"<<w->m_height;
      if(auto* text=dynamic_cast<textWidget*>(w))s<<",\"text\":"<<quoteBytes(text->m_text);
      if(auto* bitmap=dynamic_cast<bitmapBorder*>(w))if(bitmap->m_image)s<<",\"image\":"<<quoteBytes(bitmap->m_image->get_Name());
      if(auto* icon=dynamic_cast<iconWidget*>(w))s<<",\"frame\":"<<icon->m_frame<<",\"sequence\":"<<icon->m_seqId;
      s<<'}';comma=true;
    }
    s<<"]";
    if(auto* townWindow=dynamic_cast<TTownScreenWindow*>(win)){
        s<<",\"hotspots\":[";bool comma=false;
        int best[44]={},bx[44]={},by[44]={};
        for(int y=0;y<374;y+=3)for(int x=0;x<800;){
            const int start=x;const int value=townWindow->m_zBuffer[y*800+x];
            while(x<800&&townWindow->m_zBuffer[y*800+x]==value)++x;
            if(value>0&&value<=44&&x-start>best[value-1]){best[value-1]=x-start;bx[value-1]=(start+x)/2;by[value-1]=y;}
        }
        for(int building=0;building<44;++building)if(best[building]>4){
            s<<(comma?",":"")<<"{\"building\":"<<building<<",\"x\":"<<bx[building]<<",\"y\":"<<by[building]<<"}";comma=true;
        }
        s<<"]";
    }
    s<<"}";const auto data=s.str();
    EM_ASM({const value=JSON.parse(UTF8ToString($0));const decoder=new TextDecoder('windows-1251');for(const w of value.widgets)if(w.text)w.text=decoder.decode(Uint8Array.from(w.text,c=>c.charCodeAt(0)));window.homm3Harness.interfaceState=value;},data.c_str());
#endif
}
uint64_t storedBuildings(const town& t){uint64_t mask=0;for(int id=0;id<44;++id)if(t.hasBuilding(id,false))mask|=uint64_t(1)<<id;return mask;}
std::string world(bool initial) {
    std::ostringstream s;
    s<<"{\"date\":["<<g_game->m_month<<','<<g_game->m_week<<','<<g_game->m_day<<"],\"size\":["<<g_mapWidth<<','<<g_mapHeight<<','<<g_game->getNumMapLevels()<<"],\"players\":[";
    for(int i=0;i<8;++i){const auto& p=g_game->m_players[i];s<<(i?",":"")<<"{\"human\":"<<int(p.m_isHuman)<<",\"disabled\":"<<int(g_game->m_playerDisabled[i])<<",\"resources\":[";for(int j=0;j<7;++j)s<<(j?",":"")<<p.m_resources[j];s<<"]}";}
    if(initial){
        s<<"],\"rosters\":[";
        for(int i=0;i<8;++i){const auto& p=g_game->m_players[i];
            s<<(i?",":"")<<"{\"count\":"<<int(p.m_numHeroes)<<",\"heroes\":[";
            for(int j=0;j<8;++j)s<<(j?",":"")<<int(p.m_heroes[j]);
            s<<"]}";
        }
    }
    if(initial){s<<"],\"buildingRules\":[";for(int faction=0;faction<9;++faction)for(int id=0;id<44;++id)s<<(faction||id?",":"")<<town::s_includedBuildings[faction][id];}
    s<<"],\"heroes\":[";bool comma=false;
    for(int i=0;i<game::HERO_COUNT;++i){const auto& h=g_game->m_heroes[i];if(h.m_owner<0||h.m_owner>=8)continue;
      s<<(comma?",":"")<<"{\"id\":"<<i<<",\"owner\":"<<int(h.m_owner)<<",\"pos\":["<<int(h.m_x)<<','<<int(h.m_y)<<','<<int(h.m_z)<<"],\"movement\":"<<h.m_movePoints<<",\"mana\":"<<h.m_mana<<",\"experience\":"<<h.m_experience<<",\"army\":[";
      for(int j=0;j<7;++j)s<<(j?",":"")<<'['<<h.m_army.m_armies[j]<<','<<h.m_army.m_numTroops[j]<<']';s<<"],\"equipped\":[";
      for(int j=0;j<=kNumArtifactSlots;++j){const auto& item=h.getArtifact(TArtifactSlot(j));s<<(j?",":"")<<'['<<int(item.m_artifactId)<<','<<item.m_extra<<']';}
      s<<"],\"backpack\":[";for(int j=0;j<HERO_BACKPACK_CAPACITY;++j){const auto& item=h.getBackpack(j);s<<(j?",":"")<<'['<<int(item.m_artifactId)<<','<<item.m_extra<<']';}
      s<<"],\"spells\":[";for(int j=0;j<hero::NUM_SPELLS;++j)s<<(j?",":"")<<int(h.isInSpellbook(SpellID(j)));
      s<<"],\"skills\":[";for(int j=0;j<28;++j)s<<(j?",":"")<<int(h.getSecondarySkill(TSecondarySkill(j)));
      s<<"],\"primary\":[";for(int j=0;j<4;++j)s<<(j?",":"")<<h.getPrimarySkill(j);
      s<<"],\"maxMovement\":"<<h.m_maxMovePoints<<",\"level\":"<<int(h.m_level)<<",\"flags\":"<<int(h.m_flags)<<",\"townRewards\":[";
      for(int j=0;j<int(g_game->m_towns.size());++j)s<<(j?",":"")<<int(h.m_townSpecialGrantedMask[j]);
      s<<"],\"visits\":["<<uint32_t(h.m_trainingGroundsFlags)<<','<<uint32_t(h.m_defenseTowerFlags)<<','<<uint32_t(h.m_gardenOfRevelationFlags)<<','<<uint32_t(h.m_mercCampFlags)<<','<<uint32_t(h.m_powerSchoolFlags)<<','<<uint32_t(h.m_treeOfKnowledgeFlags)<<','<<uint32_t(h.m_libraryFlags)<<','<<uint32_t(h.m_arenaFlags)<<','<<uint32_t(h.m_magicSchoolFlags)<<','<<uint32_t(h.m_warSchoolFlags)<<','<<uint32_t(h.m_universityFlags)<<','<<uint32_t(h.m_shrine1Flags)<<','<<uint32_t(h.m_shrine2Flags)<<','<<uint32_t(h.m_shrine3Flags);
      s<<"],\"moraleBonus\":"<<int(h.m_moraleBonus)<<",\"luckBonus\":"<<int(h.m_luckBonus)<<",\"adventureSpells\":["<<int(h.m_flightLevel)<<','<<int(h.m_waterWalkLevel)<<','<<int(h.m_disguiseLevel)<<','<<int(h.m_identifyLevel)<<','<<int(h.m_dWalkSpellsCast)<<"]}";comma=true;
    }
    s<<"],\"towns\":[";comma=false;
    for(auto& t:g_game->m_towns){s<<(comma?",":"")<<"{\"id\":"<<int(t.m_id)<<",\"type\":"<<int(t.m_type)<<",\"storedBuildings\":\""<<storedBuildings(t)<<"\",\"owner\":"<<int(t.m_owner)<<",\"visitor\":"<<t.m_visitingHeroId<<",\"garrisonHero\":"<<t.m_garrisonHeroId<<",\"builtToday\":"<<int(t.m_builtThisTurn)<<",\"population\":[";for(int j=0;j<14;++j)s<<(j?",":"")<<t.m_population[j];
      s<<"],\"buildings\":\""<<t.getBuildingMask()<<"\",\"army\":[";
      for(int j=0;j<7;++j)s<<(j?",":"")<<'['<<t.m_garrison.m_armies[j]<<','<<t.m_garrison.m_numTroops[j]<<']';
      s<<"],\"guild\":[";for(int level=0;level<5;++level)for(int j=0;j<6;++j)s<<(level||j?",":"")<<int(t.m_mageGuildSpells[level][j]);
      s<<"]}";comma=true;}
    s<<"],\"cells\":[";comma=false;
    for(int z=0;z<g_game->getNumMapLevels();++z)for(int y=0;y<g_mapHeight;++y)for(int x=0;x<g_mapWidth;++x){
      auto* cell=g_game->getCell(type_point(x,y,z));
      int type=cell->m_type,extra=cell->m_extraInfo,flags=cell->m_cellFlags;
      // Resolve only the type for pickup postconditions; other columns stay raw.
      if(!initial&&type==HERO&&extra>=0&&extra<game::HERO_COUNT){const auto& h=g_game->m_heroes[extra];type=h.m_obscuredType;}
      s<<(comma?",":"")<<'['<<int(cell->m_groundSet)<<','<<int(cell->m_groundIndex)<<','<<int(cell->m_riverSet)<<','<<int(cell->m_riverIndex)<<','<<int(cell->m_roadSet)<<','<<int(cell->m_roadIndex)<<','<<flags<<','<<type<<','<<cell->m_objectIndex<<','<<cell->m_objectTypeIndex<<','<<uint32_t(extra)<<','<<getMapExtra(x,y,z)<<']';comma=true;
    }
    s<<"]}";return s.str();
}
}
bool accelerated(){
#ifdef __EMSCRIPTEN__
    return controlled;
#else
    return true;
#endif
}
uint32_t ticks(){
#ifdef __EMSCRIPTEN__
    if(!controlled)return SDL_GetTicks();
#endif
    return clockMs;
}
void pump(unsigned milliseconds){
#ifdef __EMSCRIPTEN__
    clockMs=EM_ASM_INT({return window.homm3Harness.tick($0,$1);},milliseconds,SDL_GetTicks());
    controlled=EM_ASM_INT({return !!window.homm3Harness.controlled;});
    if(!controlled)emscripten_sleep(8);
    else if(EM_ASM_INT({return window.homm3Harness.shouldYield();}))parityYield();
#else
    clockMs+=milliseconds;
#endif
}
void decision(const char* kind,int side,int slot){
    if(std::strcmp(kind,"combat-setup")==0){
        traceRandom=true;
#ifdef __EMSCRIPTEN__
        browserCombatRng=EM_ASM_INT({return window.homm3Harness.fixture.combatRng==='browser-libc';});
#else
        const char* mode=std::getenv("HOMM3_HARNESS_COMBAT_RNG");browserCombatRng=mode&&std::strcmp(mode,"browser-libc")==0;
#endif
    }
    std::string exactRng;
#ifdef __EMSCRIPTEN__
    // Explicit fault-injection fixture: only the authority renderer's local
    // random field changes. The trusted worker and reference RNG stay intact.
    if(std::strcmp(kind,"combat-open")==0&&EM_ASM_INT({return !!window.homm3Authority?.active&&Number.isInteger(window.homm3Harness.fixture.harnessFaults?.authorityCombatSeed);})){const auto value=EM_ASM_INT({return window.homm3Harness.fixture.harnessFaults.authorityCombatSeed;});if(browserCombatRng)BrowserRng::seed(value);else rng=value;}
#endif
#ifdef __EMSCRIPTEN__
    exactRng=std::to_string(browserCombatRng?BrowserRng::state():uint64_t(rng));
    EM_ASM({window.homm3Harness.event('decision',({kind:UTF8ToString($0),side:$1,slot:$2,rng:UTF8ToString($3),algorithm:$4?'browser-libc':'msvc-lcg32'}));},kind,side,slot,exactRng.c_str(),browserCombatRng);
#else
    if(const char* dir=std::getenv("HOMM3_HARNESS_OUTPUT")){
        std::ofstream file(std::string(dir)+"/decisions.jsonl",std::ios::app);
        file<<"{\"kind\":\""<<kind<<"\",\"side\":"<<side<<",\"slot\":"<<slot<<",\"rng\":\""<<(browserCombatRng?BrowserRng::state():uint64_t(rng))<<"\",\"algorithm\":\""<<(browserCombatRng?"browser-libc":"msvc-lcg32")<<"\"}\n";
    }
#endif
}
int random(int lower,int upper){if(traceRandom)decision("rng-draw",lower,upper);if(browserCombatRng)return BrowserRng::next();rng=rng*214013u+2531011u;return (rng>>16)&0x7fff;}
void seed(unsigned value){if(browserCombatRng)BrowserRng::seed(value);else rng=value;if(traceRandom)decision("rng-seed",0,0);}
void arm(){
    clockMs=1000000;
#ifdef __EMSCRIPTEN__
    controlled=true;
#endif
    for(auto& timer:g_timers)timer=clockMs;
    g_advManager->m_animFrame=0;g_advManager->m_animCtr=0;
    g_soundManager->stopAllSamples(1);
}
void checkpoint(const char* label){
    interfaceSnapshot();
    const auto data=world(std::strcmp(label,"action")!=0);
#ifdef __EMSCRIPTEN__
    const auto exactRng=std::to_string(BrowserRng::state());
    EM_ASM({window.homm3Harness.world(UTF8ToString($0),JSON.parse(UTF8ToString($1)),$3?UTF8ToString($4):$2>>>0);},label,data.c_str(),rng,browserCombatRng,exactRng.c_str());
#else
    if(const char* dir=std::getenv("HOMM3_HARNESS_OUTPUT")){
        std::ofstream out(std::string(dir)+"/"+label+".json");if(!out)throw std::runtime_error("Cannot write harness checkpoint");out<<"{\"world\":"<<data<<",\"rng\":";if(browserCombatRng)out<<'"'<<BrowserRng::state()<<'"';else out<<rng;out<<"}\n";
    }
#endif
}
void exportFixture(const char* path){
#ifndef __EMSCRIPTEN__
    if(const char* feedback=std::getenv("HOMM3_HARNESS_FEEDBACK")){
        auto& h=g_game->m_heroes[g_game->m_players[0].m_heroes[0]];
        h.m_army.initialize();h.m_army.add(CREATURE_MARKSMAN,20,-1);h.m_army.add(CREATURE_ARCHER,20,-1);h.m_army.add(CREATURE_GRIFFIN,20,-1);
        h.m_moraleBonus=std::strcmp(feedback,"positive")==0?3:std::strcmp(feedback,"negative")==0?-6:0;
        h.m_luckBonus=std::strcmp(feedback,"positive")==0?3:(std::strcmp(feedback,"negative")==0||std::strcmp(feedback,"unlucky")==0)?-3:0;
        h.m_movePoints=h.m_maxMovePoints=h.getMobility();h.m_mana=0;
        auto& guard=*g_game->getCell(type_point(5,8,0));
        if(guard.m_type!=MONSTER)throw std::runtime_error("Feedback fixture requires Arrogance guard at 5,8");
        guard.m_extraInfo=(guard.m_extraInfo&~4095u)|300u;
        if(guard.m_objectIndex>=0)g_game->m_worldMap.m_objects[guard.m_objectIndex].m_extraInfo=guard.m_extraInfo;
    }
#endif
#ifndef __EMSCRIPTEN__
    if(const char* profile=std::getenv("HOMM3_HARNESS_TOWN")){
        for(auto& player:g_game->m_players)if(player.m_isHuman){
            for(auto& resource:player.m_resources)resource=1000000;
            for(int i=0;i<player.m_numTowns;++i){auto& t=g_game->m_towns[player.m_townIds[i]];
                for(int id:{MAGE_GUILD_ID,TAVERN_ID,MARKETPLACE_ID,BLACKSMITH_ID,DWELLING_0_ID,DWELLING_1_ID,DWELLING_0_UPG_ID})t.buildBuilding(id,false,false);
                if(std::strcmp(profile,"full")==0)for(int pass=0;pass<44;++pass)for(int id=0;id<44;++id)
                    if(id!=HOLY_GRAIL_ID&&id!=DOCK_WITH_BOAT_ID&&(t.getBuildableMask()&g_bitNumber[id]))t.buildBuilding(id,false,false);
                t.m_builtThisTurn=0;
                for(int slot=0;slot<14;++slot)t.m_population[slot]=50;
                t.m_garrison.initialize();t.m_garrison.add(g_dwellingType[t.m_type][0],20,-1);t.m_garrison.add(g_dwellingType[t.m_type][1],10,-1);
            }
            for(int i=0;i<player.m_numHeroes;++i){auto& h=g_game->m_heroes[player.m_heroes[i]];
                h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,40,-1);h.m_army.add(CREATURE_ARCHER,20,-1);
                h.m_movePoints=h.m_maxMovePoints=h.getMobility();
            }
        }
    }
#endif
#ifndef __EMSCRIPTEN__
    if(const char* requested=std::getenv("HOMM3_HARNESS_ADVENTURE")){
        const int type=std::atoi(requested);auto& player=g_game->m_players[0];auto& h=g_game->m_heroes[player.m_heroes[0]];
        type_point target(-1,-1,-1),start(-1,-1,-1);
        for(int pass=0;pass<2&&target.m_x<0;++pass)
        for(int z=0;z<g_game->getNumMapLevels()&&target.m_x<0;++z)
          for(int y=1;y<g_mapHeight-2&&target.m_x<0;++y)
            for(int x=1;x<g_mapWidth-1;++x){
                auto* cell=g_game->getCell(type_point(x,y,z));auto* below=g_game->getCell(type_point(x,y+1,z));
                if(cell->m_type!=type||!cell->m_isTrigger||(below->m_type!=NOTHING&&below->m_type!=MONSTER)||(!below->m_passable&&below->m_type!=MONSTER)||(below->m_isBlocked&&below->m_type!=MONSTER)||below->m_groundSet==eTerrainWater)continue;
                bool guarded=false;for(int dy=-1;dy<=2;++dy)for(int dx=-1;dx<=1;++dx)if(g_game->getCell(type_point(x+dx,y+dy,z))->m_type==MONSTER)guarded=true;
                if(guarded&&pass==0)continue;
                target=type_point(x,y,z);start=type_point(x,y+1,z);break;
            }
        if(target.m_x<0)throw std::runtime_error("No adventure object with a usable southern approach");
        g_advManager->m_fullMap=&g_game->m_worldMap;
        int removedGuards=0;
        for(int dy=-1;dy<=2;++dy)for(int dx=-1;dx<=1;++dx){type_point p(target.m_x+dx,target.m_y+dy,target.m_z);auto* cell=g_game->getCell(p);if(cell->m_type==MONSTER){g_advManager->eraseObj(cell,p,false);++removedGuards;}}
        h.restoreCell();h.m_x=start.m_x;h.m_y=start.m_y;h.m_z=start.m_z;h.obscureCell();
        for(auto& resource:player.m_resources)resource=100000;
        h.m_army.initialize();h.m_army.add(CREATURE_PIKEMAN,40,-1);h.m_army.add(CREATURE_ARCHER,20,-1);
        h.m_movePoints=h.m_maxMovePoints=h.getMobility();h.m_mana=0;
        g_game->setVisibility(h.m_x,h.m_y,h.m_z,0,8,false);
        // Fix the chest payload while preserving its actual sprite/trigger.
        if(type==TREASURE_CHEST){auto* cell=g_game->getCell(target);cell->m_extraInfo=0;cell->m_treasureInfo.m_gold=2;}
        if(const char* dir=std::getenv("HOMM3_HARNESS_OUTPUT")){
            std::ofstream out(std::string(dir)+"/adventure-fixture.json");
            out<<"{\"removedGuards\":"<<removedGuards<<",\"type\":"<<type<<",\"hero\":"<<int(h.m_id)<<",\"target\":["<<int(target.m_x)<<','<<int(target.m_y)<<','<<int(target.m_z)<<"],\"start\":["<<int(start.m_x)<<','<<int(start.m_y)<<','<<int(start.m_z)<<"]}";
        }
    }
#endif
#ifndef __EMSCRIPTEN__
    if(std::getenv("HOMM3_HARNESS_MOVEMENT")){
        // Explicit cheat fixture: a disclosed flat corridor long enough to
        // exhaust a day, with no object/guard ambiguity in the walking test.
        auto& h=g_game->m_heroes[g_game->m_players[0].m_heroes[0]];h.restoreCell();
        for(int y=13;y<=17;++y)for(int x=2;x<=33;++x){auto& c=*g_game->getCell(type_point(x,y,0));
            c.m_type=NOTHING;c.m_extraInfo=0;c.m_objectIndex=-1;c.m_objectTypeIndex=-1;c.m_objects.clear();
            c.m_cellFlags=0;c.m_passable=1;c.m_groundSet=eTerrainGrass;c.m_roadSet=0;c.m_riverSet=0;
            *getMapExtraPtr(x,y,0)=1;
        }
        h.m_x=3;h.m_y=15;h.m_z=0;h.obscureCell();g_game->setupAdjacentMons();
        h.m_movePoints=h.m_maxMovePoints=h.getMobility();
        if(std::getenv("HOMM3_HARNESS_EXCHANGE")){
            auto& other=g_game->m_heroes[16];
            if(other.m_owner>=0)throw std::runtime_error("Exchange fixture hero already owned");
            other.initialize(16);other.placeInMap(0,type_point(9,15,0),true);
            other.m_army.initialize();other.m_army.add(CREATURE_PIKEMAN,20,-1);
            other.m_movePoints=other.m_maxMovePoints=other.getMobility();
        }
    }
#endif
#ifndef __EMSCRIPTEN__
    if(std::getenv("HOMM3_HARNESS_MAGIC")){
        auto& h=g_game->m_heroes[g_game->m_players[0].m_heroes[0]];
        h.giveArtifact(type_artifact(ARTIFACT_SPELLBOOK),false,false);
        for(int spell=0;spell<10;++spell)h.addSpell(spell);
        h.giveSS(eSecSkillSchoolOfEarthMagic,3);h.giveSS(eSecSkillSchoolOfAirMagic,3);h.giveSS(eSecSkillSchoolOfWaterMagic,3);h.giveSS(eSecSkillWisdom,3);
        h.m_mana=500;
    }
#endif
#ifndef __EMSCRIPTEN__
    if(const char* dir=std::getenv("HOMM3_HARNESS_OUTPUT")){
        std::ofstream setup(std::string(dir)+"/fixture-setup.json");setup<<'['<<int(g_game->m_setup.m_difficulty);
        for(int i=0;i<8;++i)setup<<','<<int(g_game->m_setup.m_playerPos[i])<<','<<int(g_game->m_setup.m_alignment[i])<<','<<g_game->m_setup.m_startingHero[i]<<','<<g_newMapStartingBonus[i]<<','<<int(g_game->m_setup.m_handicap[i]);
        setup<<']';
    }
#endif
    TGzFile output(path,"wb6");if(g_game->save(&output)<0)throw std::runtime_error("Cannot export fixture");
}
bool loadFixture(){
    if(loaded)return false;
#ifdef __EMSCRIPTEN__
    if(!EM_ASM_INT({return window.homm3Harness.installFixture(FS);}))return false;
    const char* path="/runtime/data/RMTparity.gm2";
#else
    const char* path=std::getenv("HOMM3_HARNESS_SAVE");if(!path)return false;
#endif
    const int local=g_localGamePos,watch=g_curWatchPlayer;
    unsigned dpids[8];int locals[8];for(int i=0;i<8;++i){dpids[i]=g_game->m_players[i].m_dpid;locals[i]=g_game->m_players[i].m_isLocal;}
    {TGzFile input(path,"rb");if(g_game->load(&input)<0)throw std::runtime_error("Harness save failed to load");}
    for(int i=0;i<8;++i){g_game->m_players[i].m_dpid=dpids[i];g_game->m_players[i].m_isLocal=locals[i];}
    g_localGamePos=local;g_curWatchPlayer=watch;g_mapVisibilityBit=1<<watch;
    g_playerTurn=g_netLocalGamePos;g_currentPlayer=&g_game->m_players[g_netLocalGamePos];
#ifdef __EMSCRIPTEN__
    rng=EM_ASM_INT({return window.homm3Harness.fixture.rng;});
#else
    const char* state=std::getenv("HOMM3_HARNESS_RNG");if(!state)throw std::runtime_error("Save requires RNG sidecar");rng=std::stoul(state);
#endif
#ifdef __EMSCRIPTEN__
    int setup[41];
    if(EM_ASM_INT({const setup=window.homm3Harness.fixture.setup;if(!Array.isArray(setup)||setup.length!==41)return 0;HEAP32.set(setup,$0>>2);return 1;},setup)){
        g_game->m_setup.m_difficulty=setup[0];
        for(int i=0;i<8;++i){const int* row=setup+1+i*5;g_game->m_setup.m_playerPos[i]=row[0];g_game->m_setup.m_alignment[i]=TTownType(row[1]);g_game->m_setup.m_startingHero[i]=row[2];g_newMapStartingBonus[i]=row[3];g_game->m_setup.m_startingBonus[i]=row[3];g_game->m_setup.m_handicap[i]=row[4];}
    }
#endif
    #ifdef __EMSCRIPTEN__
    if(EM_ASM_INT({return window.homm3Harness.fixture.settings?.combatGrid===true;})){g_config.m_showCombatGrid=1;g_config.m_combatShadeLevel=1;g_config.m_showCombatMouseHex=1;}
    const int walk=EM_ASM_INT({return window.homm3Harness.fixture.settings?.heroSpeed??-1;});if(walk>=0&&walk<=4)g_config.m_walkSpeed[1]=walk;
#endif
    loaded=true;checkpoint("fixture-loaded");
#ifdef __EMSCRIPTEN__
    // Start deterministic time before first-turn UI, ambient sound and palette
    // animation can consume wall-clock-dependent random draws.
    if(EM_ASM_INT({return window.homm3Harness.clock==='virtual';})) {
        clockMs=1000000;controlled=true;
        for(auto& timer:g_timers)timer=clockMs;
        EM_ASM({window.homm3Harness.t=1000000;window.homm3Harness.controlled=true;window.homm3Harness.fixtureFrozen=true;});
    }
#endif
    return true;
}
void frame(const Bitmap16Bit& bitmap){
#ifdef __EMSCRIPTEN__
    if(!EM_ASM_INT({return !!window.homm3Harness.recording;}))return;
    interfaceSnapshot();
    // Hash every RGB565 pixel, including overlays/cursor, in 32x32 tiles.
    // This taps the final engine composition, without browser screenshots.
    auto& b=const_cast<Bitmap16Bit&>(bitmap);std::vector<uint32_t> hashes;
    for(int y=0;y<b.GetHeight();y+=32)for(int x=0;x<b.GetWidth();x+=32){
      uint32_t h=2166136261u;
      for(int v=y;v<std::min(y+32,b.GetHeight());++v){auto* row=b.GetMap(0,v);for(int u=x;u<std::min(x+32,b.GetWidth());++u){h=(h^(row[u]&255))*16777619u;h=(h^(row[u]>>8))*16777619u;}}
      hashes.push_back(h);
    }
    EM_ASM({window.homm3Harness.frame($0,$1,Array.from(HEAPU32.subarray($2>>>2,($2>>>2)+$3)));},b.GetWidth(),b.GetHeight(),hashes.data(),hashes.size());
#else
    (void)bitmap;
#endif
}
} }

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void homm3_harness_arm(){Port::Harness::arm();}
extern "C" EMSCRIPTEN_KEEPALIVE void homm3_harness_checkpoint(){Port::Harness::checkpoint("action");}
#endif
