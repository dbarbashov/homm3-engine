#pragma once
class hero;
class armyGroup;
struct type_university;
namespace Port { namespace Authority {
// These calls retain the original event's stack and population pointers while
// waiting for authenticated replies. A reply never provides prices or state.
void recruitEncounter(hero& who,const int creatures[4],short* const available[4],bool viewOnly=false,bool warMachines=false);
void universityEncounter(hero& who,const type_university& university,bool townUniversity=false);
void armyEncounter(hero& who,armyGroup& other,bool canTake,bool canGive,const char* window="join",int flags=0,int otherOwner=-1);
void worldViewEncounter(hero& who,int spell,int level);
} }
