#pragma once

namespace World::Lorencia
{
    // Hide courtyard clutter, pave leftover TileGrass01 cells on the stone
    // plaza (terrain layer, not objects), make the 21x21 fountain square
    // walkable, clear TW_SAFEZONE only inside the circular PvP plaza, and
    // spawn the Crywolf circular floor (Object35/Object41). No claw pillars.
    void ApplyFountainCombatPlaza();

    bool IsCombatPlazaFloor(int type);

    // True for courtyard clutter (grass/bush/fence/light) near the fountain.
    // Used at spawn-hide time and again in RenderObject so respawned meshes stay gone.
    bool ShouldHideNearFountain(int type, float worldX, float worldY);
}
