#pragma once
#include <cstdint>
#include <cstddef>
#include <iosfwd>
// Versioned field protocol: every element is a signed 32-bit integer. Unsigned
// values use their 32-bit two's-complement representation, never native long.
// Header: schema, actor, activeActor, day, week, month, width, layers.
// Player: resources[7], recruits[2],gardenFlags,springFlags,tombFlags,leanToFlags,
// fieldHeroCount,orderedHeroIds,townCount,orderedTownIds. Heroes: count then records below.
// Hero: id, detail(0 visible,1 owned,2 tavern offer or consented exchange), x,y,z,valid,owner,facing,portrait; private record follows:
// mana,maxMove,move,xp,level,formation,flags,visitFlags[14],dWalkSpellsCast,
// townSpecialGrantedMask[2],stats[4],skills[28],skillOrder[28],
// visitFlags order: training,defense,garden,merc,power,tree,library,arena,
// magicSchool,warSchool,university,shrine1,shrine2,shrine3.
// skillCount,army(types[7],counts[7]),equipped[(id,extra)*19],
// backpack[(id,extra)*64],backpackCount,spellbook[70],availableSpells[70],
// class,sex,hasCustomName,customName(byteLength,bytes),name[13],flight,waterWalk,
// disguise,identify,morale,luck,sleeping,artifactSlotCounts[15],
// obscured(x,y,z,type,wasTrigger,extra).
// Town: count then id,private,owner,x,y,z,type; private:built,mageLevel,
// garrisonHero,visitingHero,population[14],army[14],populationMask[2],
// buildingMask[2],legalMask[2],mageSpells[30],mageSpellCounts[5],name(length,bytes),
// generatorBonus[14],manaVortexFull,pondAmount,pondResource,summoningType,population.
// Side pools, each totalSlots,knownCount and explicit id records:
// mines(owner,type,abandoned,x,y,z,army14), dwellings(owner,class,type,x,y,z,
// townId,creatures4,population4,army14), garrisons(owner,removable,x,y,z,army14),
// boats(allocated,type,facing,owner,occupyingHero,occupied,x,y,z,valid,
// obscuredX,Y,Z,type,trigger,extra). Only owned or revealed entries are emitted.
// Templates: totalSlots,visibleCount then id,imageName(length,bytes),width,height,
// drawMask[2],passableMask[2],shadowMask[2],triggerMask[2],terrainMask,type,extra,
// suppressDraw,objectTypeIndex. Sprite names/masks are authoritative: randomized
// server templates need not have the same indices as the local bootstrap map.
// Objects: totalSlots, visibleCount then id,x,y,z,typeIndex,animation,extra.
// Cells: for each z/y/x: visibilityFlags. 0: fully hidden, no data;
// 1/257: revealed (257 also has adjacent monster), then type,extra,index,
// typeIndex,flags,refCount,(objectIndex,offsets,layer)*refCount;
// 2: hidden shroud edge, then refCount,(objectIndex,offsets,layer)*refCount.
// Edge refs only reference objects already known from revealed cells; no
// hidden cell gameplay state, new objects or visibility bits are disclosed.
// Private player header additionally carries fieldHeroCount + ordered fieldHeroIds,
// townCount + ordered townIds. Owned garrison heroes are projected privately but
// do not consume a field-hero roster slot.
// Quest UI: seerCount,guardCount then for every seer and guard: activeForActor,
// quickInfoText,rolloverText,journalText; texts use length + original byte words.
// All texts come from original per-player methods; unknown quests expose only
// generic object labels. No virtual quest object or other-player flags cross.
// Obelisks: 48 boolean visitation flags for the viewer only.
// Visibility uses original explored/shroud bits (not a newly invented sight rule).
namespace Port { namespace Authority {
constexpr int ProjectionSchema = 1;
constexpr int ProjectionMaxWords = 4000000;
void writeProjection(std::ostream& out, int actor, std::uint64_t revision);
} }

struct AuthorityProjectionAccess {
    static void write(std::ostream&, int, std::uint64_t);
    static bool read(const std::int32_t*, std::size_t, bool);
};
