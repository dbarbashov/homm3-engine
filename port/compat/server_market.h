#pragma once
#include "artifact.h"
class hero;
namespace Port { namespace Authority {
void marketEncounter(hero* who,int source,int efficiency,int window,TArtifact* inventory,int townId=-1);
void townMarket(int actor,int townId,int window);
} }
