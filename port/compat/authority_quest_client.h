#pragma once
#include <string>
class TSeerHut;
class TQuestGuard;
namespace Port { namespace AuthorityQuestClient {
bool enabled();
void reset(int seers,int guards);
void row(bool guard,int index,bool active,std::string quick,std::string rollover,std::string journal);
bool active(const TSeerHut*,int player);
bool active(const TQuestGuard*,int player);
// Text kinds:0 quick information,1 adventure rollover,2 quest journal.
std::string text(const TSeerHut*,int kind);
std::string text(const TQuestGuard*,int kind);
} }
