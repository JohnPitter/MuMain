#include "stdafx.h"
#include "World/MapInfra/LorenciaCombatPlaza.h"

#include "Core/Globals/_define.h"
#include "Core/Globals/_enum.h"
#include "Data/DataHandler/LoadData.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "World/MapInfra/MapManager.h"

namespace
{
    constexpr int kCenterTileX = 141;
    constexpr int kCenterTileY = 128;
    constexpr int kPvpRadiusTiles = 5;

    constexpr int kYardMinX = 131;
    constexpr int kYardMinY = 118;
    constexpr int kYardMaxX = 151;
    constexpr int kYardMaxY = 138;

    constexpr int kPlazaMinX = 111;
    constexpr int kPlazaMaxX = 164;
    constexpr int kPlazaMinY = 105;
    constexpr int kPlazaMaxY = 150;
    constexpr unsigned char kGrassMapping = 0;
    constexpr unsigned char kStoneMapping = 3;

    constexpr int kKeepPropRadius = 2;

    constexpr int kSlotBase = 157;

    constexpr float kFountainX = 141.f * TERRAIN_SCALE;
    constexpr float kFountainY = 128.f * TERRAIN_SCALE;
    constexpr float kHideRadius = 2000.f;
    constexpr float kBaseScale = 1.05f;
    constexpr float kFloorLift = 0.4f;

    bool g_keepGrass[TERRAIN_SIZE * TERRAIN_SIZE]{};

    bool InPlazaTiles(int x, int y)
    {
        return x >= kPlazaMinX && x <= kPlazaMaxX && y >= kPlazaMinY && y <= kPlazaMaxY;
    }

    bool IsGrassObject(int type)
    {
        return type >= MODEL_GRASS01 && type <= MODEL_GRASS01 + 15;
    }

    bool IsTreeObject(int type)
    {
        return type >= MODEL_TREE01 && type <= MODEL_TREE01 + 6;
    }

    bool IsFenceObject(int type)
    {
        return type >= MODEL_FENCE01 && type <= MODEL_FENCE04;
    }

    bool InStonePlaza(float worldX, float worldY)
    {
        const float minX = static_cast<float>(kPlazaMinX) * TERRAIN_SCALE;
        const float maxX = static_cast<float>(kPlazaMaxX + 1) * TERRAIN_SCALE;
        const float minY = static_cast<float>(kPlazaMinY) * TERRAIN_SCALE;
        const float maxY = static_cast<float>(kPlazaMaxY + 1) * TERRAIN_SCALE;
        return worldX >= minX && worldX < maxX && worldY >= minY && worldY < maxY;
    }

    bool InFountainHideRadius(float worldX, float worldY)
    {
        const float dx = worldX - kFountainX;
        const float dy = worldY - kFountainY;
        return (dx * dx + dy * dy) <= (kHideRadius * kHideRadius);
    }

    bool ShouldHideType(int type)
    {
        if (IsGrassObject(type) || IsTreeObject(type))
            return true;
        if (type >= MODEL_FENCE01 && type <= MODEL_FENCE04)
            return true;
        if (type == MODEL_STREET_LIGHT || type == MODEL_WATERSPOUT || type == 127)
            return true;
        if (type >= MODEL_LIGHT01 && type <= MODEL_LIGHT03)
            return true;
        return false;
    }

    // Chests, walls, doors, houses stay. Fountain hedges/lights do not seed keep-grass.
    bool IsKeepProp(int type, float worldX, float worldY)
    {
        if (IsGrassObject(type) || IsTreeObject(type))
            return false;
        if (ShouldHideType(type) && InFountainHideRadius(worldX, worldY))
            return false;
        return true;
    }

    int TileFromWorld(float world)
    {
        return static_cast<int>(world / TERRAIN_SCALE);
    }

    bool KeepGrassAt(int x, int y)
    {
        if (!InPlazaTiles(x, y))
            return false;
        return g_keepGrass[TERRAIN_INDEX(x, y)];
    }

    bool NearKeepGrass(int tx, int ty)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (KeepGrassAt(tx + dx, ty + dy))
                    return true;
            }
        }
        return false;
    }

    // Cercadinhos stay only on remaining lawns (chest/gate). Empty paved
    // planter rows around the fountain lose their leftover frames.
    bool ShouldKeepFence(int tx, int ty)
    {
        return NearKeepGrass(tx, ty);
    }

    void SeedKeepGrass(int tx, int ty)
    {
        for (int dy = -kKeepPropRadius; dy <= kKeepPropRadius; ++dy)
        {
            for (int dx = -kKeepPropRadius; dx <= kKeepPropRadius; ++dx)
            {
                const int x = tx + dx;
                const int y = ty + dy;
                if (!InPlazaTiles(x, y))
                    continue;
                const int i = TERRAIN_INDEX(x, y);
                if (TerrainMappingLayer1[i] == kGrassMapping)
                    g_keepGrass[i] = true;
            }
        }
    }

    void FloodKeepGrass()
    {
        int qx[4096];
        int qy[4096];
        int n = 0;
        for (int y = kPlazaMinY; y <= kPlazaMaxY; ++y)
        {
            for (int x = kPlazaMinX; x <= kPlazaMaxX; ++x)
            {
                if (g_keepGrass[TERRAIN_INDEX(x, y)])
                {
                    qx[n] = x;
                    qy[n] = y;
                    ++n;
                }
            }
        }

        constexpr int kDx[4] = { 1, -1, 0, 0 };
        constexpr int kDy[4] = { 0, 0, 1, -1 };
        for (int head = 0; head < n; ++head)
        {
            for (int d = 0; d < 4; ++d)
            {
                const int x = qx[head] + kDx[d];
                const int y = qy[head] + kDy[d];
                if (!InPlazaTiles(x, y))
                    continue;
                const int i = TERRAIN_INDEX(x, y);
                if (g_keepGrass[i] || TerrainMappingLayer1[i] != kGrassMapping)
                    continue;
                g_keepGrass[i] = true;
                if (n < static_cast<int>(sizeof(qx) / sizeof(qx[0])))
                {
                    qx[n] = x;
                    qy[n] = y;
                    ++n;
                }
            }
        }
    }

    void MarkKeepGrassFromProps()
    {
        memset(g_keepGrass, 0, sizeof(g_keepGrass));
        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                OBJECT_BLOCK* block = &ObjectBlock[i * 16 + j];
                for (OBJECT* o = block->Head; o != nullptr; o = o->Next)
                {
                    if (!o->Live)
                        continue;
                    if (!IsKeepProp(o->Type, o->Position[0], o->Position[1]))
                        continue;
                    SeedKeepGrass(TileFromWorld(o->Position[0]), TileFromWorld(o->Position[1]));
                }
            }
        }
        FloodKeepGrass();
    }

    // Only orphan grass islands on stone. Grass next to chests/walls stays.
    void PaveOrphanGrassTiles()
    {
        for (int y = kPlazaMinY; y <= kPlazaMaxY; ++y)
        {
            for (int x = kPlazaMinX; x <= kPlazaMaxX; ++x)
            {
                const int i = TERRAIN_INDEX(x, y);
                if (TerrainMappingLayer1[i] != kGrassMapping)
                    continue;
                if (g_keepGrass[i])
                    continue;
                TerrainMappingLayer1[i] = kStoneMapping;
                SubTerrainAttribute(x, y, TW_NOMOVE | TW_WATER | TW_NOGROUND);
            }
        }
    }

    void HideFountainCluster()
    {
        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                OBJECT_BLOCK* block = &ObjectBlock[i * 16 + j];
                for (OBJECT* o = block->Head; o != nullptr; o = o->Next)
                {
                    if (!o->Live)
                        continue;
                    if (IsGrassObject(o->Type))
                    {
                        if (!InStonePlaza(o->Position[0], o->Position[1]))
                            continue;
                        if (KeepGrassAt(TileFromWorld(o->Position[0]), TileFromWorld(o->Position[1])))
                            continue;
                    }
                    else
                    {
                        if (!InFountainHideRadius(o->Position[0], o->Position[1]))
                            continue;
                        if (!ShouldHideType(o->Type))
                            continue;
                        if (IsFenceObject(o->Type)
                            && ShouldKeepFence(TileFromWorld(o->Position[0]), TileFromWorld(o->Position[1])))
                            continue;
                    }
                    o->Live = false;
                    o->Visible = false;
                    o->Alpha = 0.f;
                    o->AlphaTarget = 0.f;
                }
            }
        }
    }

    OBJECT* PlaceFloor(float x, float y, float z, float scale)
    {
        vec3_t pos{};
        vec3_t ang{};
        pos[0] = x;
        pos[1] = y;
        pos[2] = z;
        OBJECT* o = CreateObject(kSlotBase, pos, ang, scale);
        if (!o)
            return nullptr;
        o->Live = true;
        o->Visible = true;
        o->Alpha = 1.f;
        o->AlphaTarget = 1.f;
        o->BlendMesh = -1;
        o->m_bCollisionCheck = false;
        o->CollisionRange = -300.f;
        Vector(-40.f, -40.f, 0.f, o->BoundingBoxMin);
        Vector(40.f, 40.f, 8.f, o->BoundingBoxMax);
        return o;
    }

    void LoadCrywolfFloor()
    {
        gLoadData.AccessModel(kSlotBase, L"Data\\Object35\\", L"Object", 41);
        gLoadData.OpenTexture(kSlotBase, L"Object35\\");
    }

    void SpawnCrywolfPlaza()
    {
        const float ground = RequestTerrainHeight(kFountainX, kFountainY);
        PlaceFloor(kFountainX, kFountainY, ground + kFloorLift, kBaseScale);
    }

    bool InPvpCircle(int x, int y)
    {
        const int dx = x - kCenterTileX;
        const int dy = y - kCenterTileY;
        return (dx * dx + dy * dy) <= (kPvpRadiusTiles * kPvpRadiusTiles);
    }

    void ClearCourtyardWalkability()
    {
        for (int y = kYardMinY; y <= kYardMaxY; ++y)
        {
            for (int x = kYardMinX; x <= kYardMaxX; ++x)
            {
                if (x < 0 || y < 0 || x >= TERRAIN_SIZE || y >= TERRAIN_SIZE)
                    continue;
                SubTerrainAttribute(x, y, TW_NOMOVE | TW_WATER | TW_NOGROUND);
                if (InPvpCircle(x, y))
                    SubTerrainAttribute(x, y, TW_SAFEZONE);
                else
                    AddTerrainAttribute(x, y, TW_SAFEZONE);
            }
        }
    }
}

namespace World::Lorencia
{
    bool IsCombatPlazaFloor(int type)
    {
        return type == kSlotBase;
    }

    bool IsTownPlazaTile(int tileX, int tileY)
    {
        return InPlazaTiles(tileX, tileY);
    }

    bool ShouldHideNearFountain(int type, float worldX, float worldY)
    {
        if (IsGrassObject(type))
        {
            if (!InStonePlaza(worldX, worldY))
                return false;
            return !KeepGrassAt(TileFromWorld(worldX), TileFromWorld(worldY));
        }
        if (IsFenceObject(type))
        {
            if (!InFountainHideRadius(worldX, worldY))
                return false;
            return !ShouldKeepFence(TileFromWorld(worldX), TileFromWorld(worldY));
        }
        if (!ShouldHideType(type))
            return false;
        return InFountainHideRadius(worldX, worldY);
    }

    void ApplyFountainCombatPlaza()
    {
        if (gMapManager.WorldActive != WD_0LORENCIA)
            return;

        MarkKeepGrassFromProps();
        PaveOrphanGrassTiles();
        ClearCourtyardWalkability();
        LoadCrywolfFloor();
        HideFountainCluster();
        SpawnCrywolfPlaza();
    }
}
