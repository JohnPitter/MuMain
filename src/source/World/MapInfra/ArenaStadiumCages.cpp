#include "stdafx.h"
#include "World/MapInfra/ArenaStadiumCages.h"

#include "Render/Terrain/ZzzLodTerrain.h"
#include "World/MapInfra/MapManager.h"

namespace
{
    struct DoorRect
    {
        int x1;
        int y1;
        int x2;
        int y2;
    };

    // Mirrors OpenMU ArenaCageDoors.TerrainHoles (map 6 cage doors).
    constexpr DoorRect kCageDoors[] =
    {
        { 16, 37, 16, 39 },
        { 16, 55, 16, 57 },
        { 16, 73, 16, 75 },
        { 16, 88, 18, 93 },
        { 23, 37, 24, 39 },
        { 23, 55, 24, 57 },
        { 23, 71, 24, 74 },
        { 23, 88, 24, 91 },
        { 35, 37, 35, 39 },
        { 35, 55, 35, 58 },
        { 35, 70, 35, 74 },
        { 35, 78, 35, 84 },
        { 41, 32, 41, 34 },
        { 41, 37, 41, 42 },
        { 41, 45, 41, 50 },
        { 41, 55, 41, 61 },
        { 41, 64, 41, 68 },
        { 41, 69, 41, 74 },
        { 41, 78, 41, 84 },
        { 41, 87, 41, 93 },
    };

    void OpenDoorTiles()
    {
        for (const DoorRect& door : kCageDoors)
        {
            for (int y = door.y1; y <= door.y2; ++y)
            {
                for (int x = door.x1; x <= door.x2; ++x)
                {
                    if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                        continue;

                    SubTerrainAttribute(x, y, TW_NOMOVE | TW_WATER | TW_NOGROUND);
                }
            }
        }
    }
}

namespace World::Arena
{
    void ApplyCageDoors()
    {
        if (gMapManager.WorldActive != WD_6STADIUM)
            return;

        OpenDoorTiles();
    }
}
