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
    // The shipped EncTerrain7.att is the official Webzen authoring: fence rails
    // are NOMOVE exactly where the .obj shows rails, and every visual gap
    // between rails is walkable. Only the doors are punched open at runtime so
    // the hunting cages can be entered; no rails beyond the shipped ATT are
    // ever sealed here or by the server.
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

    // Legacy 154 campus — UNSET so corridors are PvP again.
    constexpr TileRect kPlazaCampus[] =
    {
        { 0, 25, 120, 135 },
    };

    // Mirrors OpenMU ArenaCageDoors plaza + warp (Chebyshev squares).
    constexpr int kPlazaCenterX = 65;
    constexpr int kPlazaCenterY = 43;
    constexpr int kPlazaSafeRadius = 10;
    constexpr int kWarpSafeX = 102;
    constexpr int kWarpSafeY = 116;
    constexpr int kWarpSafeRadius = 3;

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

    bool InChebyshev(int x, int y, int cx, int cy, int radius)
    {
        int dx = x - cx;
        int dy = y - cy;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        return dx <= radius && dy <= radius;
    }

    bool IsPlayerSafeTile(int x, int y)
    {
        if (InDoor(x, y))
            return false;
        return InChebyshev(x, y, kPlazaCenterX, kPlazaCenterY, kPlazaSafeRadius)
            || InChebyshev(x, y, kWarpSafeX, kWarpSafeY, kWarpSafeRadius);
    }

    void ApplyRect(const TileRect& rect, bool open)
    {
        for (int y = rect.y1; y <= rect.y2; ++y)
        {
            for (int x = rect.x1; x <= rect.x2; ++x)
            {
                if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                    continue;
                if (open)
                    SubTerrainAttribute(x, y, TW_NOMOVE | TW_WATER | TW_NOGROUND);
                else
                    SubTerrainAttribute(x, y, TW_SAFEZONE);
            }
        }
    }

    void OpenDoorTiles()
    {
        for (const TileRect& door : kCageDoors)
            ApplyRect(door, true);
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
                    if (IsPlayerSafeTile(x, y) && !InRectList(kHuntingBoxes, kBoxCount, x, y))
                        AddTerrainAttribute(x, y, TW_SAFEZONE);
                    else
                        SubTerrainAttribute(x, y, TW_SAFEZONE);
                }
            }
        }

        for (int y = kWarpSafeY - kWarpSafeRadius; y <= kWarpSafeY + kWarpSafeRadius; ++y)
        {
            for (int x = kWarpSafeX - kWarpSafeRadius; x <= kWarpSafeX + kWarpSafeRadius; ++x)
            {
                if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                    continue;
                AddTerrainAttribute(x, y, TW_SAFEZONE);
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

        OpenDoorTiles();
        ApplyPlazaSafe();
    }
}
