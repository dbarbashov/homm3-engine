#include "game_rng.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
struct MemoryFile : TAbstractFile {
    std::vector<unsigned char> bytes;size_t cursor=0;
    int read(void* data,int size) override {size=std::min(size,int(bytes.size()-cursor));std::memcpy(data,bytes.data()+cursor,size);cursor+=size;return size;}
    int write(const void* data,int size) override {auto p=static_cast<const unsigned char*>(data);bytes.insert(bytes.end(),p,p+size);return size;}
};
int main(){
    using namespace Port;
    // Independently obtained from the pinned Emscripten libc, not this helper.
    const unsigned seeds[]={0,1,481516,0xffffffffu};
    const int vectors[][6]={{2049033599,2025915578,1407788582,829275321,1509453734,681666918},{0,740882966,1616430695,1708849955,1669437588,406334850},{1983113215,410805805,572518304,1376329812,483001185,1187997980},{1308150633,1150367849,1315369323,868687688,625072825,811264014}};
    for(int s=0;s<4;++s){BrowserRng::seed(seeds[s]);for(int i=0;i<6;++i)assert(BrowserRng::next()==vectors[s][i]);}
    BrowserRng::seed(481516);const auto original=GameRng::capture();
    for(int i=0;i<10000;++i){CosmeticRng::seed(i);CosmeticRng::range(0,111);}
    assert(GameRng::capture().value==original.value);
    MemoryFile save;assert(GameRng::write(&save));assert(GameRng::capture().value==original.value);
    const auto expected=BrowserRng::next();BrowserRng::seed(123);
    {GameRng::Preserve load;for(int i=0;i<10000;++i)BrowserRng::next();assert(GameRng::read(&save,load.continuation));}
    assert(BrowserRng::next()==expected);
    // Missing trailers are old saves; malformed/truncated new ones fail without
    // changing the continuation. All 19 possible truncation boundaries matter.
    for(size_t n=0;n<save.bytes.size();++n){MemoryFile broken;broken.bytes.assign(save.bytes.begin(),save.bytes.begin()+n);auto state=original;assert(GameRng::read(&broken,state)==(n==0));assert(state.value==original.value);}
    for(int offset:{0,7,8,9,10,11}){MemoryFile broken;broken.bytes=save.bytes;broken.bytes[offset]=255;auto state=original;assert(!GameRng::read(&broken,state));assert(state.value==original.value);}
    BrowserRng::state()=UINT64_C(0xfedcba9876543210);MemoryFile high;assert(GameRng::write(&high));auto state=original;assert(GameRng::read(&high,state));assert(state.value==UINT64_C(0xfedcba9876543210));
    const auto before=GameRng::capture();{GameRng::Preserve failedLoad;BrowserRng::seed(42);BrowserRng::next();}assert(GameRng::capture().value==before.value);
    GameRng::Extras extras;extras.phases={0,1,127,128,254,255};extras.aggression={0x3f800000,0x3f400000};MemoryFile animated;assert(GameRng::write(&animated,&extras));GameRng::Extras decoded;assert(GameRng::read(&animated,state,&decoded));assert(decoded.phases==extras.phases&&decoded.aggression==extras.aggression);
    for(size_t n=20;n<animated.bytes.size();++n){MemoryFile broken;broken.bytes.assign(animated.bytes.begin(),animated.bytes.begin()+n);auto state=original;assert(!GameRng::read(&broken,state,&decoded));assert(state.value==original.value);}
    std::puts("PASS: musl vectors, cosmetic isolation, exact save continuation, old saves, truncated/invalid trailers, uint64 state and failed-load rollback");
}
