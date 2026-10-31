#include "kb.h"
#include "kbwin.h"
#include "soundmgr.h"
#include "palette.h"
#include "winmm_thunks.h"

// Original body extracted from kbwin.cpp
unsigned long GameTime::get()
{
    return timeGetTime();
}

// Original body extracted from kbwin.cpp
void GameTime::delayTil(unsigned long time)
{
    while (!GameTime::isPast(time)) {
        process1WindowsMessage();
        pollSound();
    }
}

// Original body extracted from kbwin.cpp
void GameTime::delay(int interval)
{
    GameTime::delayTil(GameTime::get() + interval);
}

// Original body extracted from kbwin.cpp
void initVideo()
{
}

// Original body extracted from wingraph.cpp
void setPlayerPaletteColors(unsigned short* pal, int whichPlayer)
{
    memcpy(pal + 224, &g_playerPalette->Palette[whichPlayer * 32],
           32 * sizeof(unsigned short));
}

// Original body extracted from wingraph.cpp
void setPlayerPaletteColors(TPalette24& pal, int whichPlayer)
{
    memcpy(pal.Palette + 224 * 3, g_playerPalette24->Palette + whichPlayer * 32 * 3,
           32 * 3);
}
