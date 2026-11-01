#pragma once
struct message;
class army;
struct AuthorityBattleAccess;
namespace Port { namespace BattleClient {
bool active();
void settleDeath(army&);
bool begin();
void project();
void installOpening();
void restoreOpeningCycles();
int main(message&);
} }
