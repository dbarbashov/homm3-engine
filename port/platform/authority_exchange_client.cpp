#include "authority_exchange_client.h"
#include "authority_choices.h"
#include "game.h"
#include "hero.h"
#include "swapmgr.h"
#include "exec.h"
#include "mousemgr.h"
#include "kb.h"
#include "winmgr.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace Port { namespace AuthorityExchangeClient {
namespace {
bool showing=false;int ids[2]={-1,-1},fields[64]={},dragSide=-1,dragSlot=-1;
int side(int id){return id==ids[0]?0:id==ids[1]?1:-1;}
int side(armyGroup* army){for(int i=0;i<2;++i)if(ids[i]>=0&&army==&g_game->m_heroes[ids[i]].m_army)return i;return -1;}
bool reply(int op,int a=0,int b=0){return AuthorityClient::repeatExtendedChoice(13,ids[0],op,a,b,fields);}
}
bool active(){return showing;}
bool canModify(int id){return showing&&side(id)>=0&&g_game->m_heroes[id].m_owner==g_localGamePos;}
bool canReceive(int id){return showing&&side(id)>=0;}
void cancelDrag(){dragSide=dragSlot=-1;g_heroScreenDraggedArtifact=type_artifact(ARTIFACT_NONE);g_mouseManager->setPointer(0,mouseManager::DEFAULT_SET);}
bool transfer(armyGroup* source,int src,armyGroup* target,int dst,int count,int mode){
    if(!showing)return false;const int from=side(source),to=side(target);if(from<0||to<0||src<0||src>=7||dst<0||dst>=7)return true;
    reply(1,mode==1?0:count,from*98+to*49+src*7+dst);return true;
}
void artifactClick(int heroId,int slot,bool rightClick){
    const int which=side(heroId);if(!showing||which<0)return;auto& hero=g_game->m_heroes[heroId];
    if(!((slot>=0&&slot<19)||(slot>=100&&slot<164)))return;
    const auto artifact=slot>=100?hero.getBackpack(slot-100):hero.getArtifact(TArtifactSlot(slot));
    if(rightClick){
        if(dragSide>=0){cancelDrag();return;}if(artifact.m_artifactId<0)return;
        if(canModify(heroId)&&slot<100&&akArtifactTraits[artifact.m_artifactId].m_comboType>=0){if(hero.showDisassembleArtifactDialog(artifact.m_artifactId)==DIALOG_RETURN_ACCEPT)reply(5,which*1000+slot,1);}
        else if(canModify(heroId)&&slot<100&&akArtifactTraits[artifact.m_artifactId].m_targetCombo>=0&&hero.hasCombinationArtifactComponents(akArtifactTraits[artifact.m_artifactId].m_targetCombo)){if(hero.showAssembleArtifactDialog(artifact.m_artifactId)==DIALOG_RETURN_ACCEPT)reply(5,which*1000+slot,0);}
        else hero.viewArtifact(artifact,true);return;
    }
    if(dragSide<0){
        if(!canModify(heroId)||artifact.m_artifactId<0||artifact.m_artifactId==ARTIFACT_CATAPULT||artifact.m_artifactId==ARTIFACT_SPELLBOOK)return;
        dragSide=which;dragSlot=slot;g_heroScreenDraggedArtifact=artifact;g_mouseManager->setPointer(artifact.m_artifactId,mouseManager::ARTIFACT_SET);return;
    }
    const int from=dragSide,source=dragSlot;cancelDrag();if(from!=which||source!=slot)reply(3,from*1000+source,which*1000+slot);
}
void rotate(int id,int direction){int which=side(id);if(showing&&which>=0)reply(6,which,direction);}
void dismiss(armyGroup* army,int slot){int which=side(army);if(showing&&which>=0&&slot>=0&&slot<7)reply(7,which*7+slot,0);}
void swapAll(){if(showing)reply(8);}
bool present(const int* values){
    if(values[0]!=13)return false;
#ifdef __EMSCRIPTEN__
    if(values[1]<0||values[1]>=game::HERO_COUNT||values[2]<0||values[2]>=game::HERO_COUNT||values[3]!=g_localGamePos){EM_ASM({window.homm3Authority.fail('Invalid hero exchange owner');});return true;}
    std::copy_n(values,64,fields);ids[0]=values[1];ids[1]=values[2];showing=true;cancelDrag();
    {swapManager manager(&g_game->m_heroes[ids[0]],&g_game->m_heroes[ids[1]]);g_executive->doDialog(&manager);}
    cancelDrag();showing=false;ids[0]=ids[1]=-1;
    EM_ASM({if(!window.homm3Authority.fatal)window.homm3Authority.answer(0);});
#endif
    return true;
}
} }
