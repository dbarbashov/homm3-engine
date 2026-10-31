#pragma once
// The original presentation blocks, shared by legacy rules and resolved replay.
// No random draws, damage, morale checks or action selection happen here.
#include "army.h"
#include "cmbtmgr.h"
#include "combatwindow.h"
#include "soundmgr.h"
#include "kb.h"
#include "textresource.h"
#include "parity_harness.h"
namespace Port {
inline void feedbackMarker(const char* name,army* stack){
#ifdef HOMM3_HARNESS
    Harness::decision(name,stack->group,stack->index);
#else
    (void)name;(void)stack;
#endif
}
inline void showMorale(army* stack,bool good){
    auto& c=*g_combatManager;if(c.suppressCombatPresentation())return;
    feedbackMarker(good?"morale-good-start":"morale-bad-start",stack);
    SAMPLE2 sample=loadPlaySample(good?"GoodMrle.wav":"BadMrle.wav");
    if(good)c.spellEffect(20,stack,100,0);
    sprintf(g_text,g_generalText->GetText(good?GENERAL_TEXT_GOOD_MORALE_FORMAT:GENERAL_TEXT_BAD_MORALE_FORMAT),stack->getName());
    c.m_combatWindow->combatMessage(g_text,1,0);
    if(!good)c.spellEffect(30,stack,100,1);
    waitEndSample(sample,-1);
    feedbackMarker(good?"morale-good-end":"morale-bad-end",stack);
}
inline void showLuck(army* stack){
    auto& c=*g_combatManager;if(c.suppressCombatPresentation())return;
    feedbackMarker("luck-good-start",stack);
    launchSample("goodluck.82m",-1,3);
    sprintf(g_text,g_generalText->GetText(GENERAL_TEXT_GOOD_LUCK_FORMAT),stack->getName());
    c.m_combatWindow->combatMessage(g_text,1,0);
    c.spellEffect(eSpellEffectFortune,stack,100,0);
    feedbackMarker("luck-good-end",stack);
}
}
