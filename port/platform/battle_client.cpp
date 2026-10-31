#include "soundmgr.h"
#include "advmgr.h"
#include "battle_client.h"
#include "authority_client.h"
#include "presentation_pointer.h"
#include "combat_presentation.h"
#include "combat_feedback.h"
#include "combat_display.h"
#include "kb.h"
#include "misc.h"
#include "spelldefs.h"
#include "game.h"
#include "hero.h"
#include "herotraits.h"
#include "town.h"
#include "cmbtmgr.h"
#include "combatwindow.h"
#include "combatresultswindow.h"
#include "creaturetype.h"
#include "text.h"
#include "command.h"
#include "message.h"
#include "exec.h"
#include "winmgr.h"
#include "inputmgr.h"
#include "resourcemanager.h"
#include "findpath.h"
#include "widget.h"
#include <vector>
#include <string>
#include <stdexcept>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace {
bool running=false;
int visualSerial=0;
std::vector<int32_t> packet;
armyGroup groups[2];
struct Stack {
    int side,slot,creature,count,hex,damage,flags,facing,originalIndex,originalCount;
    int attack,defense,speed,hp,shots,morale,luck,retaliations;
    std::vector<int> effects,effectOrder;
    Port::Authority::CombatModifiers modifiers;
};
struct Obstacle {int hex,owner,duration;std::string sprite;};
struct HeroState {int type,owner,level,portrait,formation,stats[4];std::vector<int> privateWords;};
struct Snapshot {
    int presentationVersion=1;
    int round,side,actingSide,actingSlot,tactics,x,y,z,terrain,magic,town,fort,drawbridge,surrounded,large;
    int players[2],heroId[2],mana[2],cast[2],placement;HeroState heroes[2];
    std::vector<Stack> stacks;std::vector<int> walls,cells;std::vector<Obstacle> obstacles;
    std::vector<Port::Authority::CombatVisualRecord> events;
} snapshot;
struct Read {
    size_t offset=0;
    int get(int lo=INT32_MIN,int hi=INT32_MAX){if(offset>=packet.size())throw std::runtime_error("Truncated combat projection");int n=packet[offset++];if(n<lo||n>hi)throw std::runtime_error("Combat projection range");return n;}
};
bool take(){
#ifdef __EMSCRIPTEN__
    int size=EM_ASM_INT({return window.homm3Authority.combatLength();});if(!size)return false;
    if(size<0||size>65536)throw std::runtime_error("Combat projection limit");packet.resize(size);
    EM_ASM({window.homm3Authority.takeCombat(HEAP32,$0>>2);},packet.data());
    Read r;Snapshot s;
    s.presentationVersion=EM_ASM_INT({return window.homm3Authority.combatPresentationVersion;});
    s.round=r.get(0,1000000);s.side=r.get(0,1);s.actingSide=r.get(0,1);s.actingSlot=r.get(0,20);s.tactics=r.get(0,1);
    s.x=r.get(0,g_mapWidth-1);s.y=r.get(0,g_mapHeight-1);s.z=r.get(0,g_game->getNumMapLevels()-1);s.terrain=r.get(0,9);s.magic=r.get();s.town=r.get(-1,int(g_game->m_towns.size())-1);s.fort=r.get(0,3);s.drawbridge=r.get();s.surrounded=r.get(0,1);s.large=r.get(-1,33);
    for(int i=0;i<2;++i)s.players[i]=r.get(-1,7);
    for(int i=0;i<2;++i){s.heroId[i]=r.get(-1,game::HERO_COUNT-1);s.mana[i]=r.get(0,32767);auto& h=s.heroes[i];h.type=r.get(0,17);h.owner=r.get(-1,7);h.level=r.get(0,32767);h.portrait=r.get(0,255);h.formation=r.get(0,255);for(auto& v:h.stats)v=r.get(-128,127);if(r.get(0,1))for(int j=0;j<206;++j)h.privateWords.push_back(r.get());}
    for(int i=0;i<2;++i)s.cast[i]=r.get(0,1);s.placement=r.get(-3,3);
    int count=r.get(1,42);bool seen[2][21]={};
    for(int i=0;i<count;++i){Stack a;a.side=r.get(0,1);a.slot=r.get(0,20);if(seen[a.side][a.slot])throw std::runtime_error("Duplicate stack");seen[a.side][a.slot]=true;
        a.creature=r.get(0,200);a.count=r.get(0);a.hex=r.get(-1,255);a.damage=r.get(0);a.flags=r.get();a.facing=r.get(0,1);a.originalIndex=r.get(-1,20);a.originalCount=r.get(0);
        a.attack=r.get();a.defense=r.get();a.speed=r.get();a.hp=r.get(1);a.shots=r.get(0);a.morale=r.get();a.luck=r.get();a.retaliations=r.get(0);
        int effects=r.get(0,81);for(int j=0;j<effects;++j){a.effects.push_back(r.get(0,80));a.effects.push_back(r.get(0));a.effects.push_back(r.get(0,3));}for(auto& value:a.modifiers)value=r.get();int order=r.get(0,81);for(int j=0;j<order;++j)a.effectOrder.push_back(r.get(0,80));s.stacks.push_back(std::move(a));}
    count=r.get(0,32);for(int i=0;i<count;++i)s.walls.push_back(r.get(0,3));
    count=r.get(0,187);for(int i=0;i<count;++i){Obstacle o;o.hex=r.get(0,186);o.owner=r.get(-1,1);o.duration=r.get(-1);int n=r.get(1,80);for(int j=0;j<n;++j)o.sprite.push_back(char(r.get(32,126)));s.obstacles.push_back(std::move(o));}
    count=r.get(187,187);for(int i=0;i<count*49;++i)s.cells.push_back(r.get());
    count=r.get(0,Port::Authority::CombatVisualLimit);
    for(int i=0;i<count;++i){Port::Authority::CombatVisualRecord event;for(auto& word:event)word=r.get();s.events.push_back(event);}
    if(r.offset!=packet.size())throw std::runtime_error("Trailing combat projection");snapshot=std::move(s);return true;
#else
    return false;
#endif
}
}
struct AuthorityBattleAccess {
    static const combatManager::TObstacleInfo* shape(const std::string& name){
        for(int i=0;i<91;++i)if(name==combatManager::s_obstacleInfo[i].m_spriteName)return &combatManager::s_obstacleInfo[i];
        if(name==combatManager::s_quicksandInfo[0].m_spriteName)return &combatManager::s_quicksandInfo[0];
        if(name==combatManager::s_landMineInfo[0].m_spriteName)return &combatManager::s_landMineInfo[0];
        for(int i=0;i<5;++i)if(name==combatManager::s_wallObstacleInfo[i].m_spriteName)return &combatManager::s_wallObstacleInfo[i];
        throw std::runtime_error("Unknown canonical obstacle sprite");
    }
    static void heroes(){
        for(int side=0;side<2;++side)if(snapshot.heroId[side]>=0){const auto& src=snapshot.heroes[side];auto& h=g_game->m_heroes[snapshot.heroId[side]];
            h.m_heroClass=static_cast<THeroClass>(src.type);h.m_owner=src.owner;h.m_level=src.level;h.m_portrait=src.portrait;h.m_formation=src.formation;h.m_mana=snapshot.mana[side];
            for(int i=0;i<4;++i)h.m_stats[i]=src.stats[i];
#ifdef __EMSCRIPTEN__
            EM_ASM({window.homm3Authority.copyCombatHeroName($0,HEAPU8.subarray($1,$1+$2),$2);},side,h.m_name,int(sizeof(h.m_name)));
#endif
            if(!src.privateWords.empty()){size_t i=0;for(auto& v:h.m_skillLevel)v=src.privateWords[i++];for(auto& v:h.m_inSpellbook)v=src.privateWords[i++];for(auto& v:h.m_availableSpells)v=src.privateWords[i++];for(auto& a:h.m_equipped){a.m_artifactId=static_cast<TArtifact>(src.privateWords[i++]);a.m_extra=src.privateWords[i++];}}
        }
    }
    static void configure(){
        auto& c=*g_combatManager;const auto& s=snapshot;
        c.m_fortificationLevel=static_cast<TFortificationLevel>(s.fort);
        c.m_terrainType=s.terrain;c.m_magicTerrain=s.magic;c.m_largeObstacleId=s.large;
        c.m_creaturePlacement=s.tactics;c.m_placementBoundaryDepth=s.placement;
        for(int side=0;side<2;++side)if(s.players[side]>=0)g_game->m_players[s.players[side]].m_quickCombat=0;
    }
    // Match getControl's visual enablement without running its spell AI.
    static void controls(bool ready){
#ifdef __EMSCRIPTEN__
        auto& c=*g_combatManager;
        if(!c.m_combatWindow)return;
        const bool canAct=ready && !g_processingCombatAction && EM_ASM_INT({return window.homm3Authority.combatCanAct();});
        bool changed=false;
        auto button=[&](int id,bool on){
            auto* w=c.m_combatWindow->getWidget(id);if(!w)return;
            const auto before=w->m_status;
            if(on){c.m_combatWindow->widgetClearStatus(id,widget::WIDGET_DIMMED_NODRAW);c.m_combatWindow->widgetClearStatus(id,widget::WIDGET_DISABLED);}
            else c.m_combatWindow->widgetSetStatus(id,widget::WIDGET_DIMMED_NODRAW);
            changed|=before!=w->m_status;
        };
        auto offered=[&](int action){return canAct && EM_ASM_INT({return window.homm3Authority.combatOffers($0);},action);};
        button(TCombatWindow::COMBAT_LEFT_COMMAND_0_ID,offered(5));
        button(TCombatWindow::COMBAT_LEFT_COMMAND_1_ID,offered(4));
        button(TCombatWindow::COMBAT_LEFT_COMMAND_2_ID,canAct);
        button(TCombatWindow::COMBAT_LEFT_COMMAND_3_ID,ready && EM_ASM_INT({return window.homm3Authority.combatCanAuto();}));
        button(TCombatWindow::COMBAT_RIGHT_COMMAND_0_ID,!c.m_creaturePlacement&&offered(101));
        button(TCombatWindow::COMBAT_RIGHT_COMMAND_1_ID,!c.m_creaturePlacement&&offered(8));
        button(TCombatWindow::COMBAT_RIGHT_COMMAND_2_ID,!c.m_creaturePlacement&&offered(3));
        if(c.m_creaturePlacement){button(TCombatWindow::COMBAT_PLACEMENT_COMMAND_0_ID,offered(8));button(TCombatWindow::COMBAT_PLACEMENT_COMMAND_1_ID,offered(100));}
        // NODRAW changes only the flags. drawFrame never repaints this bar.
        if(changed&&ready){c.m_combatWindow->drawWindow(0,WINDOW_ALL_WIDGETS_LOW,WINDOW_ALL_WIDGETS_HIGH);g_windowManager->updateScreen(0,556,800,44);}
#endif
    }
    static void clearInteraction(){
        auto& c=*g_combatManager;c.m_thisNetHasControl=0;
        c.updateMouseGrid(-1,1);
        std::fill_n(c.m_curDrawGridShade,187,0);
        if(c.updateGrid(0,0))c.drawFrame(1,0,0,0,1,0);
        controls(true);
    }
    static void project(bool ready=true){
        auto& c=*g_combatManager;auto& s=snapshot;heroes();const bool wasPlacement=c.m_creaturePlacement;
        c.m_turnNumber=s.round;c.m_currentSide=s.side;c.m_actingSide=s.actingSide;c.m_actingSlot=s.actingSlot;c.m_creaturePlacement=s.tactics;c.m_placementBoundaryDepth=s.placement;
        c.m_terrainType=s.terrain;c.m_magicTerrain=s.magic;c.m_largeObstacleId=s.large;c.m_drawbridgeState=s.drawbridge;
#ifdef __EMSCRIPTEN__
        c.m_thisNetHasControl=ready && EM_ASM_INT({return window.homm3Authority.combatCanAct();});
#else
        c.m_thisNetHasControl=0;
#endif
#ifdef __EMSCRIPTEN__
        c.m_autoCombatOn=EM_ASM_INT({return window.homm3Authority.combatAuto();});
        c.m_inSecondPhase=EM_ASM_INT({return window.homm3Authority.combatSecondPhase;});
#else
        c.m_autoCombatOn=0;
#endif
        c.m_nextAction=0;c.m_nextActionExtra=-1;c.m_nextActionGridIndex=-1;c.m_nextActionGridIndex2=-1;
        c.m_numArmies[0]=0;c.m_numArmies[1]=0;
        for(int side=0;side<2;++side){c.m_playerIds[side]=s.players[side];c.IsHuman[side]=s.players[side]>=0&&g_game->isHuman(s.players[side]);c.IsLocalHuman[side]=s.players[side]==g_localGamePos;c.m_spellsCast[side]=s.cast[side];if(c.m_heroes[side])c.m_heroes[side]->m_mana=s.mana[side];
            for(auto& a:c.m_armies[side])a.m_numTroops=0;}
        for(const auto& a:s.stacks){auto& out=c.m_armies[a.side][a.slot];
            if(out.m_creatureType!=a.creature||!out.m_stdIcon){out.initialize(static_cast<TCreatureType>(a.creature),a.count,c.m_heroes[a.side],a.side,a.slot,a.hex);out.loadResources();}
            // A restored snapshot has corpse cells but no preceding death animation.
            // Install the terminal pose directly; normal live replay already ends here.
            if(a.count==0 && out.m_stdIcon->IsValidSeq(cs_death)){
                out.m_currFrameType=cs_death;out.m_currFrameIndex=std::max(0,out.m_stdIcon->GetNumFrames(cs_death)-1);
            }
            out.m_numTroops=a.count;out.m_gridIndex=a.hex;out.m_topCreatureDamage=a.damage;out.m_monInfo.attributes=uint32_t(a.flags);out.m_facing=a.facing;out.m_originalIndex=a.originalIndex;out.m_origNumTroops=a.originalCount;
            out.m_monInfo.attackSkill=a.attack;out.m_monInfo.defenseSkill=a.defense;out.m_monInfo.speed=a.speed;out.m_monInfo.hitPoints=a.hp;out.m_monInfo.numShots=a.shots;out.m_morale=a.morale;out.m_luck=a.luck;out.m_retaliationCount=a.retaliations;
            std::fill_n(out.m_spellInfluence,81,0);std::fill_n(out.m_spellLevel,81,0);
            out.m_numSpellInfluences=0;out.m_spellInfluenceQueue.clear();
            for(size_t i=0;i<a.effects.size();i+=3){out.m_spellInfluence[a.effects[i]]=a.effects[i+1];out.m_spellLevel[a.effects[i]]=a.effects[i+2];if(a.effects[i+1]>0){++out.m_numSpellInfluences;}}
            for(int spell:a.effectOrder)out.m_spellInfluenceQueue.push_back(SpellID(spell));
            Port::Authority::applyCombatModifiers(out,a.modifiers);
            out.m_monFrameInfo.m_walkCycleTime=out.m_origWalkCycleTime*(out.m_spellInfluence[SPELL_HASTE]?0.65:out.m_spellInfluence[SPELL_SLOW]?1.5:1.0);
#ifdef __EMSCRIPTEN__
            int spellState[2]={};if(EM_ASM_INT({return window.homm3Authority.copyCombatSpellState($0,$1,HEAP32,$2>>2);},a.side,a.slot,spellState)){out.m_faerieDragonSpell=spellState[0];out.m_monInfo.m_hasSpell=spellState[1];}
#endif
            c.m_numArmies[a.side]=std::max(c.m_numArmies[a.side],a.slot+1);
        }
        if(s.walls.size()!=combatManager::kNumWallSections)throw std::runtime_error("Combat wall schema");
        for(size_t i=0;i<s.walls.size();++i)c.m_wallLevel[i]=s.walls[i];
        for(auto& o:c.m_obstacles)if(o.m_sprite)ResourceManager::Dispose(o.m_sprite);c.m_obstacles.clear();
        for(const auto& o:s.obstacles){combatManager::TObstacle out={};out.m_shape=shape(o.sprite);out.m_sprite=ResourceManager::GetSprite(out.m_shape->m_spriteName);out.m_hex=o.hex;out.m_owner=o.owner;out.m_isVisible=1;out.m_duration=o.duration;c.m_obstacles.push_back(out);}
        size_t p=0;for(auto& cell:c.m_cells){cell.m_attributes=s.cells[p++];cell.m_armySide=s.cells[p++];cell.m_armySlot=s.cells[p++];cell.m_partOfDouble=s.cells[p++];cell.m_bodiesInHex=s.cells[p++];cell.m_validMove=s.cells[p++];cell.m_frontMove=s.cells[p++];
            for(auto& v:cell.m_deadArmySide)v=s.cells[p++];for(auto& v:cell.m_deadArmySlot)v=s.cells[p++];for(auto& v:cell.m_deadPartOfDouble)v=s.cells[p++];cell.m_obstacleIndex=-1;}
        for(size_t i=0;i<c.m_obstacles.size();++i){const auto& o=c.m_obstacles[i];c.m_cells[o.m_hex].m_obstacleIndex=i;for(int j=0;j<o.m_shape->m_extraHexCount;++j){int hex=int(o.m_hex)+o.m_shape->m_extraHexOffsets[j];if(hex>=0&&hex<187)c.m_cells[hex].m_obstacleIndex=i;}}
        if(wasPlacement&&!s.tactics&&c.m_combatWindow)c.m_combatWindow->endPlacementPhase();
        controls(ready);
        // The original only shades a locally controlled human stack. Opening
        // observations and the opponent's turns must never expose their range.
        c.updateMouseGrid(-1,1);
        std::fill_n(c.m_curDrawGridShade,187,0);
        if(ready)c.checkChangeSelector();
        if(c.m_thisNetHasControl && !c.isComputerAction() && c.IsLocalHuman[c.getCurrentArmy()->getControllingSide()])
            c.setupGridForArmy(c.getCurrentArmy());
        else std::fill_n(c.m_curDrawGridShade,187,0);
        c.m_backgroundDrawn=0;c.m_lastCellIndex=-1;c.m_lastCommand=-99;
        if(ready)c.resetMouse();
#ifdef __EMSCRIPTEN__
        EM_ASM({window.homm3Authority.rendererReady(!!$0);},ready);
#endif
    }
    static void replay(){
#ifdef __EMSCRIPTEN__
        using namespace Port::Authority;
        auto& c=*g_combatManager;
        if(snapshot.events.empty()||snapshot.events.back()[0]<=visualSerial)return;
        // Reloads start at the latest canonical image. A bounded-history gap
        // likewise reconciles directly instead of animating an invented path.
        if(snapshot.events.front()[0]>visualSerial+1){visualSerial=snapshot.events.back()[0];return;}
        const auto processing=g_processingCombatAction;
        struct Guard {int previous;~Guard(){g_processingCombatAction=previous;EM_ASM({window.homm3Authority.presentationBusy(false);});}} guard{processing};
        EM_ASM({window.homm3Authority.presentationBusy(true);});
        g_processingCombatAction=1;clearInteraction();
        const bool impactReplay=snapshot.presentationVersion>=2;
        struct Hit {std::string source;int count;army* target;int damage,killed;};
        std::vector<Hit> hits;
        std::string source;int sourceCount=1;int castingSide=-1;int action=0;
        // Original ProcessNextAction resets idle animations around physical
        // actions and spells. Replaying only movement/effects left unrelated
        // stacks stranded in the middle of their previous fidget sequence.
        auto finishAction=[&](){
            if(action==1||action==2||action==4||action==5||action==6||action==7||action==9||action==10)c.resetCycleTimers();
            action=0;
        };
        auto finishCast=[&](){if(castingSide<0)return;auto* sprite=c.cmbtHero[castingSide];for(int frame=c.cmbtHeroFrameIndex[castingSide]+1;frame<sprite->GetNumFrames(4);++frame){c.cmbtHeroFrameIndex[castingSide]=frame;c.drawFrame(1,0,0,100,1,1);}c.cmbtHeroFrameType[castingSide]=0;c.cmbtHeroFrameIndex[castingSide]=0;castingSide=-1;};
        auto stack=[&](int side,int slot)->army*{return side>=0&&side<2&&slot>=0&&slot<c.m_numArmies[side]&&c.m_armies[side][slot].m_stdIcon?&c.m_armies[side][slot]:nullptr;};
        for(const auto& e:snapshot.events){
            if(e[0]<=visualSerial)continue;visualSerial=e[0];
            army* a=stack(e[2],e[3]);if(!a)continue;
            c.m_actingSide=e[2];c.m_actingSlot=e[3];
            if(e[4]>=0&&e[4]<187&&(a->m_gridIndex!=e[4]||a->m_facing!=e[5])){
                c.removeArmyFromGrid(*a);a->m_gridIndex=e[4];a->m_facing=e[5];if(a->m_numTroops>0)c.placeArmyInGrid(*a,e[4]);
            }
            switch(e[1]){
            case VisualWalk:
                if(e[6]>=0&&e[6]<6&&a->m_gridIndex>=0&&a->m_gridIndex<187)a->walk(e[6],e[7]!=0,e[8]!=0);
                break;
            case VisualFly:
                if(e[6]>=0&&e[6]<187&&a->m_gridIndex>=0&&a->m_gridIndex<187)a->fly(e[6]);
                break;
            case VisualStrike:{
                source=a->getName();sourceCount=a->m_numTroops;
                const int direction=e[8];const int pose=direction==6||direction==5||direction==0?cs_attack_ur:direction==1||direction==4?cs_attack_r:cs_attack_dr;
                if(impactReplay){a->m_showAttackFrames=1;a->m_showAttackFrameType=a->m_stdIcon->IsValidSeq(pose)?pose:cs_attack_r;}
                else {a->setupAnimation();a->playSample(army::ATTACK_SAMPLE);a->playAnimation(a->m_stdIcon->IsValidSeq(pose)?pose:cs_attack_r,-1,0);a->playAnimation(cs_wait,1,0);}break;
            }
            case VisualShot:{
                source=a->getName();sourceCount=a->m_numTroops;
                army* target=stack(e[6],e[7]);if(target)a->animateMissile(target);break;
            }
            case VisualDamage:{
                if(a->m_gridIndex<0||a->m_gridIndex>=187)break;
                if(impactReplay){
                    a->m_someUnitsDamaged=1;
                    if(e[7]>0)a->m_numTroopsToShowOverride=a->m_numTroops;
                    a->m_numTroops=e[8];a->m_topCreatureDamage=e[9];a->m_allUnitsKilled=e[8]<=0;
                    hits.push_back({source,sourceCount,a,e[6],e[7]});break;
                }
                a->setupAnimation();a->playSample(e[8]>0?army::WINCE_SAMPLE:army::DIE_SAMPLE);
                a->playAnimation(e[8]>0?cs_wince:cs_death,-1,0);
                a->m_numTroops=e[8];a->m_topCreatureDamage=e[9];a->m_numTroopsToShowOverride=-1;
                if(e[8]>0)a->playAnimation(cs_wait,1,0);else {c.removeArmyFromGrid(*a);a->m_monInfo.attributes|=creatureImmobilized;}
                if(!source.empty())c.damageMessage(source.c_str(),sourceCount,e[6],a,e[7]);
                c.drawFrame(1,0,0,0,1,0);break;
            }
            case VisualSpell:{
                if(e[6]<0||e[6]>=81)break;
                const auto spell=static_cast<SpellID>(e[6]);c.m_currentSide=e[10];
                source=akSpellTraits[spell].m_name;sourceCount=1;
                army* target=e[7]>=0&&e[7]<187?c.m_cells[e[7]].getArmy():nullptr;
                c.showSpellMessage(e[8],spell,target);
                finishCast();int castX=a->midX(),castY=a->midY();
                hero* caster=c.m_heroes[c.m_currentSide];
                if(caster&&(e[8]==0||e[8]==combatManager::SPELL_CASTER_ARTIFACT)){
                    const auto& sprite=g_combatHeroSprites[akHeroClassTraits[caster->m_heroClass].m_townType][akHeroTraits[caster->m_id].m_sex];
                    castingSide=c.m_currentSide;castX=castingSide?c.cmbtHero[1]->GetWidth()-sprite.m_castX+693:sprite.m_castX-43;castY=sprite.m_castY-19;
                    c.cmbtHeroFrameType[castingSide]=4;for(int frame=0;frame<sprite.m_castFrame;++frame){c.cmbtHeroFrameIndex[castingSide]=frame;c.drawFrame(1,0,0,100,1,1);}
                }
                bool resolvedEffect=false;for(const auto& next:snapshot.events){if(next[0]<=e[0])continue;if(next[1]==VisualSpell||next[1]==VisualAction)break;if(next[1]==VisualEffect||(impactReplay&&next[1]==VisualImpact&&next[6]>=0))resolvedEffect=true;}
                if(target && spell==SPELL_MAGIC_ARROW)c.animateMagicArrow(castX,castY,target);
                if(resolvedEffect)break;
                if(target){
                    const int effect=spell==SPELL_LIGHTNING_BOLT||spell==SPELL_TITANS_LIGHTNING_BOLT?1:akSpellTraits[spell].m_effect;
                    c.spellEffect(effect,target,spell==SPELL_LIGHTNING_BOLT?10:80,false);
                }else if(e[7]>=0&&e[7]<187)c.spellEffect(akSpellTraits[spell].m_effect,e[7],80,false);
                break;
            }
            case VisualMorale:
                finishCast();finishAction();Port::showMorale(a,e[6]!=0);break;
            case VisualLuck:
                Port::showLuck(a);break;
            case VisualImpact:
                for(int side=0;side<2;++side)for(int slot=0;slot<c.m_numArmies[side];++slot)
                    c.m_armies[side][slot].m_showPowEffect=(e[8+side]&(1<<slot))!=0;
                Port::feedbackMarker("impact-start",a);
                c.powEffect(static_cast<TSpellEffectID>(e[6]),e[7]);
                for(const auto& hit:hits)if(!hit.source.empty())c.damageMessage(hit.source.c_str(),hit.count,hit.damage,hit.target,hit.killed);
                hits.clear();
                // The original waits for the shoot sample after completing
                // both recovery and target reaction, before the next volley.
                if(action==7 && a->m_creatureType!=CREATURE_MAGOG && a->m_creatureType!=CREATURE_LICH && a->m_creatureType!=CREATURE_POWER_LICH)a->waitSample(army::SHOOT_SAMPLE);
                Port::feedbackMarker("impact-end",a);break;
            case VisualSpellSound:
                if(e[6]>=0&&e[6]<81)launchSample(akSpellTraits[e[6]].m_sample,-1,3);
                break;
            case VisualEffect:
                if(e[10]==0)c.spellEffect(e[6],a,e[8],e[9]!=0);
                else if(e[10]==1&&e[7]>=0&&e[7]<187)c.spellEffect(e[6],e[7],e[8],e[9]!=0);
                else if(e[10]==2){bool effected[2][20]={};for(int side=0;side<2;++side)for(int slot=0;slot<20;++slot)effected[side][slot]=(e[7+side]&(1<<slot))!=0;c.showMassSpell(effected,e[6],e[9]!=0);}
                break;
            case VisualAction:{
                finishCast();finishAction();action=e[6];c.m_currentSide=e[10];c.m_combatWindow->clearCombatMessages();
                if(action==1||action==2||action==6||action==7||action==9||action==10)c.resetCyclingCreatures();
                if(e[6]==3||e[6]==8){
                    const int text=e[6]==8?(a->m_numTroops==1?GENERAL_TEXT_COMBAT_WAIT_ONE_FORMAT:GENERAL_TEXT_COMBAT_WAIT_MANY_FORMAT):(a->m_numTroops==1?GENERAL_TEXT_COMBAT_DEFEND_ONE_FORMAT:GENERAL_TEXT_COMBAT_DEFEND_MANY_FORMAT);
                    const auto message=formatString(g_generalText->GetText(text),a->getName(),std::max(a->m_monInfo.defenseSkill*20/100,1));
                    c.m_combatWindow->combatMessage(message.c_str(),1,0);
                    c.drawFrame(1,0,0,0,1,0);
                }
                break;
            }
            }
        }
        finishCast();finishAction();
#endif
    }
    static void settleDeath(army& a){
        auto& c=*g_combatManager;
        if(a.is(creatureImmobilized))return;
        a.m_monInfo.attributes|=creatureImmobilized;a.m_allUnitsKilled=0;
        if(!c.validHex(a.m_gridIndex))return;
        if(a.leavesNoBody()){
            c.bCreatureVanish[a.getOwningSide()][a.index]=1;c.bSomeCreaturesVanish=1;
            return;
        }
        auto& cell=c.m_cells[a.m_gridIndex];
        const int second=a.is(creatureDoubleWide)?a.m_gridIndex+a.offsetToFront(-1):-1;
        if(cell.m_bodiesInHex<14 && (second<0 || (c.validHex(second)&&c.m_cells[second].m_bodiesInHex<14))){
            auto add=[&](hexcell& hex){if(!hex.hasArmy())return;const int i=hex.m_bodiesInHex++;
                hex.m_deadArmySide[i]=hex.m_armySide;hex.m_deadArmySlot[i]=hex.m_armySlot;hex.m_deadPartOfDouble[i]=hex.m_partOfDouble;};
            add(cell);if(second>=0)add(c.m_cells[second]);
        }
        c.m_highlighterOn=0;c.removeArmyFromGrid(a);
        Port::feedbackMarker("corpse-installed",&a);
    }
    static bool results(){
#ifdef __EMSCRIPTEN__
        int length=EM_ASM_INT({return window.homm3Authority.combatResultLength();});if(!length)return false;
        if(length<0||length>512)throw std::runtime_error("Combat result limit");
        std::vector<int> words(length);EM_ASM({window.homm3Authority.copyCombatResult(HEAP32,$0>>2);},words.data());
        size_t index=0;auto next=[&](int lo,int hi){if(index>=words.size()||words[index]<lo||words[index]>hi)throw std::runtime_error("Invalid combat result");return words[index++];};
        const int winner=next(0,1),mine=next(0,1),siege=next(0,1),experience=next(0,INT32_MAX);
        g_combatRetreated=next(0,1);g_combatSurrendered=next(0,1);
        auto& c=*g_combatManager;c.m_winner=winner;
        for(int side=0;side<2;++side){int size=next(0,63);std::string name;for(int i=0;i<size;++i)name.push_back(char(next(0,255)));if(c.m_heroes[side]){auto& target=c.m_heroes[side]->m_name;const size_t count=std::min(name.size(),sizeof(target)-1);std::memcpy(target,name.data(),count);target[count]=0;}}
        std::vector<type_artifact> loot;int count=next(0,128);for(int i=0;i<count;++i){int id=next(0,200),extra=next(INT32_MIN,INT32_MAX);type_artifact item(static_cast<TArtifact>(id));item.m_extra=extra;loot.push_back(item);}
        for(auto& spells:c.m_eagleEyeData)spells.clear();count=next(0,81);for(int i=0;i<count;++i)c.m_eagleEyeData[mine].insert(SpellID(next(0,80)));
        int raisedType=next(-1,200),raisedCount=next(0,INT32_MAX);if(index!=words.size())throw std::runtime_error("Trailing combat result");
        TCombatResultsWindow window(c.m_heroes[0],c.m_heroes[1],mine,winner,siege,experience);window.doModal();
        if(mine==winner){
            c.showEagleEye(mine,0);c.showLootedArtifacts(loot,0);
            if(raisedType>=0&&raisedCount>0){
                if(raisedCount==1)std::sprintf(g_text,g_generalText->GetText(GENERAL_TEXT_NECROMANCY_RAISE_ONE_FORMAT),getArmyName(raisedType,1));
                else std::sprintf(g_text,g_generalText->GetText(GENERAL_TEXT_NECROMANCY_RAISE_MANY_FORMAT),raisedCount,getArmyName(raisedType,2));
                normalDialog(g_text,1,-1,-1,0x15,(uint32_t(uint16_t(raisedCount))<<16)|uint16_t(raisedType),-1,0,-1,0,-1,0);
            }
        }
        EM_ASM({window.homm3Authority.combatResultShown();});return true;
#else
        return false;
#endif
    }
    static int main(message& msg){
#ifdef __EMSCRIPTEN__
        auto& c=*g_combatManager;
        c.doAnimations();
        if(EM_ASM_INT({return window.homm3Authority.combatFinished();})){msg.m_id=MESSAGE_EXECUTIVE;msg.m_codeX=EXECUTIVE_COMMAND_TERMINATE_LOOP;return MESSAGE_DISPATCH_FORWARD;}
        if(take()){replay();project();c.drawFrame(1,0,0,0,1,0);g_windowManager->updateScreen();}
        if(results()){msg.m_id=MESSAGE_EXECUTIVE;msg.m_codeX=EXECUTIVE_COMMAND_TERMINATE_LOOP;return MESSAGE_DISPATCH_FORWARD;}
        const bool canAct=EM_ASM_INT({return window.homm3Authority.combatCanAct();});
        if(c.m_thisNetHasControl&&!canAct)clearInteraction();
        c.m_thisNetHasControl=canAct;controls(true);
        if(msg.m_id==MESSAGE_WIDGET&&msg.m_codeX==widget::WIDGET_DESELECT&&msg.m_codeY==TCombatWindow::COMBAT_LEFT_COMMAND_3_ID){EM_ASM({window.homm3Authority.combatToggleAuto();});return MESSAGE_DISPATCH_CONSUME;}
        if(!canAct){c.processCombatMsg(msg);return MESSAGE_DISPATCH_CONSUME;}
        c.m_nextAction=0;c.m_nextActionExtra=-1;c.m_nextActionGridIndex=-1;c.m_nextActionGridIndex2=-1;
        // End-tactics has an immediate simulation branch in the original
        // window. Translate it before the original dispatcher sees it.
        if(msg.m_id==MESSAGE_WIDGET&&msg.m_codeX==widget::WIDGET_DESELECT&&msg.m_codeY==TCombatWindow::COMBAT_PLACEMENT_COMMAND_1_ID){
            EM_ASM({window.homm3Authority.combatAction(100,-1,-1,-1);});return MESSAGE_DISPATCH_CONSUME;
        }
        if(c.m_autoCombatOn&&EM_ASM_INT({return window.homm3Authority.combatOffers(103);})){EM_ASM({window.homm3Authority.combatAutoStep();});return MESSAGE_DISPATCH_CONSUME;}
        c.processCombatMsg(msg);
        if(c.m_nextAction){
            EM_ASM({window.homm3Authority.combatAction($0,$1,$2,$3);},c.m_nextAction,c.m_nextActionGridIndex,c.m_nextActionExtra,c.m_nextActionGridIndex2);
            c.m_nextAction=0;
            if(!EM_ASM_INT({return window.homm3Authority.combatCanAct();}))clearInteraction();
        }
#endif
        return MESSAGE_DISPATCH_CONSUME;
    }
};
namespace Port { namespace BattleClient {
bool active(){return running;}
void settleDeath(army& stack){AuthorityBattleAccess::settleDeath(stack);}
void project(){if(running)AuthorityBattleAccess::project();}
void installOpening(){if(running)AuthorityBattleAccess::project(false);}
int main(message& msg){try{return AuthorityBattleAccess::main(msg);}catch(const std::exception& e){
#ifdef __EMSCRIPTEN__
    EM_ASM({window.homm3Authority.fail(UTF8ToString($0));},e.what());
#endif
    msg.m_id=MESSAGE_EXECUTIVE;msg.m_codeX=EXECUTIVE_COMMAND_TERMINATE_LOOP;return MESSAGE_DISPATCH_FORWARD;
}}
bool begin(){
    if(running)return false;
    try{
        if(!take())return false;
        // Cursor redraw pumps events: claim the modal before that can reenter
        // begin() and consume another opening/observation for this battle.
        running=true;AuthorityClient::finishMovementPresentation();ScopedPresentationPointer pointer;auto& s=snapshot;
        visualSerial=s.events.empty()?0:s.events.back()[0];
#ifdef __EMSCRIPTEN__
        EM_ASM({window.homm3Authority.rendererReady(false);});
#endif
        for(auto& g:groups)g.initialize();
        for(const auto& a:s.stacks)if(a.slot<7){groups[a.side].m_armies[a.slot]=a.creature;groups[a.side].m_numTroops[a.slot]=a.count;}
        hero* heroes[2]={nullptr,nullptr};for(int side=0;side<2;++side)if(s.heroId[side]>=0){heroes[side]=&g_game->m_heroes[s.heroId[side]];heroes[side]->m_owner=s.players[side];}
        town* t=s.town>=0?&g_game->m_towns[s.town]:nullptr;
        // Match the original adventure-to-combat boundary: leave a static
        // map hero behind, not a live adventure cursor under the battle.
        g_advManager->demobilizeCurrHero(false,false);
        AuthorityBattleAccess::heroes();
        g_combatManager->setupCombat(type_point(s.x,s.y,s.z),heroes[0],&groups[0],s.players[1],t,heroes[1],&groups[1],s.x,s.y,0,s.surrounded);
        AuthorityBattleAccess::configure();g_executive->doDialog(g_combatManager);running=false;
#ifdef __EMSCRIPTEN__
        EM_ASM({window.homm3Authority.combatWindowClosed();});
#endif
        return true;
    }catch(const std::exception& e){running=false;
#ifdef __EMSCRIPTEN__
        EM_ASM({window.homm3Authority.fail(UTF8ToString($0));},e.what());
#endif
        return false;
    }
}
} }
