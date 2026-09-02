#include "stdafx.h"
#include "World/MapInfra/LorenciaCombatPlaza.h"

namespace World::Lorencia
{
    bool IsCombatPlazaFloor(int)
    {
        return false;
    }

    bool IsTownPlazaTile(int, int)
    {
        return false;
    }

    bool ShouldHideNearFountain(int, float, float)
    {
        return false;
    }

    void ApplyFountainCombatPlaza()
    {
    }
}
