#pragma once
#include "abstractfile.h"
#include "browser_rng.h"
#include "parity_harness.h"
#include <cstring>
#include <vector>

namespace Port { namespace GameRng {
struct State { unsigned algorithm; std::uint64_t value; };
struct Extras {std::vector<unsigned char> phases;std::vector<std::uint32_t> aggression;};
inline State capture() {
#ifdef HOMM3_HARNESS
    return {Harness::rngAlgorithm(),Harness::rngState()};
#else
    return {1,BrowserRng::state()};
#endif
}
inline bool restore(State s) {
#ifdef HOMM3_HARNESS
    return Harness::restoreRng(s.algorithm,s.value);
#else
    if(s.algorithm!=1)return false;
    BrowserRng::state()=s.value;return true;
#endif
}
// Deserialization creates temporary heroes and cosmetic map phases. None of
// those draws belongs to the continuation of the loaded game, even on failure.
struct Preserve {
    State continuation=capture();
    ~Preserve(){restore(continuation);}
};
// Optional, fixed-width little-endian trailer after the original GM2 payload.
// Original saves have no trailer. The original payload remains unchanged.
inline bool write(TAbstractFile* file,const Extras* extras=nullptr) {
    unsigned char bytes[20]={'H','3','R','N','G','0','0','1'};
    if(extras)bytes[7]='3';
    const auto s=capture();bytes[8]=static_cast<unsigned char>(s.algorithm);
    for(unsigned i=0;i<8;++i)bytes[12+i]=static_cast<unsigned char>(s.value>>(8*i));
    if(file->write(bytes,sizeof(bytes))!=sizeof(bytes))return false;
    if(!extras)return true;
    if(extras->phases.size()>2000000||extras->aggression.size()>1024)return false;
    auto word=[&](std::uint32_t value){unsigned char data[4];for(unsigned i=0;i<4;++i)data[i]=static_cast<unsigned char>(value>>(8*i));return file->write(data,4)==4;};
    if(!word(extras->phases.size())||file->write(extras->phases.data(),int(extras->phases.size()))!=int(extras->phases.size())||!word(extras->aggression.size()))return false;
    for(auto value:extras->aggression)if(!word(value))return false;
    return true;
}
inline bool read(TAbstractFile* file,State& continuation,Extras* extras=nullptr) {
    unsigned char bytes[20]={};
    const int n=file->read(bytes,8);
    if(n==0)return true; // Retail/previous portable saves retain the caller's RNG.
    if(n!=8||std::memcmp(bytes,"H3RNG00",7)!=0||bytes[7]<'1'||bytes[7]>'3'||file->read(bytes+8,12)!=12)return false;
    if(bytes[9]||bytes[10]||bytes[11])return false;
    State s{bytes[8],0};for(unsigned i=0;i<8;++i)s.value|=std::uint64_t(bytes[12+i])<<(8*i);
    const auto before=capture();const bool valid=restore(s);restore(before);
    if(!valid)return false;
    Extras decoded;
    auto word=[&](std::uint32_t& value){unsigned char data[4];if(file->read(data,4)!=4)return false;value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(data[i])<<(8*i);return true;};
    if(bytes[7]>='2'){
        std::uint32_t size;if(!word(size))return false;
        if(size>2000000)return false;
        decoded.phases.resize(size);if(file->read(decoded.phases.data(),int(size))!=int(size))return false;
    }
    if(bytes[7]>='3'){
        std::uint32_t count;if(!word(count)||count>1024)return false;
        decoded.aggression.resize(count);for(auto& value:decoded.aggression)if(!word(value))return false;
    }
    continuation=s;if(extras)*extras=std::move(decoded);
    return true;
}
} }
