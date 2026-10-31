// WorldDirections.h - the nine compass names of ArrayTxt.txt's cDirections
// group, and the part of the map a location lies in (WorldDirections.cpp,
// name inferred: the image's last project object, after
// WitchHutPropsDlg.cpp; GOG only). The game names a quest's monster the
// same way (seerhut.cpp's type_monster_quest::setDefaultText): the map's
// thirds across and down, north first, the centre last.
#ifndef HOMM3_EDITOR_WORLDDIRECTIONS_H
#define HOMM3_EDITOR_WORLDDIRECTIONS_H

#include "Point.h"

enum TWorldDirection {
    eDirectionNorth,
    eDirectionNorthEast,
    eDirectionEast,
    eDirectionSouthEast,
    eDirectionSouth,
    eDirectionSouthWest,
    eDirectionWest,
    eDirectionNorthWest,
    eDirectionCentre,
    kNumWorldDirections
};

extern const char** akWorldDirectionNames;

void InitializeWorldDirectionNamesTable();
TWorldDirection getWorldDirection(const TTilePoint& loc, const TTilePoint& worldSize);

#endif  /* HOMM3_EDITOR_WORLDDIRECTIONS_H */
