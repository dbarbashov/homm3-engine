#pragma once
#include <string>
namespace Port { namespace Authority {
// Final, masked original puzzle pixels only. No map origin, grail location,
// object identity, or terrain beneath an opaque piece crosses this boundary.
std::string puzzleProjection(int actor);
void puzzleEncounter(int actor);
} }
