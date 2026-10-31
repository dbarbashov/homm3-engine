#pragma once
// Implemented beside the original marketplace's file-local globals.
namespace Port { namespace AuthorityMarketClient {
void show(int heroId,int townId,int source,int efficiency,int window,const int* inventory);
void refresh(const int* inventory);
} }
