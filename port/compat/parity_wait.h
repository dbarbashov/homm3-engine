#pragma once
#include "parity_harness.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
namespace Port { namespace Harness {
// Preserve production wall-clock waits. Instrumented IPC waits must also
// advance/yield the virtual clock, even while another seat owns a dialog.
inline void wait(unsigned ms) {
#ifdef HOMM3_HARNESS
    if (accelerated()) { pump(ms); return; }
#endif
    emscripten_sleep(ms);
}
} }
#endif
