#pragma once
#include <string>
#include <vector>
class armyGroup;
class NewmapCell;
namespace Port { namespace AuthorityActionsClient {
bool enabled();
bool castSpell(int spell);
int armyId(armyGroup* army);
bool transfer(armyGroup* source,int src,armyGroup* target,int dst,int count,int mode);
void split(armyGroup* source,int src,armyGroup* target,int dst,int destinationCount);
void artifactClick(int heroId,int slot,bool rightMouse);
void cancelArtifactDrag();
void present(const std::string& kind,int id,int other);
void showWorld(int spell,int level,const std::vector<int>& markers);
bool worldViewActive();
NewmapCell* worldViewCell(int x,int y,int z);
int worldViewOwner(NewmapCell* cell);
} }
