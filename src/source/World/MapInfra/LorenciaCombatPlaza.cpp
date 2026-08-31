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

    // Original fountain courtyard (21x21 safezone). Hedge/fence tiles in this
    // square still carry TW_NOMOVE after we hide the meshes — walking onto
    // them makes OpenMU reject the step and the character snaps back.
    constexpr int kYardMinX = 131;
    constexpr int kYardMinY = 118;
    constexpr int kYardMaxX = 151;
    constexpr int kYardMaxY = 138;

    // Stone-paved Lorencia town (inside the water/wall ring). Forest grass
    // outside these bounds stays. EncTerrain1.map layer1: 0 = TileGrass01
    // (the leftover green rectangles), 3 = TileGround02 (plaza stone).
    constexpr int kPlazaMinX = 111;
    constexpr int kPlazaMaxX = 164;
    constexpr int kPlazaMinY = 105;
    constexpr int kPlazaMaxY = 150;
    constexpr unsigned char kGrassMapping = 0;
    constexpr unsigned char kStoneMapping = 3;

    constexpr int kSlotBase = 157; // Object35/Object41 — circular floor

    constexpr float kFountainX = 141.f * TERRAIN_SCALE;
    constexpr float kFountainY = 128.f * TERRAIN_SCALE;
    constexpr float kHideRadius = 2000.f;
    // Native Object41 radius ~370. 1.05 ≈ 8-tile ring, walkable inside.
    constexpr float kBaseScale = 1.05f;
    // Sit almost flush with terrain so feet are not swallowed by a raised disc.
    // Depth-mask stays off on render.
    constexpr float kFloorLift = 0.4f;

    bool IsGrassObject(int type)
    {
        return type >= MODEL_GRASS01 && type <= MODEL_GRASS01 + 15;
    }

    bool InStonePlaza(float worldX, float worldY)
    {
        const float minX = static_cast<float>(kPlazaMinX) * TERRAIN_SCALE;
        const float maxX = static_cast<float>(kPlazaMaxX + 1) * TERRAIN_SCALE;
        const float minY = static_cast<float>(kPlazaMinY) * TERRAIN_SCALE;
        const float maxY = static_cast<float>(kPlazaMaxY + 1) * TERRAIN_SCALE;
        return worldX >= minX && worldX < maxX && worldY >= minY && worldY < maxY;
    }

    bool ShouldHideType(int type)
    {
        if (IsGrassObject(type))
            return true;
        if (type >= MODEL_TREE01 && type <= MODEL_TREE01 + 6)
            return true;
        if (type >= MODEL_FENCE01 && type <= MODEL_FENCE04)
            return true;
        if (type == MODEL_STREET_LIGHT || type == MODEL_WATERSPOUT || type == 127)
            return true;
        if (type >= MODEL_LIGHT01 && type <= MODEL_LIGHT03)
            return true;
        return false;
    }

    // Previous hide only set Live=false on grass OBJECTS. João's screenshot is a
    // 1-cell TerrainMappingLayer1==0 (TileGrass01) sitting on layer1==3 stone —
    // the terrain pass still drew the green rectangle. Object IDs were never
    // the leak; paving these cells is.
    void PavePlazaGrassTiles()
    {
        for (int y = kPlazaMinY; y <= kPlazaMaxY; ++y)
        {
            for (int x = kPlazaMinX; x <= kPlazaMaxX; ++x)
            {
                const int i = TERRAIN_INDEX(x, y);
                if (TerrainMappingLayer1[i] != kGrassMapping)
                    continue;
                TerrainMappingLayer1[i] = kStoneMapping;
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
                    }
                    else
                    {
                        const float dx = o->Position[0] - kFountainX;
                        const float dy = o->Position[1] - kFountainY;
                        if ((dx * dx + dy * dy) > (kHideRadius * kHideRadius))
                            continue;
                        if (!ShouldHideType(o->Type))
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

    bool ShouldHideNearFountain(int type, float worldX, float worldY)
    {
        if (IsGrassObject(type))
            return InStonePlaza(worldX, worldY);
        if (!ShouldHideType(type))
            return false;
        const float dx = worldX - kFountainX;
        const float dy = worldY - kFountainY;
        return (dx * dx + dy * dy) <= (kHideRadius * kHideRadius);
    }

    void ApplyFountainCombatPlaza()
    {
        if (gMapManager.WorldActive != WD_0LORENCIA)
            return;

        PavePlazaGrassTiles();
        ClearCourtyardWalkability();
        LoadCrywolfFloor();
        HideFountainCluster();
        SpawnCrywolfPlaza();
    }
}
