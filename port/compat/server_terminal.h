#pragma once
#include <iosfwd>
namespace Port { namespace Authority {
void resetTerminal();
void checkTerminal(int forceWin);
bool captureTerminalDialog(const char* text);
void writeTerminal(std::ostream& out,int actor);
int outcome(int actor);
} }
