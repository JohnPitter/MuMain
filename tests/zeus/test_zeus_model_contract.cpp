#include "doctest.h"

#include <windows.h>
#include <cstddef>
#include <map>
#include "Core/Globals/_define.h"
#include "Render/Models/PoseidonModels.h"
#include "Render/Models/ZeusModels.h"

// Backend item IDs (update 249, "Herdeiro de Zeus" / Duel Master): the client
// model ID is the structural address MODEL_ITEM + group * MAX_ITEM_INDEX + index,
// so the extended equipment decoder maps (group, index) items onto the authored
// models without any explicit translation table.
namespace
{
    constexpr int ItemModel(int group, int index) { return MODEL_ITEM + group * MAX_ITEM_INDEX + index; }
}

TEST_CASE("Zeus covers exactly the eleven authored equipment models")
{
    int count = 0;
    for (int model = 0; model < MAX_MODELS; ++model)
        count += Render::Items::Zeus::IsEquipment(model) ? 1 : 0;

    CHECK(count == 11);
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(0, 36)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(5, 38)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(8, 76)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(9, 76)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(10, 76)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(11, 76)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(12, 53)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(12, 54)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(13, 207)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(13, 208)));
    CHECK(Render::Items::Zeus::IsEquipment(ItemModel(13, 209)));
}

TEST_CASE("Zeus model IDs sit at the reserved offsets next to the Poseidon set")
{
    CHECK(MODEL_ZEUS_SWORD == MODEL_SWORD + 36);
    CHECK(MODEL_ZEUS_STAFF == MODEL_STAFF + 38);
    CHECK(MODEL_ZEUS_ARMOR == MODEL_ARMOR + 76);
    CHECK(MODEL_ZEUS_PANTS == MODEL_PANTS + 76);
    CHECK(MODEL_ZEUS_GLOVES == MODEL_GLOVES + 76);
    CHECK(MODEL_ZEUS_BOOTS == MODEL_BOOTS + 76);
    CHECK(MODEL_ZEUS_WINGS == MODEL_WING + 53);
    CHECK(MODEL_ZEUS_CAPE == MODEL_WING + 54);
    CHECK(MODEL_ZEUS_PENDANT == MODEL_HELPER + 207);
    CHECK(MODEL_ZEUS_STORM_RING == MODEL_HELPER + 208);
    CHECK(MODEL_ZEUS_WISDOM_RING == MODEL_HELPER + 209);
}

TEST_CASE("Zeus offsets never collide with native, Celestial or Poseidon equipment")
{
    // Native neighbours on every touched group.
    CHECK(MODEL_ZEUS_SWORD != MODEL_CELESTIAL_HELM); // group sanity: sword group vs helm group
    CHECK(MODEL_ZEUS_STAFF != MODEL_STAFF_OF_KUNDUN);
    CHECK(MODEL_ZEUS_WINGS != MODEL_CELESTIAL_WINGS);
    CHECK(MODEL_ZEUS_WINGS != MODEL_SEED_FIRE);
    CHECK(MODEL_ZEUS_CAPE != MODEL_POSEIDON_CAPE);
    CHECK(MODEL_ZEUS_CAPE != MODEL_CAPE_OF_OVERRULE);
    CHECK(MODEL_ZEUS_ARMOR != MODEL_CELESTIAL_ARMOR);
    CHECK(MODEL_ZEUS_ARMOR != MODEL_POSEIDON_HELM);
    CHECK(MODEL_ZEUS_PANTS != MODEL_CELESTIAL_PANTS);
    CHECK(MODEL_ZEUS_GLOVES != MODEL_CELESTIAL_GLOVES);
    CHECK(MODEL_ZEUS_BOOTS != MODEL_CELESTIAL_BOOTS);
    CHECK(MODEL_ZEUS_PENDANT != MODEL_CELESTIAL_PENDANT);
    CHECK(MODEL_ZEUS_PENDANT != MODEL_POSEIDON_EAGLE_ITEM);
    CHECK(MODEL_ZEUS_STORM_RING != MODEL_POSEIDON_TIDES_RING);
    CHECK(MODEL_ZEUS_WISDOM_RING != MODEL_POSEIDON_EMPEROR_RING);
    // All eleven addresses are distinct and inside the model array.
    constexpr int zeus[] = {MODEL_ZEUS_SWORD, MODEL_ZEUS_STAFF, MODEL_ZEUS_ARMOR,
        MODEL_ZEUS_PANTS, MODEL_ZEUS_GLOVES, MODEL_ZEUS_BOOTS, MODEL_ZEUS_WINGS,
        MODEL_ZEUS_CAPE, MODEL_ZEUS_PENDANT, MODEL_ZEUS_STORM_RING, MODEL_ZEUS_WISDOM_RING};
    static_assert(std::size(zeus) == 11);
    for (std::size_t i = 0; i < std::size(zeus); ++i)
    {
        CHECK(zeus[i] >= 0);
        CHECK(zeus[i] < MAX_MODELS);
        for (std::size_t j = i + 1; j < std::size(zeus); ++j)
            CHECK(zeus[i] != zeus[j]);
    }
}

TEST_CASE("Zeus policy does not claim unrelated equipment")
{
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(MODEL_STAFF_OF_KUNDUN));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(MODEL_CAPE_OF_EMPEROR));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(MODEL_CELESTIAL_WINGS));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(MODEL_POSEIDON_CAPE));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(ItemModel(12, 52)));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(ItemModel(13, 206)));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(ItemModel(13, 210)));
    CHECK_FALSE(Render::Items::Zeus::IsEquipment(-1));
}

TEST_CASE("Cape cloth anchor and dedicated fabric slot follow the cloth contract")
{
    CHECK(Render::Items::Zeus::CapeLinkBone == 19);
    CHECK(Render::Items::Zeus::HasClothCape(MODEL_ZEUS_CAPE));
    CHECK_FALSE(Render::Items::Zeus::HasClothCape(MODEL_CAPE_OF_EMPEROR));
    CHECK_FALSE(Render::Items::Zeus::HasClothCape(MODEL_CAPE_OF_LORD));
    CHECK_FALSE(Render::Items::Zeus::HasClothCape(MODEL_POSEIDON_CAPE));

    // The authored fabric slot lives inside the BITMAP_ROBE window but never
    // on the native Dark Lord slots (+6 shoulder, +7 small cape, +9 main,
    // +10 side panels) nor on the Poseidon grades (+11/+12/+13).
    CHECK(Render::Items::Zeus::CapeClothFabricSlot == BITMAP_ROBE + 14);
    CHECK(Render::Items::Zeus::CapeClothFabricSlot < BITMAP_ROBE_END);
    for (const int taken : {BITMAP_ROBE + 6, BITMAP_ROBE + 7, BITMAP_ROBE + 8,
        BITMAP_ROBE + 9, BITMAP_ROBE + 10,
        Render::Items::Poseidon::CapeClothShoulderSlot,
        Render::Items::Poseidon::CapeClothMainSlot,
        Render::Items::Poseidon::CapeClothSideSlot})
    {
        CHECK(Render::Items::Zeus::CapeClothFabricSlot != taken);
    }
}
