#include "authority_puzzle_client.h"
#include "authority_client.h"
#include "authority_actions_client.h"
#include "fast_game.h"
#include "puzzlewindow.h"
#include "viewwrld.h"
#include "bitmap16.h"
#include "advmgr.h"
#include "winmgr.h"
#include "soundmgr.h"
#include "mousemgr.h"
#include "hd_view.h"
#include <algorithm>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace Port { namespace AuthorityPuzzleClient {
namespace {
void show(int alignment,int revealed){
#ifdef __EMSCRIPTEN__
    constexpr int width=592,height=544;std::vector<unsigned short> pixels(width*height);
    const int count=EM_ASM_INT({return window.homm3Authority.copyPuzzlePixels(HEAPU16,$0>>1,$1);},pixels.data(),int(pixels.size()));
    if(count!=int(pixels.size())||alignment<0||alignment>=9||revealed<0||revealed>48){EM_ASM({window.homm3Authority.fail('Invalid puzzle image');});return;}
    g_mouseManager->setPointer(0,mouseManager::ADVENTURE_SET);
    g_advManager->demobilizeCurrHero(0,1);
    {
        TPuzzleWindow window(alignment);auto sound=loadPlaySample("Obelisk.wav");
        window.updatePuzzle(1);
        g_advManager->drawAdventureMapGems();g_windowManager->updateScreen(0,0,HD::width(),HD::height());
        if(revealed)g_windowManager->saveFizzleSourceX(8,8,width,height);
        for(int y=0;y<height;++y)std::copy(pixels.begin()+y*width,pixels.begin()+(y+1)*width,g_windowManager->m_screenBitmap->GetMap(8,y+8));
        g_advManager->drawAdventureMapGems();
        if(revealed)g_windowManager->fizzleForwardX(8,8,width,height,220);
        else g_windowManager->updateScreen(8,8,width,height);
        window.doModal(0);clearMemSample(sound);
    }
    EM_ASM({window.homm3Authority.answer(0);});
    if(!g_inViewWorld){
        g_advManager->redrawAdvScreen(true,false);
        g_soundManager->switchAmbientMusic(g_terrainMusicIds[g_advManager->m_lastTerrain]);
        g_advManager->setEnvironmentOrigin(g_advManager->getMapCenter(),1);
    }
#else
    (void)alignment;(void)revealed;
#endif
}
}
void showPending(int alignment,int revealed){show(alignment,revealed);}
void request(){
#ifdef __EMSCRIPTEN__
    if(AuthorityActionsClient::worldViewActive()){
        const int spell=EM_ASM_INT({return window.homm3Authority.worldSpell();});
        AuthorityClient::repeatExtendedChoice(6,spell,1,0,0,nullptr);return;
    }
#endif
    AuthorityClient::request(40,0,0,0);
}
} }
