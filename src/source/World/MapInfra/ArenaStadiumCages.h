#pragma once

namespace World::Arena
{
    // Stadium (map 6) hunting cages: fence rails = NOMOVE, one official door
    // per pit. Server C1-46 mirrors this. Does not touch Webzen sentinel tiles.
    void ApplyCageDoors();
}
