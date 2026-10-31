#include "parity_wait.h"
#include "authority_puzzle_client.h"
#include "authority_choices.h"
#include "authority_actions_client.h"
#include "authority_market_client.h"
#include "authority_guild_client.h"
#include "server_projection.h"
#include "game.h"
#include "hero.h"
#include "townmgr.h"
#include "hillfortwindow.h"
#include "sacrifice_window.h"
#include "winmgr.h"
#include "levelupwindow.h"
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
extern hero* g_tavernHero;
extern int g_mapTavern;
namespace {
int mode=0,heroId=-1,selected=-1;
const hero* sacrificeOriginal=nullptr;
std::vector<std::pair<int,int>> sacrificeStages;bool sacrificeCommitted=false;
armyGroup other;
int current[64]={};
void updateFields(const int* fields){
    std::copy_n(fields,64,current);
    if(mode==5)for(int slot=0;slot<7;++slot){other.m_armies[slot]=fields[7+slot*2];other.m_numTroops[slot]=fields[8+slot*2];}
    if(mode==7)Port::AuthorityMarketClient::refresh(fields+6);
}
// A modal retains pointers to the fixed hero pool and this local other army.
// Install only at this explicit, suspended-input boundary; never call tick()
// recursively or reopen the same window while its callback is on the stack.
bool repeatReply(int value,int a,int b){
#ifdef __EMSCRIPTEN__
    if(!EM_ASM_INT({return window.homm3Authority.answer($0,$1,$2);},value,a,b))return false;
    for(;;){
        if(EM_ASM_INT({return window.homm3Authority.fatal;}))return false;
        int length=EM_ASM_INT({return window.homm3Authority.pendingLength();});
        if(length){
            if(length<0||length>Port::Authority::ProjectionMaxWords)return false;
            std::vector<int32_t> words(length);
            EM_ASM({window.homm3Authority.copyPending(HEAP32,$0>>2);},words.data());
            if(!AuthorityProjectionAccess::read(words.data(),words.size(),false)||!AuthorityProjectionAccess::read(words.data(),words.size(),true)){
                EM_ASM({window.homm3Authority.fail('Invalid encounter world projection');});return false;
            }
            EM_ASM({window.homm3Authority.applied();});
        }
        char text[65537]={};int fields[64]={};
        if(EM_ASM_INT({return window.homm3Authority.takeChoiceError(HEAPU8.subarray($0,$0+65537),65537);},text))normalDialog(text,1,-1,-1,-1,0,-1,0,-1,0,-1,0);
        int next=EM_ASM_INT({return window.homm3Authority.takeChoice(HEAP32,$0>>2,HEAPU8.subarray($1,$1+65537),65537,$2);},fields,text,int(sizeof(fields)/sizeof(fields[0])));
        if(next){
            if(next==15){Port::AuthorityPuzzleClient::showPending(fields[1],fields[4]);continue;}
            if(next==11){Port::AuthorityClient::presentExtendedChoice(fields,text);continue;}
            if(next==14){Port::AuthorityGuildClient::showPending();EM_ASM({window.homm3Authority.answer(0);});continue;}
            if(next==1){
                normalDialog(text,fields[1],fields[9],fields[10],fields[3],fields[4],fields[5],fields[6],fields[2],0,fields[7],fields[8]);
                EM_ASM({window.homm3Authority.answer($0);},g_windowManager->m_dialogReturn);continue;
            }
            if(next==2&&fields[1]>=0&&fields[1]<game::HERO_COUNT){
                auto& who=g_game->m_heroes[fields[1]];const int first=fields[3],second=fields[4];
                if(first < -1 || first>=28 || second < -1 || second>=28){EM_ASM({window.homm3Authority.fail('Invalid encounter level choices');});return false;}
                TLevelUpWindow window(&who,fields[2],first<0?-1:first*3+3+who.m_skillLevel[first],second<0?-1:second*3+3+who.m_skillLevel[second]);window.doModal(0);
                const int answer=second>=0&&g_windowManager->m_dialogReturn==TLevelUpWindow::SKILLICON_2_ID?second:first;
                EM_ASM({window.homm3Authority.answer($0);},answer);continue;
            }
            if(next!=mode||fields[1]!=heroId){EM_ASM({window.homm3Authority.fail('Encounter changed while its original window was open');});return false;}
            updateFields(fields);return true;
        }
        Port::Harness::wait(10);
    }
#else
    return false;
#endif
}
}
struct AuthoritySacrificeAccess {
    static void capture(type_sacrifice_window& window){
        sacrificeStages.clear();sacrificeCommitted=true;
        if(window.m_sacrificingArtifacts){
            bool used[83]={};
            auto select=[&](const type_artifact_offering& offering){
                if(offering.m_artifactId<0)return;
                int slot=-1;
                auto matches=[&](int source){const auto item=source<19?sacrificeOriginal->getArtifact(static_cast<TArtifactSlot>(source)):sacrificeOriginal->getBackpack(source-19);return !used[source]&&item.m_artifactId==offering.m_artifactId&&item.m_extra==offering.m_extra;};
                if(offering.m_source>=0&&offering.m_source<19&&matches(offering.m_source))slot=offering.m_source;
                else for(int i=0;i<83;++i)if(matches(i)){slot=i;break;}
                if(slot<0||used[slot]){sacrificeCommitted=false;return;}
                used[slot]=true;sacrificeStages.emplace_back(2,slot);
            };
            for(const auto& offering:window.m_artifactOfferings)select(offering);
            select(window.m_holdingArtifact);
        }else for(int slot=0;slot<7;++slot)if(window.m_creatureOfferings[slot].m_amount>0)sacrificeStages.emplace_back(slot,int(window.m_creatureOfferings[slot].m_amount));
        selected=window.m_sacrificingArtifacts?2:1;
    }
};
namespace Port { namespace AuthorityClient {
bool repeatExtendedChoice(int kind,int hero,int value,int a,int b,int* fields64){
    const int savedMode=mode,savedHero=heroId;mode=kind;heroId=hero;
    const bool okay=repeatReply(value,a,b);if(okay&&fields64)std::copy_n(current,64,fields64);
    mode=savedMode;heroId=savedHero;return okay;
}
bool hillFortChoiceActive(){return mode==8;}
void chooseHillFort(int slot){if(mode==8)selected=slot;}
bool sacrificeChoiceActive(){return mode==10;}
void chooseSacrifice(type_sacrifice_window* window){if(mode==10&&window)AuthoritySacrificeAccess::capture(*window);}
bool armyExchangeActive(){return mode==5;}
bool chooseArmyTransfer(armyGroup* source,int src,armyGroup* target,int dst,int count,int operation){
    if(mode!=5)return false;
    auto* own=&g_game->m_heroes[heroId].m_army;
    if((source!=own&&source!=&other)||(target!=own&&target!=&other)||src<0||src>=7||dst<0||dst>=7)return true;
    int action=source==own?(target==own?3:1):(target==own?2:4);
    repeatReply(action,operation==1?0:count,src*7+dst);return true;
}
bool chooseArmyDismiss(armyGroup* group,int slot){
    if(mode!=5)return false;
    auto* own=&g_game->m_heroes[heroId].m_army;
    if(slot>=0&&slot<7&&(group==own||group==&other))repeatReply(group==own?5:6,0,slot);
    return true;
}
bool chooseArmySwap(){if(mode!=5)return false;repeatReply(7,0,0);return true;}
bool mapTavernChoiceActive(){return mode==9;}
void chooseMapTavernGuild(){if(mode==9)repeatReply(-2,0,0);}
bool marketChoiceActive(){return mode==7;}
void chooseMarket(int kind,int units,int packed){if(mode==7)repeatReply(kind,units,packed);}
bool presentExtendedChoice(const int* fields,const char* text){
#ifdef __EMSCRIPTEN__
    if(fields[0]==11){
        int length=EM_ASM_INT({return window.homm3Authority.normalIconsLength();});if(length<0||length>4096||length%2){EM_ASM({window.homm3Authority.fail('Invalid extended dialog resources');});return true;}
        std::vector<int> words(length);EM_ASM({window.homm3Authority.copyNormalIcons(HEAP32,$0>>2);},words.data());
        std::vector<type_dialog_resource> icons;for(int i=0;i<length;i+=2){type_dialog_resource item;item.m_resource=words[i];item.m_qualifier=words[i+1];icons.push_back(item);}
        extendedDialog(text,icons,-1,-1,0);EM_ASM({window.homm3Authority.answer($0);},DIALOG_RETURN_OK);return true;
    }
    if(fields[0]==16){doShipyard(fields[1]);EM_ASM({window.homm3Authority.answer($0);},g_windowManager->m_dialogReturn);return true;}
    if(fields[0]==14){AuthorityGuildClient::showPending();EM_ASM({window.homm3Authority.answer(0);});return true;}
    if(fields[0]==6){
        int length=EM_ASM_INT({return window.homm3Authority.worldMarkersLength();});
        if(length<0||length>1000000||length%6){EM_ASM({window.homm3Authority.fail('Invalid world spell symbols');});return true;}
        std::vector<int> markers(length);EM_ASM({window.homm3Authority.copyWorldMarkers(HEAP32,$0>>2);},markers.data());
        AuthorityActionsClient::showWorld(fields[1],fields[2],markers);
        EM_ASM({window.homm3Authority.answer(0);});return true;
    }
    if(fields[0]>=8&&fields[0]<=10){
        if(fields[1]<0||fields[1]>=game::HERO_COUNT){EM_ASM({window.homm3Authority.fail('Invalid service hero');});return true;}
        mode=fields[0];heroId=fields[1];selected=-1;
        if(mode==8){
            const auto saved=g_currentPlayer->m_currHeroId;g_currentPlayer->m_currHeroId=static_cast<HeroId>(heroId);
            THillFortWindow window;window.doModal();g_currentPlayer->m_currHeroId=saved;
            EM_ASM({window.homm3Authority.answer($0);},selected);
        }else if(mode==9){
            const int saved=g_mapTavern;g_mapTavern=1;
            if(doTavern()&&g_tavernHero)selected=g_tavernHero->m_id;
            g_mapTavern=saved;EM_ASM({window.homm3Authority.answer($0);},selected);
        }else{
            sacrificeOriginal=&g_game->m_heroes[heroId];hero preview=*sacrificeOriginal;
            sacrificeCommitted=false;sacrificeStages.clear();
            {type_sacrifice_window window(&preview,g_localGamePos);window.doModal(false);}
            if(sacrificeCommitted&&!sacrificeStages.empty()){
                bool okay=repeatReply(4,0,0);
                for(const auto& stage:sacrificeStages)if(okay)okay=selected==2?repeatReply(2,stage.second,1):repeatReply(1,stage.first,stage.second);
                if(okay)EM_ASM({window.homm3Authority.answer(3);});
            }else EM_ASM({window.homm3Authority.answer(0);});
            sacrificeOriginal=nullptr;
        }
        mode=0;heroId=-1;return true;
    }
    if(fields[0]!=5&&fields[0]!=7)return false;
    if(fields[1]<(fields[0]==7?-1:0)||fields[1]>=game::HERO_COUNT){EM_ASM({window.homm3Authority.fail('Invalid encounter hero');});return true;}
    mode=fields[0];heroId=fields[1];updateFields(fields);
    if(mode==5){
        auto* who=&g_game->m_heroes[heroId];
        // Empty recruitment offers cannot name a creature in the original
        // constructor. Guard placement (flags != 0) has its own title and
        // must open even when the mine has no guards yet.
        if(fields[2]&&(fields[3]||other.hasCreatures())){type_monster_join_window window(who,&other,fields[3]);window.doModal(false);}
        else if(!fields[2]){TGarrisonWindow window(who,fields[4],other);window.doModal(false);}
    }else AuthorityMarketClient::show(heroId,fields[2],fields[3],fields[4],fields[5],fields+6);
    EM_ASM({if(!window.homm3Authority.fatal)window.homm3Authority.answer(0);});
    mode=0;heroId=-1;return true;
#else
    return false;
#endif
}
} }
