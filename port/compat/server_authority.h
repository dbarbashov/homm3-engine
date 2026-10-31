#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <iosfwd>
namespace Port { namespace Authority {
// Only the supervisor supplies actor IDs. No browser may import world state.
class Rejected : public std::runtime_error { public: using std::runtime_error::runtime_error; };
enum class Phase { Adventure, Choice, Combat, Ai, Finished };
struct Reply { int value=0, a=0, b=0; };
struct Choice {
    uint64_t token=0;
    int actor=-1;
    std::string kind, text;
    std::vector<int> options;
    // Server-produced JSON object, never copied from a client request.
    std::string metadata="{}";
    // Optional typed argument validator over server-owned suspended state.
    // Default decisions have no auxiliary arguments.
    std::function<bool(const Reply&)> validate;
};
struct Presentation { std::string kind;int actor=-1,id=-1,other=-1;std::string text; };
void present(const char* kind,int actor,int id,int other=-1,const char* text="");
std::vector<Presentation> takePresentations();
void adventureSound(const char* name);
void writeAdventureSounds(std::ostream& out,int actor);
using ChoiceHandler=std::function<Reply(const Choice&)>;
void setChoiceHandler(ChoiceHandler handler);
void setTurnHandler(std::function<void()> handler);
Reply choose(Choice choice);
bool active();
int activeActor();
Phase phase();
const char* phaseName();
void setPhase(Phase value);
uint64_t revision();
void start();
void apply(int operation,int actor,int object,int a,int b,int c=0,int d=0,int e=0);
void advanceTurn();
void beginTurn();
void shipyardChoice(int townType);
void normalChoice(const char* text,int mbType,int r1,int e1,int r2,int e2,int special,int r3,int e3,int x=-1,int y=-1);
void levelChoice(int actor,int heroId,int primary,int skill1,int skill2);
// Preserve the opt-in prototype and its tests independently of ordinary policy.
void prototypeStart();
void prototypeApply(int operation,int actor,int object,int a,int b);
int day();
} }
