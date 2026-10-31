#pragma once
class hero;
struct type_point;
namespace Port { namespace Authority {
void hillFortEncounter(hero& who);
void sacrificeEncounter(hero& who);
void mapTavernEncounter(hero& visitor,type_point point);
} }
