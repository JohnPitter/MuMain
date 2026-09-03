#include "stdafx.h"
#include "World/MapInfra/ArenaStadiumCages.h"

#include "Render/Terrain/ZzzLodTerrain.h"
#include "World/MapInfra/MapManager.h"

namespace
{
    struct TileRect
    {
        int x1;
        int y1;
        int x2;
        int y2;
    };

    // Mirrors OpenMU ArenaCageDoors.TerrainHoles (one official door per cage).
    constexpr TileRect kCageDoors[] =
    {
        { 16, 37, 16, 39 },
        { 16, 55, 16, 57 },
        { 16, 73, 16, 75 },
        { 16, 89, 18, 92 },
        { 23, 37, 24, 39 },
        { 23, 55, 24, 57 },
        { 23, 71, 24, 74 },
        { 23, 88, 24, 91 },
        { 41, 37, 41, 39 },
        { 41, 55, 41, 58 },
        { 41, 69, 41, 72 },
        { 41, 79, 41, 82 },
        { 60, 73, 64, 75 },
    };

    // Mirrors OpenMU ArenaCageDoors.FenceSeals (visual rails + over-punched back walls).
    constexpr TileRect kFenceSeals[] =
    {
        { 35, 33, 35, 45 },
        { 35, 51, 35, 63 },
        { 35, 69, 35, 97 },
        { 36, 33, 36, 45 },
        { 36, 51, 36, 63 },
        { 36, 69, 36, 81 },
        { 36, 85, 36, 97 },
        { 41, 32, 41, 36 },
        { 41, 40, 41, 54 },
        { 41, 59, 41, 68 },
        { 41, 73, 41, 78 },
        { 41, 83, 41, 93 },
        { 54, 33, 54, 45 },
        { 54, 51, 54, 63 },
        { 54, 68, 54, 97 },
        { 16, 88, 18, 88 },
        { 16, 93, 18, 93 },
        { 60, 69, 63, 72 },
        { 60, 76, 63, 88 },
        { 61, 62, 61, 73 },
        { 64, 69, 71, 69 },
        { 64, 81, 71, 81 },
        { 7, 34, 7, 43 },
        { 7, 52, 7, 61 },
        { 6, 70, 7, 79 },
        { 6, 87, 7, 91 },
        { 8, 95, 15, 96 },
    };

    constexpr TileRect kHuntingBoxes[] =
    {
        { 9, 35, 15, 41 },
        { 9, 53, 15, 59 },
        { 9, 71, 15, 77 },
        { 9, 88, 15, 94 },
        { 25, 35, 31, 41 },
        { 25, 53, 31, 59 },
        { 25, 70, 31, 76 },
        { 25, 87, 31, 93 },
        { 45, 35, 51, 41 },
        { 45, 54, 51, 60 },
        { 45, 69, 51, 74 },
        { 45, 78, 51, 84 },
        { 65, 71, 71, 77 },
    };

    constexpr TileRect kPlazaCampus[] =
    {
        { 0, 25, 120, 135 },
    };

    bool InRectList(const TileRect* rects, int count, int x, int y)
    {
        for (int i = 0; i < count; ++i)
        {
            const TileRect& r = rects[i];
            if (x >= r.x1 && x <= r.x2 && y >= r.y1 && y <= r.y2)
                return true;
        }
        return false;
    }

    bool InDoor(int x, int y)
    {
        for (const TileRect& door : kCageDoors)
        {
            if (x >= door.x1 && x <= door.x2 && y >= door.y1 && y <= door.y2)
                return true;
        }
        return false;
    }

    void ApplyRect(const TileRect& rect, bool seal)
    {
        for (int y = rect.y1; y <= rect.y2; ++y)
        {
            for (int x = rect.x1; x <= rect.x2; ++x)
            {
                if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                    continue;
                if (seal)
                {
                    if (InDoor(x, y))
                        continue;
                    AddTerrainAttribute(x, y, TW_NOMOVE);
                }
                else
                {
                    SubTerrainAttribute(x, y, TW_NOMOVE | TW_WATER | TW_NOGROUND);
                }
            }
        }
    }

    void SealFenceTiles()
    {
        for (const TileRect& seal : kFenceSeals)
            ApplyRect(seal, true);
    }

    void OpenDoorTiles()
    {
        for (const TileRect& door : kCageDoors)
            ApplyRect(door, false);
    }

    void ApplyPlazaSafe()
    {
        constexpr int kBoxCount = (int)(sizeof(kHuntingBoxes) / sizeof(kHuntingBoxes[0]));
        for (const TileRect& campus : kPlazaCampus)
        {
            for (int y = campus.y1; y <= campus.y2; ++y)
            {
                for (int x = campus.x1; x <= campus.x2; ++x)
                {
                    if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                        continue;
                    if (InDoor(x, y) || InRectList(kHuntingBoxes, kBoxCount, x, y))
                        continue;
                    AddTerrainAttribute(x, y, TW_SAFEZONE);
                }
            }
        }

        for (const TileRect& box : kHuntingBoxes)
        {
            for (int y = box.y1; y <= box.y2; ++y)
            {
                for (int x = box.x1; x <= box.x2; ++x)
                {
                    if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                        continue;
                    SubTerrainAttribute(x, y, TW_SAFEZONE);
                }
            }
        }

        for (const TileRect& door : kCageDoors)
        {
            for (int y = door.y1; y <= door.y2; ++y)
            {
                for (int x = door.x1; x <= door.x2; ++x)
                {
                    if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                        continue;
                    SubTerrainAttribute(x, y, TW_SAFEZONE);
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

        SealFenceTiles();
        OpenDoorTiles();
        ApplyPlazaSafe();
    }
}
