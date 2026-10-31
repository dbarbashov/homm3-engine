#include "authority_quest_client.h"
#include "authority_client.h"
#include "game.h"
#include <vector>
namespace Port { namespace AuthorityQuestClient {
namespace {
struct Row {bool active=false;std::string text[3];};
std::vector<Row> seers,guards;
template<class T> int index(const T* object,const std::vector<T>& values){
    if(values.empty())return -1;
    const auto start=reinterpret_cast<uintptr_t>(values.data()),address=reinterpret_cast<uintptr_t>(object);
    if(address<start||address-start>=values.size()*sizeof(T)||(address-start)%sizeof(T))return -1;
    return int((address-start)/sizeof(T));
}
std::string get(const std::vector<Row>& rows,int id,int kind){return id>=0&&size_t(id)<rows.size()&&kind>=0&&kind<3?rows[id].text[kind]:std::string();}
bool visible(const std::vector<Row>& rows,int id,int player){return player==g_localGamePos&&id>=0&&size_t(id)<rows.size()&&rows[id].active;}
}
bool enabled(){return AuthorityClient::enabled();}
void reset(int seerCount,int guardCount){seers.assign(seerCount,Row());guards.assign(guardCount,Row());}
void row(bool guard,int id,bool active,std::string quick,std::string rollover,std::string journal){
    auto& rows=guard?guards:seers;if(id<0||size_t(id)>=rows.size())return;
    rows[id].active=active;rows[id].text[0]=std::move(quick);rows[id].text[1]=std::move(rollover);rows[id].text[2]=std::move(journal);
}
bool active(const TSeerHut* object,int player){return visible(seers,index(object,g_game->m_worldMap.m_seerHutList),player);}
bool active(const TQuestGuard* object,int player){return visible(guards,index(object,g_game->m_worldMap.m_questGuardList),player);}
std::string text(const TSeerHut* object,int kind){return get(seers,index(object,g_game->m_worldMap.m_seerHutList),kind);}
std::string text(const TQuestGuard* object,int kind){return get(guards,index(object,g_game->m_worldMap.m_questGuardList),kind);}
} }
