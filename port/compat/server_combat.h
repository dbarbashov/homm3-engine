#pragma once
#include <string>
#include <functional>
#include <vector>
struct type_artifact;
class combatManager;
namespace Port { namespace Authority {
// Internal synchronous boundary. The callback may wait indefinitely for a human
// answer; the original adventure/combat stack remains alive in this worker.
void runCombat(combatManager& combat);
// One private result event per human participant, before level-up choices.
using CombatResultHandler = std::function<void(int, const std::string&)>;
void setCombatResultHandler(CombatResultHandler handler);
void setCombatStateHandler(CombatResultHandler handler);
void publishCombatResult(combatManager& combat, int experience,
                         const std::vector<type_artifact>& lootedArtifacts);
// Private per-participant observer snapshot. Empty for nonparticipants; never
// includes legal actions or another player's hidden spells/artifacts/mines.
std::string combatProjection(int actor);
} }
