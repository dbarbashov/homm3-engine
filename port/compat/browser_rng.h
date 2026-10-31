#pragma once
#include <cstdint>

// Match rand()/srand() in the pinned Emscripten musl libc. Native libc differs
// between macOS and Linux; it cannot be the RNG of a server hosting WASM games.
// The browser's existing generator and legacy games are deliberately unchanged.
namespace Port { namespace BrowserRng {
inline std::uint64_t& state() { static std::uint64_t value=0; return value; }
inline void seed(unsigned value) { state()=static_cast<unsigned>(value-1u); }
inline int next() {
    state()=UINT64_C(6364136223846793005)*state()+1;
    return static_cast<int>(state()>>33);
}
} }
