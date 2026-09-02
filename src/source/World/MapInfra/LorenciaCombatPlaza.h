#pragma once

namespace World::Lorencia
{
    // Hide fountain courtyard clutter, pave orphan TileGrass01 islands on
    // stone (keep grass next to chests/walls), strip leftover planter
    // frames on those empty rows, keep EncTerrain walk flags (no courtyard
    // punch), and spawn the Crywolf circular floor.
    void ApplyFountainCombatPlaza();

    bool IsCombatPlazaFloor(int type);

    // True for fountain clutter that must stay gone (fountain hedges/lights,
    // orphan grass, empty planter fences). Chest-side bushes and cercadinhos
    // on remaining lawns return false.
    bool ShouldHideNearFountain(int type, float worldX, float worldY);

    // Paved Lorencia town plaza. CheckGate must not treat these tiles as warps.
    bool IsTownPlazaTile(int tileX, int tileY);
}
