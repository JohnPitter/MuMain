#pragma once

#include "Core/Globals/_TextureIndex.h"
#include "Core/Globals/_enum.h"

// Authored Poseidon set (Lord Emperor). The thirteen model IDs mirror the
// backend item IDs (update 248) through the structural identity
// MODEL_ITEM + group * MAX_ITEM_INDEX + index, so the extended equipment
// decoder resolves them without any explicit item-to-model mapping.
namespace Render::Items::Poseidon
{
    // Cape cloth texture slots. The native Dark Lord cape grades own the
    // global slots BITMAP_ROBE+6 (shoulder caps), +9 (main cape) and
    // +10 (side panels); the Poseidon cape points at these dedicated slots
    // instead, inside the unused half of the BITMAP_ROBE window
    // (BITMAP_ROBE_END = BITMAP_ROBE_BEGIN + 15), so the authored fabric can
    // never leak into the native capes and vice versa.
    constexpr int CapeClothShoulderSlot = BITMAP_ROBE + 11;
    constexpr int CapeClothMainSlot = BITMAP_ROBE + 12;
    constexpr int CapeClothSideSlot = BITMAP_ROBE + 13;

    // Same neck-base anchor bone as MODEL_CAPE_OF_EMPEROR/MODEL_CAPE_OF_OVERRULE.
    constexpr int CapeLinkBone = 19;

    inline bool IsEquipment(int model)
    {
        return model == MODEL_POSEIDON_TRIDENT || model == MODEL_POSEIDON_SCEPTER
            || model == MODEL_POSEIDON_HELM || model == MODEL_POSEIDON_ARMOR
            || model == MODEL_POSEIDON_PANTS || model == MODEL_POSEIDON_GLOVES
            || model == MODEL_POSEIDON_BOOTS || model == MODEL_POSEIDON_CAPE
            || model == MODEL_POSEIDON_PENDANT || model == MODEL_POSEIDON_TIDES_RING
            || model == MODEL_POSEIDON_EMPEROR_RING || model == MODEL_POSEIDON_HORSE_ITEM
            || model == MODEL_POSEIDON_EAGLE_ITEM;
    }

    // The Poseidon cape runs the full six-grade cloth layout (shoulder caps,
    // main cape, two side panels) like MODEL_CAPE_OF_EMPEROR, but never the
    // native skirt: the skirt stays keyed to the Dark Lord / Lord Emperor
    // armor skins, and the Poseidon cuirass is a different model.
    inline bool HasClothCape(int model)
    {
        return model == MODEL_POSEIDON_CAPE;
    }
}
