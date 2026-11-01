#pragma once
#include <cstdint>

// Match rand()/srand() in the pinned Emscripten musl libc. Native libc differs
// between macOS and Linux; it cannot be the RNG of a server hosting WASM games.
// Use the same explicit, serializable state in browsers and native builds.
namespace Port { namespace BrowserRng {
inline std::uint64_t& state() { static std::uint64_t value=0; return value; }
inline void seed(unsigned value) { state()=static_cast<unsigned>(value-1u); }
inline int next() {
    state()=UINT64_C(6364136223846793005)*state()+1;
    return static_cast<int>(state()>>33);
}
} }

// Presentation must never advance the rules stream. This stream is deliberately
// independent of save/load and the number of UI frames on another client.
namespace Port { namespace CosmeticRng {
// Frame-based cosmetic choices are relative to the latest presentation
// boundary, never to process uptime or transport latency.
inline std::uint32_t& clockEpoch(){static std::uint32_t value=0;return value;}
inline std::uint64_t& state() { static std::uint64_t value=UINT64_C(0x6a09e667f3bcc909); return value; }
inline void seed(unsigned value) { state()=static_cast<unsigned>(value-1u); }
inline int next() { state()=UINT64_C(6364136223846793005)*state()+1; return static_cast<int>(state()>>33); }
inline int range(int lower,int upper) { return upper<=lower?lower:lower+next()%(upper-lower+1); }
} }
