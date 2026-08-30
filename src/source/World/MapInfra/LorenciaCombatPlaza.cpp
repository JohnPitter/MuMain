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
    // Fountain sits at tile (141, 128). Clear a plaza around it so players
    // can fight where the Waterspout used to force a safezone.
    constexpr int kPlazaMinX = 131;
    constexpr int kPlazaMinY = 118;
    constexpr int kPlazaSizeX = 21;
    constexpr int kPlazaSizeY = 21;

    // Unused Lorencia world-object slots (not present in EncTerrain1.obj).
    // We load Crywolf (Object34) meshes into these so CreateObject does not
    // stomp anything already placed in town.
    constexpr int kSlotPlaza = 49;   // Object50.bmd — circular claw platform at Crywolf altar
    constexpr int kSlotGlow = 154;   // Object42.bmd — light/effect anchor (Crywolf type 41)

    // Crywolf altar placement of Object50 (type 49): scale 2.18, yaw 36.
    constexpr float kPlazaScale = 2.18f;
    constexpr float kFountainTileX = 141.f;
    constexpr float kFountainTileY = 128.f;

    void HideWaterspoutObjects()
    {
        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                OBJECT_BLOCK* block = &ObjectBlock[i * 16 + j];
                for (OBJECT* o = block->Head; o != nullptr; o = o->Next)
                {
                    if (o->Live && o->Type == MODEL_WATERSPOUT)
                    {
                        o->Live = false;
                        o->Visible = false;
                        o->Alpha = 0.f;
                        o->AlphaTarget = 0.f;
                    }
                }
            }
        }
    }

    void LoadCrywolfPlazaMeshes()
    {
        gLoadData.AccessModel(kSlotPlaza, L"Data\\Object34\\", L"Object", 50);
        gLoadData.OpenTexture(kSlotPlaza, L"Object34\\");

        gLoadData.AccessModel(kSlotGlow, L"Data\\Object34\\", L"Object", 42);
        gLoadData.OpenTexture(kSlotGlow, L"Object34\\");
    }

    void SpawnCrywolfStylePlaza()
    {
        vec3_t pos{};
        vec3_t ang{};

        pos[0] = kFountainTileX * TERRAIN_SCALE;
        pos[1] = kFountainTileY * TERRAIN_SCALE;
        pos[2] = RequestTerrainHeight(pos[0], pos[1]);

        // Match Crywolf altar orientation of the claw platform.
        ang[0] = 0.f;
        ang[1] = 0.f;
        ang[2] = 36.f;
        CreateObject(kSlotPlaza, pos, ang, kPlazaScale);

        // Center glow anchor (same role as Crywolf type 41).
        ang[2] = 0.f;
        CreateObject(kSlotGlow, pos, ang, 1.0f);

        vec3_t light{};
        Vector(0.2f, 0.7f, 0.85f, light);
        AddTerrainLight(pos[0], pos[1], light, 3, PrimaryTerrainLight);
    }
}

namespace World::Lorencia
{
    void ApplyFountainCombatPlaza()
    {
        if (gMapManager.WorldActive != WD_0LORENCIA)
            return;

        // Add=0 clears the attribute bits (see AddTerrainAttributeRange).
        AddTerrainAttributeRange(kPlazaMinX, kPlazaMinY, kPlazaSizeX, kPlazaSizeY, TW_SAFEZONE, 0);
        HideWaterspoutObjects();
        LoadCrywolfPlazaMeshes();
        SpawnCrywolfStylePlaza();
    }
}
