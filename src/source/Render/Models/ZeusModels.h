#pragma once

#include "Core/Globals/_TextureIndex.h"
#include "Core/Globals/_enum.h"

// Authored Zeus set (Duel Master, the magic Gladiator evolution). The eleven
// model IDs mirror the backend item IDs (update 249) through the structural
// identity MODEL_ITEM + group * MAX_ITEM_INDEX + index, so the extended
// equipment decoder resolves them without any explicit item-to-model mapping.
namespace Render::Items::Zeus
{
    // Cape cloth fabric slot. The native Dark Lord capes own BITMAP_ROBE+6
    // (shoulder caps), +9 (main cape) and +10 (side panels), the Poseidon cape
    // owns +11/+12/+13; with side hair on +4 and the monster robes on
    // +3/+5/+6 the window was exhausted, so the Zeus cape fabric takes the
    // new last slot the widened window provides (BITMAP_ROBE_END moved from
    // +15 to +16; bitmap indexes are name-keyed map entries, internal-only).
    // All Zeus cloth grades share the single authored fabric atlas
    // (Zeus_Blue); the Platina and Emissive atlases only dress the rigid BMD
    // harness meshes, so no authored fabric ever shares a cape slot.
    constexpr int CapeClothFabricSlot = BITMAP_ROBE + 14;

    // Same neck-base anchor bone as MODEL_CAPE_OF_EMPEROR/MODEL_CAPE_OF_OVERRULE
    // and the authored Poseidon cape.
    constexpr int CapeLinkBone = 19;

    inline bool IsEquipment(int model)
    {
        return model == MODEL_ZEUS_SWORD || model == MODEL_ZEUS_STAFF
            || model == MODEL_ZEUS_ARMOR || model == MODEL_ZEUS_PANTS
            || model == MODEL_ZEUS_GLOVES || model == MODEL_ZEUS_BOOTS
            || model == MODEL_ZEUS_WINGS || model == MODEL_ZEUS_CAPE
            || model == MODEL_ZEUS_PENDANT || model == MODEL_ZEUS_STORM_RING
            || model == MODEL_ZEUS_WISDOM_RING;
    }

    // The Zeus cape runs a cloth layout of its own for the CLASS_DARK family
    // (Magic Gladiator / Duel Master): a main cape plus two side panels, never
    // the native Dark Lord skirt (that grade stays keyed to the DL/LE armor
    // skins and the Zeus faldão lives in the pants geometry, cape-cloth-contract.md).
    inline bool HasClothCape(int model)
    {
        return model == MODEL_ZEUS_CAPE;
    }
}
