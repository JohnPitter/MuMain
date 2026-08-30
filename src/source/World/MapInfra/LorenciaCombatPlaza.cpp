#include "stdafx.h"
#include "World/MapInfra/LorenciaCombatPlaza.h"

#include <cmath>

#include "Core/Globals/_define.h"
#include "Core/Globals/_enum.h"
#include "Data/DataHandler/LoadData.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "World/MapInfra/MapManager.h"

namespace
{
    constexpr int kPlazaMinX = 131;
    constexpr int kPlazaMinY = 118;
    constexpr int kPlazaSizeX = 21;
    constexpr int kPlazaSizeY = 21;

    // Unused Lorencia world-object slots.
    constexpr int kSlotPillar = 53; // Object34/Object53 — altar ring pieces
    constexpr int kSlotGlow = 154;  // Object34/Object42 — cyan glow anchor

    constexpr float kFountainX = 141.f * TERRAIN_SCALE;
    constexpr float kFountainY = 128.f * TERRAIN_SCALE;
    constexpr float kHideRadius = 550.f;
    constexpr float kPlazaScale = 2.6f;
    constexpr float kPillarRadius = 480.f;
    constexpr float kPillarScale = 3.2f;
    constexpr int kPillarCount = 8;

    bool IsFountainDebris(int type, float dist)
    {
        if (type == MODEL_WATERSPOUT)
            return true;
        // Object24/25 sit on the same tile as the fountain and make up the
        // carved basin; nearby fences/steps ring the structure.
        if (dist <= 350.f && (type == 23 || type == 24 || type == 25))
            return true;
        if (dist <= 450.f && (type == 83 || type == 84 || type == 127))
            return true;
        return false;
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
                    const float dx = o->Position[0] - kFountainX;
                    const float dy = o->Position[1] - kFountainY;
                    const float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist > kHideRadius)
                        continue;
                    if (!IsFountainDebris(o->Type, dist))
                        continue;
                    o->Live = false;
                    o->Visible = false;
                    o->Alpha = 0.f;
                    o->AlphaTarget = 0.f;
                }
            }
        }
    }

    void LoadCrywolfMeshes()
    {
        // Object50 = circular claw platform used at the Crywolf altar.
        gLoadData.AccessModel(MODEL_WATERSPOUT, L"Data\\Object34\\", L"Object", 50);
        gLoadData.OpenTexture(MODEL_WATERSPOUT, L"Object34\\");

        gLoadData.AccessModel(kSlotPillar, L"Data\\Object34\\", L"Object", 53);
        gLoadData.OpenTexture(kSlotPillar, L"Object34\\");

        gLoadData.AccessModel(kSlotGlow, L"Data\\Object34\\", L"Object", 42);
        gLoadData.OpenTexture(kSlotGlow, L"Object34\\");
    }

    void SpawnCrywolfPlaza()
    {
        const float ground = RequestTerrainHeight(kFountainX, kFountainY);

        vec3_t pos{};
        vec3_t ang{};
        pos[0] = kFountainX;
        pos[1] = kFountainY;
        pos[2] = ground;
        ang[2] = 36.f;

        // Reuse the waterspout slot: same world transform, Crywolf mesh.
        OBJECT* plaza = CreateObject(MODEL_WATERSPOUT, pos, ang, kPlazaScale);
        if (plaza)
        {
            plaza->Live = true;
            plaza->Visible = true;
            plaza->Alpha = 1.f;
            plaza->AlphaTarget = 1.f;
        }

        ang[2] = 0.f;
        CreateObject(kSlotGlow, pos, ang, 1.2f);

        for (int i = 0; i < kPillarCount; ++i)
        {
            const float rad = (static_cast<float>(i) * 45.f) * (3.14159265f / 180.f);
            pos[0] = kFountainX + std::cos(rad) * kPillarRadius;
            pos[1] = kFountainY + std::sin(rad) * kPillarRadius;
            pos[2] = RequestTerrainHeight(pos[0], pos[1]);
            // Face inward toward plaza center.
            ang[2] = static_cast<float>(i) * 45.f + 180.f;
            CreateObject(kSlotPillar, pos, ang, kPillarScale);
        }

        vec3_t light{};
        Vector(0.15f, 0.65f, 0.9f, light);
        AddTerrainLight(kFountainX, kFountainY, light, 4, PrimaryTerrainLight);
    }
}

namespace World::Lorencia
{
    void ApplyFountainCombatPlaza()
    {
        if (gMapManager.WorldActive != WD_0LORENCIA)
            return;

        AddTerrainAttributeRange(kPlazaMinX, kPlazaMinY, kPlazaSizeX, kPlazaSizeY, TW_SAFEZONE, 0);
        HideFountainCluster();
        LoadCrywolfMeshes();
        SpawnCrywolfPlaza();
    }
}
