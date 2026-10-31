#include "parity_wait.h"
#include "presentation_pointer.h"
#include "authority_puzzle_client.h"
#include "authority_client.h"
#include "authority_exchange_client.h"
#include "battle_client.h"
#include "authority_actions_client.h"
#include "authority_quest_client.h"
#include "recruit.h"
#include "university_window.h"
#include "exec.h"
#include "widget.h"
#include "server_projection.h"
#include "fast_game.h"
#include "game.h"
#include "hero.h"
#include "findpath.h"
#include "town.h"
#include "kb.h"
#include "advmgr.h"
#include "adventuremapwindow.h"
#include "inputmgr.h"
#include "kbwin.h"
#include "winmgr.h"
#include "levelupwindow.h"
#include "hd_view.h"
#include "remote.h"
#include "resourcemanager.h"
#include "remotedlg.h"
#include "bottomviewsubwindow.h"
#include "text.h"
#include "font.h"
#include "mousemgr.h"
#include "soundmgr.h"
void popupPlayerTurnInfo();
#include <vector>
#include <stdexcept>
#include <type_traits>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
struct AuthorityTurnViewAccess {
    static type_bottom_view_window* panel(TAdventureMapWindow* win){return win->m_bottomView;}
    static bool enemy(bool force){return g_advManager->updBottomViewEnemyTurn(force);}
    static void animate(TAdventureMapWindow* win,bool background){if((!background||win->m_animateInBackground)&&win->m_bottomView)win->m_bottomView->animate();}
};
namespace {
bool active=false,applying=false,failed=false,choosing=false,viewInstalled=false;
int actor=-1,turn=-1;
int shownTurn=-1;bool pendingTurnPopup=false;
int transferStage=0;
int choiceMode=0,choiceValue=-1,choiceQuantity=0;
bool movementStandEnd=true,movementRequestActive=false;
int startupSetup[41]={};bool capturedSetup=false;
std::vector<std::int32_t> pending;
struct Reader {
    const std::int32_t* values;size_t count,pos=0;bool apply;
    int get(){if(pos==count)throw std::runtime_error("Truncated projection");return values[pos++];}
    int bounded(int lo,int hi){int v=get();if(v<lo||v>hi)throw std::runtime_error("Projection word "+std::to_string(pos-1)+": "+std::to_string(v)+" outside "+std::to_string(lo)+".."+std::to_string(hi));return v;}
    template<class T> void set(T& v){auto x=get();if(apply)v=static_cast<T>(x);}
    template<class T> void array(T& v){for(auto& x:v)set(x);}
    void text(std::string& value){int n=bounded(0,65536);std::string result;result.reserve(n);for(int i=0;i<n;++i)result.push_back(char(bounded(0,255)));if(apply)value=std::move(result);}
    void army(armyGroup& a){array(a.m_armies);array(a.m_numTroops);}
    uint64_t maskValue(){uint64_t lo=uint32_t(get()),hi=uint32_t(get());return lo|(hi<<32);}
    void mask(long long& v){uint64_t bits=maskValue();if(apply)v=static_cast<long long>(bits);}
    void artifacts(type_artifact* a,int n){for(int i=0;i<n;++i){set(a[i].m_artifactId);set(a[i].m_extra);}}
};
}
bool AuthorityProjectionAccess::read(const std::int32_t* words,size_t length,bool apply){
    try {
        Reader r{words,length,0,apply};if(r.get()!=Port::Authority::ProjectionSchema)return false;
        const int local=r.bounded(0,7),current=r.bounded(-1,7);if(actor>=0&&actor!=local)return false;
        r.set(g_game->m_day);r.set(g_game->m_week);r.set(g_game->m_month);
        if(r.get()!=g_mapWidth||r.get()!=g_game->getNumMapLevels())return false;
        auto& player=g_game->m_players[local];r.array(player.m_resources);r.array(player.m_recruits);r.set(player.m_mysticalGardenFlags);r.set(player.m_magicSpringFlags);r.set(player.m_deadGuyFlags);r.set(player.m_leanToFlags);
        std::vector<int> fieldHeroes,townRoster;
        int rosterCount=r.bounded(0,playerData::HERO_SLOT_COUNT);
        for(int i=0;i<rosterCount;++i){int id=r.bounded(0,game::HERO_COUNT-1);if(std::find(fieldHeroes.begin(),fieldHeroes.end(),id)!=fieldHeroes.end())return false;fieldHeroes.push_back(id);}
        rosterCount=r.bounded(0,72);
        for(int i=0;i<rosterCount;++i){int id=r.bounded(0,int(g_game->m_towns.size())-1);if(std::find(townRoster.begin(),townRoster.end(),id)!=townRoster.end())return false;townRoster.push_back(id);}
        // Fixed hero and town storage keeps original modal pointers valid. The
        // whole packet is parsed once before any field is changed.
        if(apply){
            actor=local;turn=current;
            for(auto& p:g_game->m_players){p.m_numHeroes=0;p.m_numTowns=0;p.m_isLocal=0;std::fill_n(p.m_resources,7,0);std::fill_n(p.m_heroes,8,static_cast<HeroId>(-1));std::fill_n(p.m_townIds,72,-1);}
            // Restore private resources after sanitizing every other player.
            for(int i=0;i<7;++i)player.m_resources[i]=words[8+i];
            for(auto& h:g_game->m_heroes){h.m_valid=false;h.m_owner=-1;h.m_x=-1;h.m_y=-1;h.m_z=-1;h.m_army.initialize();h.m_mana=0;h.m_movePoints=0;h.m_maxMovePoints=0;h.m_experience=0;h.m_level=0;h.m_formation=0;h.m_flags=0;h.m_skillCount=0;
                h.m_trainingGroundsFlags=0;h.m_defenseTowerFlags=0;h.m_gardenOfRevelationFlags=0;h.m_mercCampFlags=0;h.m_powerSchoolFlags=0;h.m_treeOfKnowledgeFlags=0;h.m_libraryFlags=0;h.m_arenaFlags=0;h.m_magicSchoolFlags=0;h.m_warSchoolFlags=0;h.m_universityFlags=0;h.m_shrine1Flags=0;h.m_shrine2Flags=0;h.m_shrine3Flags=0;h.m_dWalkSpellsCast=0;h.m_townSpecialGrantedMask.reset();
                h.m_flightLevel=static_cast<TSkillMastery>(0);h.m_waterWalkLevel=static_cast<TSkillMastery>(0);h.m_disguiseLevel=static_cast<TSkillMastery>(0);h.m_identifyLevel=static_cast<TSkillMastery>(0);h.m_moraleBonus=0;h.m_luckBonus=0;h.m_isSleeping=0;
                std::fill_n(h.m_skillOrder,28,0);std::fill_n(h.m_artifactSlotCounts,15,0);
                std::fill_n(h.m_stats,4,0);std::fill_n(h.m_skillLevel,28,0);std::fill_n(h.m_inSpellbook,sizeof(h.m_inSpellbook),0);std::fill_n(h.m_availableSpells,sizeof(h.m_availableSpells),0);
                for(auto& a:h.m_equipped){a.m_artifactId=ARTIFACT_NONE;a.m_extra=-1;}for(auto& a:h.m_backpack){a.m_artifactId=ARTIFACT_NONE;a.m_extra=-1;}h.m_backpackCount=0;}
            for(auto& t:g_game->m_towns){t.m_owner=-1;t.m_garrison.initialize();std::fill_n(t.m_population,14,0);t.m_visitingHeroId=-1;t.m_garrisonHeroId=-1;t.m_populationMask=0;t.m_fullBuildingMask=0;t.m_legalBuildings=0;}
        }
        int heroes=r.bounded(0,game::HERO_COUNT);std::vector<bool> seenHeroes(game::HERO_COUNT,false),privateHeroes(game::HERO_COUNT,false);
        for(int i=0;i<heroes;++i){int id=r.bounded(0,game::HERO_COUNT-1);if(seenHeroes[id])return false;seenHeroes[id]=true;int own=r.bounded(0,2);auto& h=g_game->m_heroes[id];
            r.set(h.m_x);r.set(h.m_y);r.set(h.m_z);r.set(h.m_valid);
            int owner=r.bounded(-1,7);if(own==1&&owner!=local)return false;privateHeroes[id]=own==1;
            if(apply)h.m_owner=owner;r.set(h.m_facing);r.set(h.m_portrait);
            if(apply){if(!viewInstalled||owner!=local){h.m_pathTargetX=-1;h.m_pathTargetY=-1;h.m_pathTargetZ=-1;}h.m_obscuredLocation=type_point(h.m_x,h.m_y,h.m_z);}
            if(!own)continue;
            r.set(h.m_mana);r.set(h.m_maxMovePoints);r.set(h.m_movePoints);r.set(h.m_experience);r.set(h.m_level);r.set(h.m_formation);r.set(h.m_flags);
            {auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_trainingGroundsFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_defenseTowerFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_gardenOfRevelationFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_mercCampFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_powerSchoolFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_treeOfKnowledgeFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_libraryFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_arenaFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_magicSchoolFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_warSchoolFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_universityFlags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_shrine1Flags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_shrine2Flags=bits;}{auto bits=static_cast<std::uint32_t>(r.get());if(apply)h.m_shrine3Flags=bits;}
            r.set(h.m_dWalkSpellsCast);{auto bits=r.maskValue();if(bits>>48)return false;if(apply)h.m_townSpecialGrantedMask=std::bitset<48>(bits);}
            r.array(h.m_stats);r.array(h.m_skillLevel);r.array(h.m_skillOrder);r.set(h.m_skillCount);r.army(h.m_army);
            r.artifacts(h.m_equipped,sizeof(h.m_equipped)/sizeof(h.m_equipped[0]));r.artifacts(h.m_backpack,sizeof(h.m_backpack)/sizeof(h.m_backpack[0]));r.set(h.m_backpackCount);r.array(h.m_inSpellbook);r.array(h.m_availableSpells);
            r.set(h.m_heroClass);r.set(h.m_sex);r.set(h.m_hasCustomName);r.text(h.m_customName);r.array(h.m_name);
            r.set(h.m_flightLevel);r.set(h.m_waterWalkLevel);r.set(h.m_disguiseLevel);r.set(h.m_identifyLevel);r.set(h.m_moraleBonus);r.set(h.m_luckBonus);r.set(h.m_isSleeping);r.array(h.m_artifactSlotCounts);
            int ox=r.get(),oy=r.get(),oz=r.get();if(apply)h.m_obscuredLocation=type_point(ox,oy,oz);r.set(h.m_obscuredType);r.set(h.m_wasTrigger);r.set(h.m_extraInfo);
        }
        for(int id:fieldHeroes){if(!privateHeroes[id])return false;if(apply)player.m_heroes[player.m_numHeroes++]=static_cast<HeroId>(id);}
        int towns=r.bounded(0,int(g_game->m_towns.size()));std::vector<bool> seenTowns(g_game->m_towns.size(),false),privateTowns(g_game->m_towns.size(),false);
        for(int i=0;i<towns;++i){int id=r.bounded(0,int(g_game->m_towns.size())-1);if(seenTowns[id])return false;seenTowns[id]=true;bool own=r.bounded(0,1);privateTowns[id]=own;auto& t=g_game->m_towns[id];
            r.set(t.m_owner);r.set(t.m_mapX);r.set(t.m_mapY);r.set(t.m_mapZ);r.set(t.m_type);if(!own)continue;
            r.set(t.m_builtThisTurn);r.set(t.m_mageLevel);r.set(t.m_garrisonHeroId);r.set(t.m_visitingHeroId);r.array(t.m_population);r.army(t.m_garrison);
            r.mask(t.m_populationMask);r.mask(t.m_fullBuildingMask);r.mask(t.m_legalBuildings);for(auto& row:t.m_mageGuildSpells)r.array(row);r.array(t.m_mageGuildSpellCounts);
            r.text(t.m_name);r.array(t.m_generatorBonus);r.set(t.m_manaVortexFull);r.set(t.m_pondAmount);r.set(t.m_pondResource);r.set(t.m_summoningType);r.set(t.m_summoningPopulation);
        }
        for(int id:townRoster){if(!privateTowns[id])return false;if(apply)player.m_townIds[player.m_numTowns++]=id;}
        int total=r.bounded(0,65535),known=r.bounded(0,total);
        if(apply){g_game->m_mines.resize(total);for(auto& m:g_game->m_mines){m.m_playerOwner=-1;m.m_mapX=255;m.m_mapY=255;m.m_mapZ=255;m.m_guards.initialize();}}
        for(int i=0;i<known;++i){int id=r.bounded(0,total-1);mine scratch;auto& m=apply?g_game->m_mines[id]:scratch;r.set(m.m_playerOwner);r.set(m.m_type);r.set(m.m_isAbandoned);r.set(m.m_mapX);r.set(m.m_mapY);r.set(m.m_mapZ);r.army(m.m_guards);}
        total=r.bounded(0,65535);known=r.bounded(0,total);
        if(apply){g_game->m_generators.resize(total);for(auto& d:g_game->m_generators){d.m_playerOwner=-1;d.m_mapX=255;d.m_mapY=255;d.m_mapZ=255;d.m_guards.initialize();std::fill_n(d.m_population,4,0);}}
        for(int i=0;i<known;++i){int id=r.bounded(0,total-1);generator scratch;auto& d=apply?g_game->m_generators[id]:scratch;r.set(d.m_playerOwner);r.set(d.m_genClass);r.set(d.m_genType);r.set(d.m_mapX);r.set(d.m_mapY);r.set(d.m_mapZ);r.set(d.m_townId);r.array(d.m_type);r.array(d.m_population);r.army(d.m_guards);}
        total=r.bounded(0,65535);known=r.bounded(0,total);
        if(apply){g_game->m_garrisons.resize(total);for(auto& g:g_game->m_garrisons){g.m_playerOwner=-1;g.m_mapX=255;g.m_mapY=255;g.m_mapZ=255;g.m_garrisonArmy.initialize();}}
        for(int i=0;i<known;++i){int id=r.bounded(0,total-1);garrison scratch;auto& g=apply?g_game->m_garrisons[id]:scratch;r.set(g.m_playerOwner);r.set(g.m_removableTroops);r.set(g.m_mapX);r.set(g.m_mapY);r.set(g.m_mapZ);r.army(g.m_garrisonArmy);}
        total=r.bounded(0,65535);known=r.bounded(0,total);
        if(apply){g_game->m_boats.resize(total);for(auto& b:g_game->m_boats){b.m_allocated=0;b.m_playerOwner=-1;b.m_x=-1;b.m_y=-1;b.m_z=-1;b.m_valid=false;}}
        for(int i=0;i<known;++i){int id=r.bounded(0,total-1);boat scratch;auto& b=apply?g_game->m_boats[id]:scratch;r.set(b.m_allocated);r.set(b.m_type);r.set(b.m_facing);r.set(b.m_playerOwner);r.set(b.m_occupyingHero);r.set(b.m_occupied);r.set(b.m_x);r.set(b.m_y);r.set(b.m_z);r.set(b.m_valid);
            int x=r.get(),y=r.get(),z=r.get();if(apply)b.m_obscuredLocation=type_point(x,y,z);r.set(b.m_obscuredType);r.set(b.m_wasTrigger);r.set(b.m_extraInfo);}
        auto& map=g_game->m_worldMap;int typeCount=r.bounded(1,65535),visibleTypes=r.bounded(0,typeCount);
        if(map.m_objectTypes.empty()||map.m_sprites.empty())return false;
        if(apply){
            const auto fallback=map.m_objectTypes.front();
            while(map.m_sprites.size()>size_t(typeCount)){ResourceManager::Dispose(map.m_sprites.back());map.m_sprites.pop_back();}
            map.m_objectTypes.resize(typeCount,fallback);
            while(map.m_sprites.size()<size_t(typeCount))map.m_sprites.push_back(ResourceManager::GetSprite(fallback.m_imageName.c_str()));
        }
        std::vector<bool> seenTypes(typeCount,false);
        for(int i=0;i<visibleTypes;++i){int id=r.bounded(0,typeCount-1);if(seenTypes[id])return false;seenTypes[id]=true;
            CObjectType t;std::string name;
            int n=r.bounded(1,80);for(int j=0;j<n;++j){int ch=r.bounded(32,126);if(ch=='/'||ch=='\\'||ch==':')return false;name.push_back(char(ch));}
            t.m_imageName=name;t.m_width=r.bounded(0,8);t.m_height=r.bounded(0,6);
            t.m_drawCells=std::bitset<48>(r.maskValue());t.m_passableCells=std::bitset<48>(r.maskValue());t.m_shadowCells=std::bitset<48>(r.maskValue());t.m_triggerCells=std::bitset<48>(r.maskValue());
            t.m_recommendedTerrainMask=std::bitset<10>(r.bounded(0,1023));t.m_objectType=static_cast<TAdventureObjectType>(r.bounded(0,231));t.m_extra=r.get();t.m_suppressDraw=r.bounded(0,1);t.m_objectTypeIndex=r.bounded(0,65535);
            if(apply){if(map.m_objectTypes[id].m_imageName!=name){auto* sprite=ResourceManager::GetSprite(name.c_str());ResourceManager::Dispose(map.m_sprites[id]);map.m_sprites[id]=sprite;}map.m_objectTypes[id]=t;}
        }
        int objectCount=r.bounded(0,65535),visibleObjects=r.bounded(0,objectCount);
        if(apply){map.m_objects.resize(objectCount);for(auto& o:map.m_objects){o.m_x=255;o.m_y=255;o.m_z=255;o.m_typeIndex=65535;o.m_extraInfo=0;}}
        std::vector<bool> seenObjects(objectCount,false);
        for(int i=0;i<visibleObjects;++i){int id=r.bounded(0,objectCount-1);if(seenObjects[id])return false;seenObjects[id]=true;
            // Multi-tile scenery may anchor just beyond the east/south edge.
            // Its visible footprint is bounded by the original 8x6 masks.
            int x=r.bounded(0,g_mapWidth+6),y=r.bounded(0,g_mapHeight+4),z=r.bounded(0,g_game->getNumMapLevels()-1),type=r.bounded(0,typeCount-1),frame=r.bounded(0,255);uint32_t extra=uint32_t(r.get());if(!seenTypes[type])return false;
            if(apply){auto& o=map.m_objects[id];o.m_x=x;o.m_y=y;o.m_z=z;o.m_typeIndex=type;o.m_animationOffset=frame;o.m_extraInfo=extra;}
        }
        for(int z=0;z<g_game->getNumMapLevels();++z)for(int y=0;y<g_mapHeight;++y)for(int x=0;x<g_mapWidth;++x){const int flags=r.get();if(flags!=0&&flags!=1&&flags!=2&&flags!=257)return false;bool visible=(flags&1)!=0;auto& cell=*map.cell(x,y,z);
            if(apply){*getMapExtraPtr(x,y,z)=visible?((1<<local)|(flags&0x100)):0;cell.m_objects.clear();cell.m_type=NOTHING;cell.m_extraInfo=0;cell.m_objectIndex=-1;cell.m_objectTypeIndex=-1;cell.m_isTrigger=0;}
            if(!flags)continue;
            if(visible){r.set(cell.m_type);r.set(cell.m_extraInfo);r.set(cell.m_objectIndex);r.set(cell.m_objectTypeIndex);r.set(cell.m_cellFlags);}
            int refs=r.bounded(0,256);
            for(int i=0;i<refs;++i){int id=r.bounded(0,objectCount-1),offset=r.bounded(0,255),layer=r.bounded(-128,127);if(!seenObjects[id])return false;
                if(apply){NewmapCell::TObjectCell ref;ref.m_objectIndex=id;ref.m_offsets=offset;ref.m_layer=layer;cell.m_objects.push_back(ref);}}
        }
        const int seers=r.bounded(0,65535),guards=r.bounded(0,65535);
        if(seers!=int(map.m_seerHutList.size())||guards!=int(map.m_questGuardList.size()))return false;
        if(apply)Port::AuthorityQuestClient::reset(seers,guards);
        for(int group=0;group<2;++group)for(int index=0;index<(group?guards:seers);++index){
            const bool active=r.bounded(0,1);std::string quick,rollover,journal;r.text(quick);r.text(rollover);r.text(journal);
            if(apply)Port::AuthorityQuestClient::row(group,index,active,std::move(quick),std::move(rollover),std::move(journal));
        }
        for(auto& flags:g_game->m_obeliskFlags){const int visited=r.bounded(0,1);if(apply)flags=visited?(1<<local):0;}
        return r.pos==r.count;
    }catch(const std::exception& error){std::fprintf(stderr,"Authority projection rejected: %s\n",error.what());return false;}
}
namespace Port { namespace AuthorityClient {
bool enabled(){
#ifdef __EMSCRIPTEN__
    return active||EM_ASM_INT({return !!window.homm3Authority?.active;});
#else
    return active;
#endif
}
bool modalInput(){return active&&(choosing||BattleClient::active());}
// The filtered world belongs to our seat. Only these original presentation
// routines temporarily use the acting seat; they never execute game rules.
struct TurnViewContext {
    playerData* player=g_currentPlayer;int net=g_netLocalGamePos;
    explicit TurnViewContext(int current){g_currentPlayer=&g_game->m_players[current];g_netLocalGamePos=current;}
    ~TurnViewContext(){g_currentPlayer=player;g_netLocalGamePos=net;}
};
int presentationActor(){
#ifdef __EMSCRIPTEN__
    const int current=EM_ASM_INT({return window.homm3AuthorityStatus?.activeActor??-1;});
    if(current>=0&&current<8)return current;
#endif
    return turn;
}
bool turnView(bool force,bool draw,bool update){
    if(!active||actor<0||!viewInstalled)return false;
    auto* win=g_advManager->m_advWindow;
    int transfer=transferStage;
#ifdef __EMSCRIPTEN__
    if(!transfer)transfer=EM_ASM_INT({return window.homm3Authority.turnTransfer;});
#endif
    const int current=presentationActor();
    if(transfer){
        // The original panel, without its save-file byte percentage: this
        // protocol sends commands/projections rather than a peer's save file.
        auto* panel=AuthorityTurnViewAccess::panel(win);
        const int x=panel?panel->m_x+8:622,y=panel?panel->m_y+5:403;
        // Keep the same backing message as the original save-transfer path.
        // The worker owns the world; this only updates the original UI model.
        g_advManager->m_bottomViewMessage=g_generalText->GetText(transfer==1?GENERAL_TEXT_SENDING_GAME:GENERAL_TEXT_RECEIVING_GAME);
        CGameTransferSmack animation;animation.setup(x,y,transfer==1,0);animation.start();animation.setPercentage(0);animation.stop();
        g_mediumFont->DrawBoundedString(g_generalText->GetText(transfer==1?GENERAL_TEXT_SENDING_GAME:GENERAL_TEXT_RECEIVING_GAME),g_windowManager->m_screenBitmap,x,y,160,160,font::PRIMARY,5,-1);
        g_windowManager->updateScreen(x,y,160,160);
#ifdef __EMSCRIPTEN__
        EM_ASM({window.homm3TurnPresentation=({mode:$0===1?'sending':'receiving',actor:$1});},transfer,current);
#endif
        return true;
    }
    if(current<0||current==actor)return false;
    TurnViewContext context(current);
    if(shownTurn!=current||force){g_advManager->clearBottomView();shownTurn=current;}
    const bool changed=AuthorityTurnViewAccess::enemy(force);
    if(draw&&(changed||force))win->drawBottomView(update);
#ifdef __EMSCRIPTEN__
    EM_ASM({window.homm3TurnPresentation=({mode:'waiting',actor:$0});},current);
#endif
    return true;
}
bool animateTurnView(bool background){
    if(!active||actor<0||!viewInstalled)return false;
    const int current=presentationActor();
    if(transferStage)return true;
    if(current<0||current==actor)return false;
    auto* win=g_advManager->m_advWindow;
    if(shownTurn!=current){turnView(true,true,true);return true;}
    TurnViewContext context(current);
    AuthorityTurnViewAccess::animate(win,background);
    return true;
}

bool recruitChoiceActive(){return choiceMode==3;}
void chooseRecruit(int creature,int count){if(choiceMode==3){choiceValue=creature;choiceQuantity=count;}}
bool universityChoiceActive(){return choiceMode==4;}
void chooseUniversity(int skill){if(choiceMode==4)choiceValue=skill;}
void finishMovementPresentation(){
    if(!active||!HD::adventureVisible())return;
    // Original MoveHero stops before dispatching an encounter. The route
    // continuation must not resume through a modal or keep a walking sample.
    g_advManager->stopCursor(true);
    if(movementRequestActive){movementStandEnd=true;g_heroMoveTriggeredEvent=1;}
    g_advManager->hideRoute(1,1,1);
}
void presentDeferred(){
#ifdef __EMSCRIPTEN__
    if(Fast::state().waiting||choosing||BattleClient::active()||!HD::adventureVisible())return;
    int event[3]={};char text[65537]={};if(EM_ASM_INT({return window.homm3Authority.takeUIEvent(HEAP32,$0>>2,HEAPU8.subarray($1,$1+65537),65537);},event,text)){
        choosing=true;
        finishMovementPresentation();
        ScopedPresentationPointer pointer;
        if(event[0]==3)g_advManager->bvResMsg(text,event[1],event[2]);
        else AuthorityActionsClient::present(event[0]==1?"open_town":"hero_exchange",event[1],event[2]);
        choosing=false;
    }
#endif
}
void captureSetup(){
    if(!enabled())return;
#if defined(__EMSCRIPTEN__) && defined(HOMM3_HARNESS)
    if(EM_ASM_INT({const setup=window.homm3Harness?.fixture?.setup;if(!Array.isArray(setup)||setup.length!==41)return 0;HEAP32.set(setup,$0>>2);return 1;},startupSetup)){capturedSetup=true;return;}
#endif
#ifdef __EMSCRIPTEN__
    // Restore already has the room's validated color-to-seat mapping; its
    // presentation-only player records have no original network DPID yet.
    if(EM_ASM_INT({const setup=window.homm3AuthorityRestore?.setup;if(!Array.isArray(setup)||setup.length!==41)return 0;HEAP32.set(setup,$0>>2);return 1;},startupSetup)){capturedSetup=true;return;}
#endif
    startupSetup[0]=g_game->m_setup.m_difficulty;
    for(int color=0;color<8;++color){
        int* p=startupSetup+1+color*5;p[0]=g_game->m_setup.m_playerPos[color];
#ifdef __EMSCRIPTEN__
        // The original hot-seat selector encodes remote humans as position10
        // too. Its player roster retains each authenticated room DPID, so
        // derive the seat ordinal from that roster instead of treating10 asAI.
        if(p[0]>=0&&g_game->m_players[color].m_isHuman){
            const int seat=EM_ASM_INT({return (window.homm3Network?.players||[]).findIndex(player=>Number(player.dpid)===$0);},int(g_game->m_players[color].m_dpid));
            if(seat<0||seat>=8)throw std::runtime_error("Selected human does not match the room roster");
            p[0]=seat;
        }else if(p[0]>=0)p[0]=10;
#endif
        p[1]=g_game->m_setup.m_alignment[color];p[2]=g_startingHeroOverrides[color];p[3]=g_newMapStartingBonus[color];p[4]=g_game->m_setup.m_handicap[color];
    }
    capturedSetup=true;
}
void begin(){
#ifdef __EMSCRIPTEN__
    if(active||!EM_ASM_INT({return !!window.homm3Authority?.active;}))return;
    if(!capturedSetup)captureSetup();
    active=true;Fast::state().active=true;
    EM_ASM({window.homm3Authority.nativeReady(UTF8ToString($0),Array.from(HEAP32.subarray($1>>2,($1>>2)+41)));},g_game->m_setup.m_filename,startupSetup);tick();
#endif
}
void playAdventureSounds(){
#ifdef __EMSCRIPTEN__
    // Stopping/redrawing the moving cursor pumps events. Keep a nested tick
    // from opening the encounter modal before its sound has actually started.
    struct RestoreChoice {bool previous;~RestoreChoice(){choosing=previous;}} restore{choosing};
    choosing=true;
    char name[81];while(EM_ASM_INT({return window.homm3Authority.takeAdventureSound(HEAPU8.subarray($0,$0+81),81);},name)){
        if(movementRequestActive)finishMovementPresentation();
        launchSample(name,-1,3);
    }
#endif
}
void tick(){
#ifdef __EMSCRIPTEN__
    if(!active||applying||choosing||failed||BattleClient::active())return;
    playAdventureSounds();
    if(pendingTurnPopup&&!Fast::state().waiting&&HD::adventureVisible()){
        pendingTurnPopup=false;choosing=true;
        popupPlayerTurnInfo();
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_1,-1);
        g_advManager->updBottomView(1,1,1);choosing=false;
#ifdef __EMSCRIPTEN__
        EM_ASM({window.homm3TurnPresentation=({mode:'new_turn',actor:$0});},actor);
#endif
    }
    // JS only copies at this explicit engine boundary. It never invokes a
    // WASM mutation while Asyncify is suspended in an unrelated modal.
    if(!choosing){
        int choice[64]={};char text[65537]={};
        if(EM_ASM_INT({return window.homm3Authority.takeChoiceError(HEAPU8.subarray($0,$0+65537),65537);},text))normalDialog(text,1,-1,-1,-1,0,-1,0,-1,0,-1,0);
        if(EM_ASM_INT({return window.homm3Authority.takeNotice(HEAPU8.subarray($0,$0+65537),65537);},text)){
            choosing=true;normalDialog(text,1,-1,-1,-1,0,-1,0,-1,0,-1,0);choosing=false;EM_ASM({window.homm3Authority.noticeShown();});
        }
        int kind=EM_ASM_INT({return window.homm3Authority.takeChoice(HEAP32,$0>>2,HEAPU8.subarray($1,$1+65537),65537,$2);},choice,text,int(sizeof(choice)/sizeof(choice[0])));
        if(kind){
            finishMovementPresentation();
            ScopedPresentationPointer pointer;
            choosing=true;
            int answer=0,quantity=0;
            if(kind==15){AuthorityPuzzleClient::showPending(choice[1],choice[4]);choosing=false;return;}
            if(AuthorityExchangeClient::present(choice)){choosing=false;return;}
            if(presentExtendedChoice(choice,text)){choosing=false;return;}
            if(kind==1){normalDialog(text,choice[1],choice[9],choice[10],choice[3],choice[4],choice[5],choice[6],choice[2],0,choice[7],choice[8]);answer=g_windowManager->m_dialogReturn;}
            else if(kind==2&&choice[1]>=0&&choice[1]<game::HERO_COUNT){
                auto& h=g_game->m_heroes[choice[1]];
                const int first=choice[3],second=choice[4];
                if(first < -1 || first>=28 || second < -1 || second>=28){failed=true;choosing=false;return;}
                TLevelUpWindow window(&h,choice[2],first<0?-1:first*3+3+h.m_skillLevel[first],second<0?-1:second*3+3+h.m_skillLevel[second]);
                window.doModal(0);
                answer=second>=0&&g_windowManager->m_dialogReturn==TLevelUpWindow::SKILLICON_2_ID?second:first;
            }
            else if(kind==3&&choice[1]>=0&&choice[1]<game::HERO_COUNT){
                auto& h=g_game->m_heroes[choice[1]];short available[4]={};TCreatureType types[4];
                for(int i=0;i<4;++i){types[i]=static_cast<TCreatureType>(choice[4+i*2]);available[i]=choice[5+i*2];}
                choiceMode=3;choiceValue=-1;choiceQuantity=0;
                if(choice[2]){recruitUnit manager(&h,types[0],&available[0],types[1],&available[1],types[2],&available[2],types[3],&available[3]);g_executive->doDialog(&manager);}
                else {recruitUnit manager(&h.m_army,0,types[0],&available[0],types[1],&available[1],types[2],&available[2],types[3],&available[3]);g_executive->doDialog(&manager);}
                answer=choiceValue;quantity=choiceQuantity;choiceMode=0;
            }else if(kind==4&&choice[1]>=0&&choice[1]<game::HERO_COUNT){
                type_university university;for(int i=0;i<4;++i)university.m_skills[i]=static_cast<TSecondarySkill>(choice[3+i]);
                choiceMode=4;choiceValue=-1;type_university_window window(&g_game->m_heroes[choice[1]],&university,choice[7]==1);window.doModal(false);answer=choiceValue;choiceMode=0;
            }
            EM_ASM({window.homm3Authority.answer($0,$1);},answer,quantity);choosing=false;
        }
    }
    int length=EM_ASM_INT({return window.homm3Authority.pendingLength();});
    if(!length&&(viewInstalled||actor<0||!HD::adventureVisible())){if(!BattleClient::active())BattleClient::begin();presentDeferred();return;}
    if(length<0||length>Authority::ProjectionMaxWords){failed=true;return;}
    type_point targets[game::HERO_COUNT];for(int id=0;id<game::HERO_COUNT;++id)targets[id]=g_game->m_heroes[id].getTarget();
    const int previousTurn=turn;
    const int previousDate=g_game->m_month*28+g_game->m_week*7+g_game->m_day;
    const int previousHero=g_game->getLocalPlayer()->m_currHeroId;
    const bool previousMobile=g_advManager->m_curHeroMobile;
    const type_point previousLocation=previousHero>=0?g_game->m_heroes[previousHero].getLocation():type_point(-1,-1,-1);
    applying=true;
    if(length){
        pending.resize(length);
        EM_ASM({window.homm3Authority.copyPending(HEAP32,$0>>2);},pending.data());
        if(viewInstalled&&HD::adventureVisible()&&length>=6&&pending[2]==actor&&(previousTurn!=actor||previousDate!=pending[5]*28+pending[4]*7+pending[3])){
            transferStage=2;turnView(false,true,true);Port::Harness::wait(16);transferStage=0;
        }
        if(!AuthorityProjectionAccess::read(pending.data(),pending.size(),false)||!AuthorityProjectionAccess::read(pending.data(),pending.size(),true)){
            failed=true;EM_ASM({window.homm3Authority.fail('Projection does not match the selected map');});applying=false;return;
        }
    }
    if(length)EM_ASM({window.homm3Authority.applied();});
    g_localGamePos=actor;g_netLocalGamePos=actor;g_playerTurn=turn;g_currentPlayer=&g_game->m_players[actor];g_curPlayerBit=1<<actor;g_curWatchPlayer=actor;g_mapVisibilityBit=1<<actor;
    g_currentPlayer->m_isLocal=turn==actor;g_thisNetGotAdventureControl=turn==actor;g_remoteOn=0;
    Fast::state().ready=turn!=actor;
    auto& p=*g_currentPlayer;
    if(!viewInstalled&&turn!=actor){
        // Adventure startup can have mobilized the local hero after the first
        // projection. With no fresh map packet, restore that cell before
        // dropping selection; otherwise the waiting hero disappears until
        // the next server update.
        if(!length)g_advManager->demobilizeCurrHero(false,false);
        p.m_currHeroId=static_cast<HeroId>(-1);p.m_currTownId=-1;
    }
    if(viewInstalled&&previousTurn==actor&&turn!=actor){
        // Original turn handoff clears selection before entering wait mode.
        // Keep the projected hero in its map cell; do not turn it into a cursor.
        p.m_currHeroId=static_cast<HeroId>(-1);p.m_currTownId=-1;
    }
    if(std::find(p.m_heroes,p.m_heroes+p.m_numHeroes,p.m_currHeroId)==p.m_heroes+p.m_numHeroes){
        // No hero selected is intentional when viewing a town or waiting.
        // Only start a fresh active turn with the first available hero.
        p.m_currHeroId=turn==actor&&(!viewInstalled||previousTurn!=actor)&&p.m_numHeroes?p.m_heroes[0]:static_cast<HeroId>(-1);
    }
    if(p.m_currHeroId>=0)p.m_currTownId=-1;
    else if(p.m_currTownId<0||p.m_currTownId>=int(g_game->m_towns.size())||g_game->m_towns[p.m_currTownId].m_owner!=actor)p.m_currTownId=turn==actor&&p.m_numTowns?p.m_townIds[0]:-1;
    // The original misleadingly named all-cells flag is the AI blackout
    // screen: it paints opaque shroud and removes all locator portraits.
    g_completeDrawAllCells=0;g_completeDrawEnabled=1;g_blackoutPlayer=0;
    if(HD::adventureVisible()){
        const auto camera=g_advManager->m_radarOrigin;
        const bool sameHero=p.m_currHeroId>=0&&p.m_currHeroId==previousHero;
        const bool continuing=viewInstalled&&sameHero&&turn==previousTurn;
        g_advManager->m_curHeroMobile=0;
        // Waiting selection must leave the hero in the canonical map cell.
        // The mobile-cursor path removes it; original waiting-player town
        // selection deliberately does not demobilize that cursor again.
        const bool samePosition=sameHero&&g_game->m_heroes[p.m_currHeroId].getLocation()==previousLocation;
        if(continuing&&(turn!=actor||previousMobile||samePosition)){
            // The projection restores canonical map cells. Keep an existing
            // mobile cursor without reselecting the hero (which restarts all
            // ambient loops and recentres the camera on every server update).
            // Puzzle windows deliberately demobilize the hero. A same-position
            // acknowledgement must preserve that original selection state.
            if(turn==actor&&previousMobile){g_game->m_heroes[p.m_currHeroId].restoreCell();g_advManager->m_curHeroMobile=1;}
        }else if(p.m_currHeroId>=0)g_advManager->setHeroContext(p.m_currHeroId,0,turn!=actor,false);
        if(turn!=actor)g_advManager->m_drawCursor=0;
        hero* selected=p.m_currHeroId>=0?&g_game->m_heroes[p.m_currHeroId]:nullptr;
        const int dx=selected?int(selected->m_x)-int(previousLocation.m_x):0;
        const int dy=selected?int(selected->m_y)-int(previousLocation.m_y):0;
        const bool moved=sameHero&&(dx||dy||selected->m_z!=previousLocation.m_z);
        if(viewInstalled&&sameHero&&turn==previousTurn&&!moved)g_advManager->m_radarOrigin=camera;
        if(viewInstalled&&moved&&selected->m_z==previousLocation.m_z&&std::abs(dx)<=1&&std::abs(dy)<=1){
            // Only replay the original cursor/scroll presentation. Legality,
            // costs, encounters and the final coordinates came from the server.
            int direction=0;while(direction<8&&(g_normalDirTable[direction].m_x!=dx||g_normalDirTable[direction].m_y!=dy))++direction;
            if(direction<8){
                if(g_advManager->m_showRoute)*g_advManager->getRouteArrayPtr(selected->m_x,selected->m_y,selected->m_z)=0;
                selected->m_x=previousLocation.m_x;selected->m_y=previousLocation.m_y;
                g_advManager->turnTo(direction);
                g_advManager->animateMove(selected,direction,dx,dy);
                g_advManager->stopCursor(!Fast::state().waiting||movementStandEnd);
                g_advManager->setEnvironmentOrigin(g_advManager->getMapCenter(),0);
            }
        }
        // A continuing route already owns the walking presentation. Full
        // window redraws here reset the original cursor cycle between tiles.
        // Reconcile the world, then let the next original step paint it.
        const bool continuous=continuing&&turn==actor&&Fast::state().waiting&&!movementStandEnd&&
            !EM_ASM_INT({return !!window.homm3Authority.pendingChoice||window.homm3AuthorityStatus?.phase!=='adventure';});
        if(continuous){
            for(int id=0;id<game::HERO_COUNT;++id)if(g_game->m_heroes[id].m_owner==actor){auto& h=g_game->m_heroes[id];h.m_pathTargetX=targets[id].m_x;h.m_pathTargetY=targets[id].m_y;h.m_pathTargetZ=targets[id].m_z;}
            pending.clear();applying=false;return;
        }
        g_game->cancelComputerScreen();g_advManager->enableButtons();
        if(turn!=actor)g_game->showComputerScreen();
        auto* win=g_advManager->m_advWindow;
        win->getWidget(TAdventureMapWindow::END_TURN_ID)->enable(turn==actor&&!EM_ASM_INT({return window.homm3Authority.eliminated||window.homm3Authority.finished;}));
        win->updateSpellButton(p.m_currHeroId>=0?&g_game->m_heroes[p.m_currHeroId]:nullptr);
        win->updateSleepButton(p.m_currHeroId>=0?&g_game->m_heroes[p.m_currHeroId]:nullptr);
        win->updateHeroLocators(-1,1,1);win->updateTownLocators(-1,1,1);win->updateResourceDisplay(1,1);
        const bool newTurn=turn==actor&&viewInstalled&&(previousTurn!=actor||previousDate!=g_game->m_month*28+g_game->m_week*7+g_game->m_day);
        if(newTurn)pendingTurnPopup=true;
        if(viewInstalled&&turn!=previousTurn&&turn!=actor&&turn>=0&&g_game->m_players[turn].isHuman())
            g_chatMan.systemMsg((*g_generalText)[GENERAL_TEXT_PLAYER_TURN_ITS_FORMAT],g_game->m_players[turn].m_name);
        if(turn!=previousTurn)g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_DEFAULT,0);
        // Let the original 3-second new-day/week card expire naturally.
        g_advManager->updBottomView(1,1,0);
        for(int id=0;id<game::HERO_COUNT;++id)if(viewInstalled&&g_game->m_heroes[id].m_owner==actor){auto& h=g_game->m_heroes[id];h.m_pathTargetX=targets[id].m_x;h.m_pathTargetY=targets[id].m_y;h.m_pathTargetZ=targets[id].m_z;}
        g_advManager->reseed(0,0);
        if(turn==actor&&selected&&selected->m_pathTargetX>=0&&selected->getTarget()!=selected->getLocation()){g_advManager->seedTo(selected->getTarget());g_advManager->showRoute(0,0,1);}
        g_advManager->forceNewHover();g_advManager->updateRadar(1,1,0,0,0);
        g_advManager->redrawAdvScreen(1,0);g_advManager->updateScreen(1,0);viewInstalled=true;
        if(turn!=actor)g_mouseManager->setPointer(2,mouseManager::DEFAULT_SET);
    }
    pending.clear();applying=false;
    playAdventureSounds();
    if(!BattleClient::active())BattleClient::begin();presentDeferred();
#endif
}
namespace {
void discardPendingActions(){
    // Discard queued commands while retaining button releases for the original
    // widget that initiated this synchronous request. Dropping its release
    // leaves WIDGET_SELECTED stuck after an army transfer.
    process1WindowsMessage();
    message releases[2];int count=0;
    for(int i=g_inputManager->m_head;i!=g_inputManager->m_tail;i=(i+1)%64){
        const auto& event=g_inputManager->m_buffer[i];
        if(event.m_id!=MESSAGE_LEFT_BUTTON_UP&&event.m_id!=MESSAGE_RIGHT_BUTTON_UP)continue;
        int slot=0;while(slot<count&&releases[slot].m_id!=event.m_id)++slot;
        releases[slot]=event;if(slot==count)++count;
    }
    g_inputManager->m_head=0;g_inputManager->m_tail=count;
    for(int i=0;i<count;++i)g_inputManager->m_buffer[i]=releases[i];
}
}
bool requestMove(int id,int direction,bool standEnd){
    struct Restore {bool stand,active;~Restore(){movementStandEnd=stand;movementRequestActive=active;}} restore{movementStandEnd,movementRequestActive};
    movementStandEnd=standEnd;movementRequestActive=true;g_heroMoveTriggeredEvent=0;
    return request(1,id,direction);
}
bool request(int op,int id,int a,int b,int c,int d,int e){
#ifdef __EMSCRIPTEN__
    if(!active||failed||turn!=actor)return false;
    int ticket=EM_ASM_INT({return window.homm3Authority.send($0,$1,$2,$3,$4,$5,$6);},op,id,a,b,c,d,e);if(!ticket)return false;
    const bool outerChoiceGuard=choosing;choosing=false;
    Fast::state().waiting=true;int result=0;
    // Predict presentation only on clear, disclosed, unguarded terrain. The
    // command is already on the wire; authority still owns cost and legality.
    hero* predicted=nullptr;type_point predictionStart;const auto predictionCamera=g_advManager->m_radarOrigin;int predictionFacing=0;
    if(op==1&&id>=0&&id<game::HERO_COUNT&&a>=0&&a<8&&HD::adventureVisible()){
        auto& h=g_game->m_heroes[id];const auto start=h.getLocation();
        const type_point dest(start.m_x+g_normalDirTable[a].m_x,start.m_y+g_normalDirTable[a].m_y,start.m_z);
        if(h.m_owner==actor&&g_currentPlayer->m_currHeroId==id&&dest.isValid()&&g_advManager->m_curHeroMobile){
            const auto* cell=g_game->getCell(dest);const int extra=getMapExtra(dest.m_x,dest.m_y,dest.m_z);
            if(cell->m_type==NOTHING&&!cell->m_isTrigger&&(extra&(1<<actor))&&!(extra&0x100)&&g_advManager->validMove(&h,a,0,true)){
                predicted=&h;predictionStart=start;predictionFacing=h.m_facing;applying=true;
                *g_advManager->getRouteArrayPtr(dest.m_x,dest.m_y,dest.m_z)=0;
                h.m_facing=a;g_advManager->turnTo(a);g_advManager->animateMove(&h,a,g_normalDirTable[a].m_x,g_normalDirTable[a].m_y);
                g_advManager->stopCursor(movementStandEnd);g_advManager->setEnvironmentOrigin(g_advManager->getMapCenter(),0);applying=false;
            }
        }
    }
    if(op==5&&HD::adventureVisible()){transferStage=1;turnView(true,true,true);Port::Harness::wait(16);transferStage=0;}
    while(!failed&&!(result=EM_ASM_INT({return window.homm3Authority.result($0);},ticket))){
        // In-flight commands own the original UI continuation; no unrelated
        // window handler can run until the authoritative state is installed.
        tick();Port::Harness::wait(8);
    }
    tick();Fast::state().waiting=false;choosing=outerChoiceGuard;discardPendingActions();
    if(result<0&&predicted){
        predicted->m_x=predictionStart.m_x;predicted->m_y=predictionStart.m_y;predicted->m_z=predictionStart.m_z;predicted->m_facing=predictionFacing;
        g_advManager->m_radarOrigin=predictionCamera;g_advManager->stopCursor(true);g_advManager->reseed(0,0);
        if(predicted->m_pathTargetX>=0){g_advManager->seedTo(predicted->getTarget());g_advManager->showRoute(0,0,1);}
        g_advManager->setEnvironmentOrigin(g_advManager->getMapCenter(),0);g_advManager->redrawAdvScreen(1,0);g_advManager->updateScreen(1,0);
    }
    if(result<0&&!EM_ASM_INT({return window.homm3Authority.fatal;})){
        ScopedPresentationPointer pointer;
        char error[512]={};EM_ASM({stringToUTF8(window.homm3Authority.lastError||'Command rejected',$0,512);},error);
        normalDialog(error,1,-1,-1,-1,0,-1,0,-1,0,-1,0);
    }
    return result==1;
#else
    return false;
#endif
}
bool requestRecruit(armyGroup* army,short* available,int creature,int count){
    for(auto& t:g_game->m_towns)if(t.m_owner==actor&&&t.getArmy()==army)
        for(int slot=0;slot<14;++slot)if(available==&t.m_population[slot]&&g_dwellingType[t.m_type][slot]==creature)return request(4,t.m_id,slot,count);
    return false;
}
} }
