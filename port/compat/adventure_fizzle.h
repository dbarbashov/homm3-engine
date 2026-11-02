#pragma once
#include "advmgr.h"
#include "mousemgr.h"
#include "winmgr.h"
#include "soundmgr.h"
#include "parity_harness.h"

namespace Port {
// Original fizzleCenter presentation. The authority renderer captures the old
// map before installing its projection; legacy captures it here before redraw.
inline void showAdventureFizzle(const char* name,bool sourceSaved=false){
#ifdef HOMM3_HARNESS
    Harness::decision("adventure-fizzle-start",0,0);
#endif
    launchSample(name,-1,3);
    g_mouseManager->hidePointer();
    if(!sourceSaved)g_windowManager->saveFizzleSourceX(0xc0,0xa0,0xe0,0xe0);
    g_advManager->completeDraw(false);
    g_windowManager->fizzleForwardX(0xc0,0xa0,0xe0,0xe0,0x41);
    g_mouseManager->showPointer(false);
#ifdef HOMM3_HARNESS
    Harness::decision("adventure-fizzle-end",0,0);
#endif
}
}
