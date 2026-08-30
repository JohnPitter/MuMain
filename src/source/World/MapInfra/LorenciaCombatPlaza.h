#pragma once

namespace World::Lorencia
{
    // Hide the town fountain, clear TW_SAFEZONE on the plaza, and spawn a
    // Crywolf-style circular claw platform (Object34/Object50) so players can
    // fight there. Client-side only — server TerrainData must match.
    void ApplyFountainCombatPlaza();
}
