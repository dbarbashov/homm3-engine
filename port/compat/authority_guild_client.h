#pragma once
#include "authority_guild_model.h"
class hero;
class armyGroup;
namespace Port { namespace AuthorityGuildClient {
bool active();
void install(const Guild::View& view);
void clear();
void showPending();
void requestTown(int townId);
void requestTavern();
int tier();
int playerCount();
bool disabled(int player);
void rankStats(int category,long* values,signed char* players);
hero* bestHero(int player);
const char* heroName(int column);
int personality(int player);
int creature(int player);
void creatureDisplay(int player,armyGroup& army,int& slot);
} }
