#include "authority_remote_move.h"
#include "game.h"
#include "hero.h"
#include "advmgr.h"
#include "findpath.h"
#include "prefs.h"
#include "remote.h"
#include "parity_harness.h"
#include <vector>
#include <cstdlib>

namespace Port { namespace AuthorityRemoteMove {
namespace {
struct Position {int id,owner;type_point point;};
std::vector<Position> previous;

// Rendering borrows the original current-player/cursor context. Restore every
// borrowed field and the canonical destination cell even if rendering throws.
struct ViewScope {
    advManager& view=*g_advManager;
    hero& moving;
    NewmapCell& cell;
    NewmapCell savedCell;
    type_point destination;
    playerData* player=g_currentPlayer;
    int netPlayer=g_netLocalGamePos,remote=g_remoteOn;
    unsigned char playerBit=g_curPlayerBit;
    int selected;
    int cursorType=view.m_cursorType,direction=view.m_cursorDirection;
    int sequence=view.m_cursorSequence,frame=view.m_cursorFrameCount;
    unsigned char mobile=view.m_curHeroMobile,cursor=view.m_drawCursor;
    ViewScope(hero& h):moving(h),cell(*g_game->getCell(h.getLocation())),savedCell(cell),destination(h.getLocation()),selected(g_game->m_players[h.m_owner].m_currHeroId){
        g_currentPlayer=&g_game->m_players[h.m_owner];g_currentPlayer->m_currHeroId=h.m_id;
        g_netLocalGamePos=h.m_owner;g_curPlayerBit=1<<h.m_owner;g_remoteOn=1;
    }
    ~ViewScope(){
        moving.m_x=destination.m_x;moving.m_y=destination.m_y;moving.m_z=destination.m_z;
        cell=savedCell;g_currentPlayer->m_currHeroId=selected;
        g_currentPlayer=player;g_netLocalGamePos=netPlayer;g_curPlayerBit=playerBit;g_remoteOn=remote;
        view.m_cursorType=cursorType;view.m_cursorDirection=direction;
        view.m_cursorSequence=sequence;view.m_cursorFrameCount=frame;
        view.m_curHeroMobile=mobile;view.m_drawCursor=cursor;
    }
};
}
void capture(int viewer){
    previous.clear();
    for(const auto& h:g_game->m_heroes)
        if(h.m_owner>=0&&h.m_owner!=viewer&&h.isOnMap())
            previous.push_back({int(h.m_id),int(h.m_owner),h.getLocation()});
}
void replay(int viewer){
    auto positions=std::move(previous);previous.clear();
    for(const auto& before:positions){
        auto& h=g_game->m_heroes[before.id];const auto destination=h.getLocation();
        if(h.m_owner!=before.owner||h.m_owner==viewer||!h.isOnMap()||!destination.isValid())continue;
        const int dx=int(destination.m_x)-before.point.m_x,dy=int(destination.m_y)-before.point.m_y;
        if((!dx&&!dy)||std::abs(dx)>1||std::abs(dy)>1||destination.m_z!=before.point.m_z)continue;
        auto* cell=g_game->getCell(destination);
        // A projection only authorizes known visible positions. Never invent
        // a teleport path or a boat appearance from an incomplete observation.
        if(cell->m_type!=HERO||cell->m_extraInfo!=h.m_id||cell->m_groundSet==8)continue;
        int direction=0;while(direction<8&&(g_normalDirTable[direction].m_x!=dx||g_normalDirTable[direction].m_y!=dy))++direction;
        if(direction==8)continue;
        ViewScope scope(h);
        h.m_x=before.point.m_x;h.m_y=before.point.m_y;
        if(!g_advManager->getMoveShowIt(&h,direction))continue;
        cell->m_type=NOTHING;cell->m_extraInfo=0;cell->m_isTrigger=0;
        auto& view=*g_advManager;view.m_curHeroMobile=1;view.m_drawCursor=1;
        view.m_cursorType=34;view.m_cursorDirection=direction;
        view.m_cursorSequence=h.getStandSequence();view.m_cursorFrameCount=0;
#ifdef HOMM3_HARNESS
        Harness::decision("remote-move-start",h.m_owner,h.m_id);
#endif
        view.turnTo(direction);view.animateMove(&h,direction,dx,dy);view.stopCursor(true);
#ifdef HOMM3_HARNESS
        Harness::decision("remote-move-end",h.m_owner,h.m_id);
#endif
    }
}
} }
