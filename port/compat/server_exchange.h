#pragma once
class hero;
namespace Port { namespace Authority {
void heroExchange(hero& left,hero& right);
bool exchangeCanInspectHero(int actor,int heroId);
bool exchangeAllowsPair(int firstHero,int secondHero);
} }
