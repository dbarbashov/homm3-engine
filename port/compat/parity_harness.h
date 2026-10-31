#pragma once
#include <cstdint>
class Bitmap16Bit;
namespace Port { namespace Harness {
#ifdef HOMM3_HARNESS
uint32_t ticks();
bool accelerated();
void pump(unsigned milliseconds=8);
void frame(const Bitmap16Bit& bitmap);
bool loadFixture();
void exportFixture(const char* path);
void checkpoint(const char* label);
void decision(const char* kind,int side,int slot);
int random(int lower=0,int upper=32767);
void seed(unsigned value);
#else
inline bool accelerated() { return false; }
inline void frame(const Bitmap16Bit&) {}
inline bool loadFixture() { return false; }
inline void checkpoint(const char*) {}
#endif
} }
