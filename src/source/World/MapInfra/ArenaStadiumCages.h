#pragma once

namespace World::Arena
{
    // Stadium (map 6) hunting cages: open the door tiles in client ATT so players
    // can walk into fenced pits. Server C1-46 mirrors this; fences stay solid.
    void ApplyCageDoors();
}
