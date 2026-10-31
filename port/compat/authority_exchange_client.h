#pragma once
class armyGroup;
namespace Port { namespace AuthorityExchangeClient {
bool active();
bool present(const int* fields);
bool canModify(int heroId);
bool canReceive(int heroId);
bool transfer(armyGroup* source,int src,armyGroup* target,int dst,int count,int mode);
void artifactClick(int heroId,int slot,bool rightClick);
void rotate(int heroId,int direction);
void dismiss(armyGroup* army,int slot);
void swapAll();
void cancelDrag();
} }
