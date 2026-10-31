#pragma once
class armyGroup;
class type_sacrifice_window;
namespace Port { namespace AuthorityClient {
// Called only from the adventure dispatcher while its modal guard is held.
// Returns true when this helper owns the choice and its final reply.
bool presentExtendedChoice(const int* fields,const char* text="");
// Call only while an original modal owns the adventure choice guard.
bool repeatExtendedChoice(int kind,int hero,int value,int a,int b,int* fields64);
bool armyExchangeActive();
bool chooseArmyTransfer(armyGroup* source,int src,armyGroup* target,int dst,int count,int mode);
bool chooseArmyDismiss(armyGroup* group,int slot);
bool chooseArmySwap();
bool hillFortChoiceActive();
void chooseHillFort(int slot);
bool sacrificeChoiceActive();
void chooseSacrifice(type_sacrifice_window* window);
bool mapTavernChoiceActive();
void chooseMapTavernGuild();
bool marketChoiceActive();
void chooseMarket(int kind,int units,int packed);
} }
