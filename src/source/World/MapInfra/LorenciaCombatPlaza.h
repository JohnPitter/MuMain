#pragma once

namespace World::Lorencia
{
    // Lorencia plaza/fountain combat leftover is gone. EncTerrain walk and
    // safezone stay official. Stubs keep MapManager / RenderObject compiling.
    void ApplyFountainCombatPlaza();

    bool IsCombatPlazaFloor(int type);

    bool ShouldHideNearFountain(int type, float worldX, float worldY);

    bool IsTownPlazaTile(int tileX, int tileY);
}
