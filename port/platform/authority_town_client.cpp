#include "authority_town_client.h"
#include "authority_actions_client.h"
#include "authority_client.h"
#include "server_town_actions.h"
#include "game.h"
#include "hero.h"
#include "town.h"
#include "sacrifice_window.h"
namespace Port { namespace AuthorityTownClient {
void transformer(armyGroup* original){
    if(!original)return;const armyGroup before=*original;armyGroup local=before;
    {type_skeleton_window window(&local);window.doModal(0);}
    // The original transformer moves whole stacks between its two lists. Map
    // the selected quantities back to original slots; transmit only a bounded
    // selection mask, never the editable working army or its final contents.
    int selected=0;
    for(int slot=0;slot<7;++slot){
        const int type=before.m_armies[slot];if(type<0||type>=145||authorityDeathCreature(type)==type)continue;
        bool first=true;for(int i=0;i<slot;++i)if(before.m_armies[i]==type)first=false;if(!first)continue;
        const long amount=before.getCreatureTotal(TCreatureType(type))-local.getCreatureTotal(TCreatureType(type));if(amount<=0)continue;
        for(int mask=1;mask<128;++mask){long total=0;bool valid=true;for(int i=0;i<7;++i)if(mask&(1<<i)){if(before.m_armies[i]!=type){valid=false;break;}total+=before.m_numTroops[i];}
            if(valid&&total==amount){selected|=mask;break;}
        }
    }
    if(selected)AuthorityClient::request(36,AuthorityActionsClient::armyId(original),selected);
}
bool recruitPortal(armyGroup* army,short* population,int count){
    if(!AuthorityClient::enabled())return false;
    for(auto& t:g_game->m_towns)if(t.m_owner==g_localGamePos&&&t.getArmy()==army&&&t.m_summoningPopulation==population){AuthorityClient::request(38,t.m_id,count);return true;}
    return false;
}
} }
