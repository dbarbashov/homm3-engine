#pragma once
// A bounded journal of facts already resolved by the server. These records
// drive original animation routines; they are never executable rule commands.
#include <array>
class army;
namespace Port { namespace Authority {
enum CombatVisual { VisualWalk=1, VisualFly, VisualStrike, VisualShot,
                    VisualDamage, VisualSpell, VisualAction, VisualEffect, VisualSpellSound,
                    VisualMorale, VisualLuck, VisualImpact, VisualRound, VisualCycleTimers, VisualPause, VisualTurn, VisualFlightEnd, VisualSpellMessage };
constexpr int CombatVisualWords=12;
constexpr int CombatVisualLimit=256;
using CombatVisualRecord=std::array<int,CombatVisualWords>;
#ifdef HOMM3_SERVER
// Walk/fly replay already includes its own turns. Only journal turns outside
// those compound animations, so a replay never turns the same stack twice.
inline thread_local int compoundTurnDepth=0;
struct CompoundTurnScope {
    CompoundTurnScope(){++compoundTurnDepth;}
    ~CompoundTurnScope(){--compoundTurnDepth;}
    CompoundTurnScope(const CompoundTurnScope&)=delete;
};
void recordCombatVisual(CombatVisual kind,const army* stack,
                        int a=0,int b=0,int c=0,int d=0,int e=0,int f=0);
#endif
} }
