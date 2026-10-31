#pragma once
#include "authority_choices.h"
class armyGroup;
namespace Port { namespace AuthorityClient {
bool enabled();
bool modalInput();
bool recruitChoiceActive();
void chooseRecruit(int creature,int count);
bool universityChoiceActive();
void chooseUniversity(int skill);
void captureSetup();
void begin();
void tick();
bool turnView(bool force,bool draw,bool update);
bool animateTurnView(bool background);
void finishMovementPresentation();
bool requestMove(int hero,int direction,bool standEnd);
bool request(int op,int id=0,int a=0,int b=0,int c=0,int d=0,int e=0);
bool requestRecruit(armyGroup*,short*,int creature,int count);
} }
