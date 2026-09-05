#pragma once

namespace World::Arena
{
    // Stadium (map 6): the shipped EncTerrain7.att is the official Webzen
    // authoring (fence rails = NOMOVE where the .obj shows rails; every visual
    // gap stays walkable). At load we only punch the official cage doors open
    // and paint the safezone pads (plaza 65,43 r=10 + warp 102,116 r=3),
    // mirroring OpenMU ArenaCageDoors.
    void ApplyCageDoors();
}
