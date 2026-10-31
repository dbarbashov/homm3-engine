#pragma once
// Values already resolved by the authority, needed by original tooltips,
// movement highlights and damage previews. No spell rules run on the client.
#include "army.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace Port { namespace Authority {
constexpr int CombatModifierWords=25;
using CombatModifiers=std::array<int32_t,CombatModifierWords>;
inline int32_t floatWord(float value){static_assert(sizeof(float)==sizeof(int32_t),"32-bit float required");int32_t word;std::memcpy(&word,&value,sizeof(word));return word;}
inline float wordFloat(int32_t word){float value;std::memcpy(&value,&word,sizeof(value));if(!std::isfinite(value)||std::abs(value)>1000)throw std::runtime_error("Invalid combat modifier");return value;}
inline CombatModifiers combatModifiers(const army& a){return {{
    a.m_blessAmount,
    a.m_curseAmount,
    a.m_antiMagicSpellLevel,
    a.m_bloodlustAmount,
    a.m_precisionAmount,
    a.m_slayerLevel,
    a.m_forgetfulnessLevel,
    a.m_defendBonus,
    a.m_residualBlindness,
    a.m_residualParalyze,
    a.m_moraleBonus,
    a.m_moralePenalty,
    a.m_luckBonus,
    a.m_luckPenalty,
    floatWord(a.m_slowFactor),
    floatWord(a.m_frenzyFactor),
    floatWord(a.m_blindFactor),
    floatWord(a.m_fireShieldStrength),
    floatWord(a.m_poisonPenalty),
    floatWord(a.m_protectionFromAirFactor),
    floatWord(a.m_protectionFromFireFactor),
    floatWord(a.m_protectionFromWaterFactor),
    floatWord(a.m_protectionFromEarthFactor),
    floatWord(a.m_shieldFactor),
    floatWord(a.m_airShieldFactor)
}};}
inline void applyCombatModifiers(army& a,const CombatModifiers& values){
    a.m_blessAmount=values[0];
    a.m_curseAmount=values[1];
    a.m_antiMagicSpellLevel=values[2];
    a.m_bloodlustAmount=values[3];
    a.m_precisionAmount=values[4];
    a.m_slayerLevel=values[5];
    a.m_forgetfulnessLevel=values[6];
    a.m_defendBonus=values[7];
    a.m_residualBlindness=values[8];
    a.m_residualParalyze=values[9];
    a.m_moraleBonus=values[10];
    a.m_moralePenalty=values[11];
    a.m_luckBonus=values[12];
    a.m_luckPenalty=values[13];
    a.m_slowFactor=wordFloat(values[14]);
    a.m_frenzyFactor=wordFloat(values[15]);
    a.m_blindFactor=wordFloat(values[16]);
    a.m_fireShieldStrength=wordFloat(values[17]);
    a.m_poisonPenalty=wordFloat(values[18]);
    a.m_protectionFromAirFactor=wordFloat(values[19]);
    a.m_protectionFromFireFactor=wordFloat(values[20]);
    a.m_protectionFromWaterFactor=wordFloat(values[21]);
    a.m_protectionFromEarthFactor=wordFloat(values[22]);
    a.m_shieldFactor=wordFloat(values[23]);
    a.m_airShieldFactor=wordFloat(values[24]);
}
} }
