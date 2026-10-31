#include "authority_actions_client.h"
#include "textresource.h"
#include "authority_exchange_client.h"
#include "authority_client.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "advmgr.h"
#include "kb.h"
#include "winmgr.h"
#include "mousemgr.h"
#include "dimensiondoorwindow.h"
#include "towngatewindow.h"
#include "swapmgr.h"
#include "exec.h"
#include <climits>
namespace Port { namespace AuthorityActionsClient {
namespace {
int draggedHero=-1,draggedSlot=-1;
struct WorldMarker{int type=-1,index=0,owner=-1;};
std::vector<WorldMarker> worldMarkers;NewmapCell worldCell;int symbolOwner=-1;
}
bool enabled(){return AuthorityClient::enabled();}
bool worldViewActive(){return !worldMarkers.empty();}
NewmapCell* worldViewCell(int x,int y,int z){
    if(!worldViewActive()||x<0||x>=g_mapWidth||y<0||y>=g_mapHeight||z<0||z>=g_game->getNumMapLevels())return nullptr;
    const auto& marker=worldMarkers[(z*g_mapHeight+y)*g_mapWidth+x];if(marker.type<0)return nullptr;
    worldCell.m_type=TAdventureObjectType(marker.type);worldCell.m_objectIndex=marker.index;worldCell.m_isTrigger=true;worldCell.m_extraInfo=0;symbolOwner=marker.owner;return &worldCell;
}
int worldViewOwner(NewmapCell* cell){return worldViewActive()&&cell==&worldCell?symbolOwner:-2;}
void showWorld(int spell,int level,const std::vector<int>& markers){
    if(!enabled()||markers.size()%6)return;
    worldMarkers.assign(g_mapWidth*g_mapHeight*g_game->getNumMapLevels(),WorldMarker());
    for(size_t i=0;i<markers.size();i+=6){const int x=markers[i],y=markers[i+1],z=markers[i+2];if(x<0||x>=g_mapWidth||y<0||y>=g_mapHeight||z<0||z>=g_game->getNumMapLevels())continue;worldMarkers[(z*g_mapHeight+y)*g_mapWidth+x]={markers[i+3],markers[i+4],markers[i+5]};}
    g_advManager->viewWorld(spell,TSkillMastery(level));worldMarkers.clear();
}
void present(const std::string& kind,int id,int other){
    if(!enabled())return;
    if(kind=="open_town"){
        if(id>=0&&id<int(g_game->m_towns.size())&&g_game->m_towns[id].m_owner==g_localGamePos){
            // A selected map cursor temporarily restores the underlying cell
            // and clears its town visitor. Entering a town must put the hero
            // back in that cell, just like the original townEvent path.
            g_advManager->demobilizeCurrHero(false,false);
            g_game->m_towns[id].view(0);
        }
    }else if(kind=="hero_exchange"){
        if(id<0||id>=game::HERO_COUNT||other<0||other>=game::HERO_COUNT)return;
        auto& left=g_game->m_heroes[id];auto& right=g_game->m_heroes[other];
        if(left.m_owner!=g_localGamePos||right.m_owner!=g_localGamePos)return;
        swapManager manager(&left,&right);g_executive->doDialog(&manager);g_advManager->redrawAdvScreen(1,0);
    }
}
int armyId(armyGroup* army){
    for(int i=0;i<game::HERO_COUNT;++i)if(g_game->m_heroes[i].m_owner==g_localGamePos&&&g_game->m_heroes[i].m_army==army)return i;
    for(auto& t:g_game->m_towns)if(t.m_owner==g_localGamePos&&&t.getArmy()==army)return -int(t.m_id)-1;
    return INT_MIN;
}
bool transfer(armyGroup* source,int src,armyGroup* target,int dst,int count,int mode){
    if(AuthorityExchangeClient::active())return AuthorityExchangeClient::transfer(source,src,target,dst,count,mode);
    if(AuthorityClient::armyExchangeActive())return AuthorityClient::chooseArmyTransfer(source,src,target,dst,count,mode);
    int from=armyId(source),to=armyId(target);if(from==INT_MIN||to==INT_MIN)return false;
    return AuthorityClient::request(16,from,src,to,dst,count,mode);
}
void split(armyGroup* source,int src,armyGroup* target,int dst,int destinationCount){
    int change=destinationCount-target->m_numTroops[dst];
    if(change>0)transfer(source,src,target,dst,change,0);
    else if(change<0)transfer(target,dst,source,src,-change,0);
}
void cancelArtifactDrag(){draggedHero=draggedSlot=-1;g_mouseManager->setPointer(0,mouseManager::DEFAULT_SET);}
void artifactClick(int heroId,int slot,bool rightMouse){
    if(heroId<0||heroId>=game::HERO_COUNT)return;auto& h=g_game->m_heroes[heroId];
    if(h.m_owner!=g_localGamePos)return;
    const auto item=slot>=100?h.getBackpack(slot-100):h.getArtifact(TArtifactSlot(slot));
    if(rightMouse){
        if(draggedHero>=0){cancelArtifactDrag();return;}
        if(item.m_artifactId<0)return;
        if(slot<100&&akArtifactTraits[item.m_artifactId].m_comboType>=0){
            if(h.showDisassembleArtifactDialog(item.m_artifactId)==DIALOG_RETURN_ACCEPT)AuthorityClient::request(24,heroId,slot,1);
        }else if(slot<100&&akArtifactTraits[item.m_artifactId].m_targetCombo>=0&&h.hasCombinationArtifactComponents(akArtifactTraits[item.m_artifactId].m_targetCombo)){
            if(h.showAssembleArtifactDialog(item.m_artifactId)==DIALOG_RETURN_ACCEPT)AuthorityClient::request(24,heroId,slot,0);
        }else h.viewArtifact(item,true);
        return;
    }
    if(draggedHero<0){
        if(item.m_artifactId<0||item.m_artifactId==ARTIFACT_SPELLBOOK||item.m_artifactId==ARTIFACT_CATAPULT)return;
        draggedHero=heroId;draggedSlot=slot;g_mouseManager->setPointer(item.m_artifactId,mouseManager::ARTIFACT_SET);return;
    }
    const int source=draggedHero,from=draggedSlot;cancelArtifactDrag();
    if(source!=heroId||from!=slot)AuthorityClient::request(18,source,from,heroId,slot);
}
bool castSpell(int spell){
    hero* h=g_game->getCurrHero();if(!h)return false;int x=0,y=0,z=0;
    if(spell==SPELL_DIMENSION_DOOR||spell==SPELL_SCUTTLE_BOAT){
        if(spell==SPELL_DIMENSION_DOOR){TDimensionDoorWindow window;window.doModal(0);}
        else {TSkuttleBoatWindow window;window.doModal(0);}
        if(g_windowManager->m_dialogReturn!=1){
            normalDialog(g_generalText->GetText(GENERAL_TEXT_ADVENTURE_SPELL_NO_TARGET),1,-1,-1,-1,0,-1,0,-1,0,-1,0);
            return false;
        }
        auto p=g_advManager->get_mouse_map_point();x=p.m_x;y=p.m_y;z=p.m_z;
    }
    else if(spell==SPELL_TOWN_PORTAL){
        x=-1;
        if(h->getSpellLevel(SPELL_TOWN_PORTAL)>=eMasteryAdvanced){TTownGateWindow window(1);for(size_t i=0;i<g_game->m_towns.size();++i){const auto& t=g_game->m_towns[i];if(g_game->onSameTeam(h->m_owner,t.m_owner)&&t.m_visitingHeroId<0)window.addTown(i);}window.doModal();x=g_windowManager->m_dialogReturn;}
        else {int distance=INT_MAX;for(size_t i=0;i<g_game->m_towns.size();++i){const auto& t=g_game->m_towns[i];if(g_game->onSameTeam(h->m_owner,t.m_owner)){int d=h->getLocation().distanceSquared(t.getLocation());if(d<distance){distance=d;x=i;}}}}
        if(x<0)return false;
    }
    return AuthorityClient::request(15,h->m_id,spell,x,y,z);
}
} }
