#pragma once

#include "Core/Globals/_struct.h"

namespace Character::Equipment
{
    ITEM_ATTRIBUTE DisplayRequirements(int itemType);
    bool MatchesDisplayedClass(int itemType, int baseClass, int classStep);
    unsigned DisplayLevel(int itemType, unsigned legacyLevel);
}
