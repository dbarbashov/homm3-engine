#pragma once
#include "authority_guild_model.h"
namespace Port { namespace Authority {
// Internal rule queries; tier is derived from owned towns or the visited den.
Guild::View guildView(int actor,int tier);
void guildEncounter(int actor,bool den=false);
void townGuildEncounter(int actor,int townId);
} }
