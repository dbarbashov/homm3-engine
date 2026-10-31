#pragma once
namespace Port { namespace Authority {
bool supportsAction(int operation);
void applyAction(int operation,int actor,int object,int a,int b,int c,int d,int e);
} }
