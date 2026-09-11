#include "doctest.h"

#include <windows.h>
#include <cstddef>
#include <map>
#include "Core/Globals/_define.h"
#include "Render/Models/PoseidonModels.h"

// Backend item IDs (update 248, "PoseidonLordEmperor"): the client model ID is
// the structural address MODEL_ITEM + group * MAX_ITEM_INDEX + index, so the
// extended equipment decoder maps (group, index) items onto the authored
// models without any explicit translation table.
namespace
{
    constexpr int ItemModel(int group, int index) { return MODEL_ITEM + group * MAX_ITEM_INDEX + index; }
}

TEST_CASE("Poseidon covers exactly the thirteen authored equipment models")
{
    int count = 0;
    for (int model = 0; model < MAX_MODELS; ++model)
        count += Render::Items::Poseidon::IsEquipment(model) ? 1 : 0;

    CHECK(count == 13);
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(2, 20)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(2, 21)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(7, 75)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(8, 75)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(9, 75)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(10, 75)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(11, 75)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(12, 52)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(13, 202)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(13, 203)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(13, 204)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(13, 205)));
    CHECK(Render::Items::Poseidon::IsEquipment(ItemModel(13, 206)));
}

TEST_CASE("Poseidon model IDs sit at the reserved offsets next to the Celestial set")
{
    CHECK(MODEL_POSEIDON_TRIDENT == MODEL_MACE + 20);
    CHECK(MODEL_POSEIDON_SCEPTER == MODEL_MACE + 21);
    CHECK(MODEL_POSEIDON_HELM == MODEL_HELM + 75);
    CHECK(MODEL_POSEIDON_ARMOR == MODEL_ARMOR + 75);
    CHECK(MODEL_POSEIDON_PANTS == MODEL_PANTS + 75);
    CHECK(MODEL_POSEIDON_GLOVES == MODEL_GLOVES + 75);
    CHECK(MODEL_POSEIDON_BOOTS == MODEL_BOOTS + 75);
    CHECK(MODEL_POSEIDON_CAPE == MODEL_WING + 52);
    CHECK(MODEL_POSEIDON_PENDANT == MODEL_HELPER + 202);
    CHECK(MODEL_POSEIDON_TIDES_RING == MODEL_HELPER + 203);
    CHECK(MODEL_POSEIDON_EMPEROR_RING == MODEL_HELPER + 204);
    CHECK(MODEL_POSEIDON_HORSE_ITEM == MODEL_HELPER + 205);
    CHECK(MODEL_POSEIDON_EAGLE_ITEM == MODEL_HELPER + 206);
}

TEST_CASE("Poseidon offsets never collide with native or Celestial equipment")
{
    // Native neighbours on every touched group.
    CHECK(MODEL_POSEIDON_TRIDENT != MODEL_STRYKER_SCEPTER);
    CHECK(MODEL_POSEIDON_CAPE != MODEL_CELESTIAL_WINGS);
    CHECK(MODEL_POSEIDON_CAPE != MODEL_CAPE_OF_OVERRULE);
    CHECK(MODEL_POSEIDON_HELM != MODEL_CELESTIAL_HELM);
    CHECK(MODEL_POSEIDON_ARMOR != MODEL_CELESTIAL_ARMOR);
    CHECK(MODEL_POSEIDON_PANTS != MODEL_CELESTIAL_PANTS);
    CHECK(MODEL_POSEIDON_GLOVES != MODEL_CELESTIAL_GLOVES);
    CHECK(MODEL_POSEIDON_BOOTS != MODEL_CELESTIAL_BOOTS);
    CHECK(MODEL_POSEIDON_PENDANT != MODEL_CELESTIAL_PENDANT);
    CHECK(MODEL_POSEIDON_TIDES_RING != MODEL_CELESTIAL_RING);
    CHECK(MODEL_POSEIDON_HORSE_ITEM != MODEL_DARK_HORSE_ITEM);
    CHECK(MODEL_POSEIDON_EAGLE_ITEM != MODEL_DARK_RAVEN_ITEM);
    // All thirteen addresses are distinct and inside the model array.
    constexpr int poseidon[] = {MODEL_POSEIDON_TRIDENT, MODEL_POSEIDON_SCEPTER,
        MODEL_POSEIDON_HELM, MODEL_POSEIDON_ARMOR, MODEL_POSEIDON_PANTS,
        MODEL_POSEIDON_GLOVES, MODEL_POSEIDON_BOOTS, MODEL_POSEIDON_CAPE,
        MODEL_POSEIDON_PENDANT, MODEL_POSEIDON_TIDES_RING, MODEL_POSEIDON_EMPEROR_RING,
        MODEL_POSEIDON_HORSE_ITEM, MODEL_POSEIDON_EAGLE_ITEM};
    static_assert(std::size(poseidon) == 13);
    for (std::size_t i = 0; i < std::size(poseidon); ++i)
    {
        CHECK(poseidon[i] >= 0);
        CHECK(poseidon[i] < MAX_MODELS);
        for (std::size_t j = i + 1; j < std::size(poseidon); ++j)
            CHECK(poseidon[i] != poseidon[j]);
    }
}

TEST_CASE("Poseidon policy does not claim unrelated equipment")
{
    CHECK_FALSE(Render::Items::Poseidon::IsEquipment(MODEL_STAFF_OF_KUNDUN));
    CHECK_FALSE(Render::Items::Poseidon::IsEquipment(MODEL_CAPE_OF_EMPEROR));
    CHECK_FALSE(Render::Items::Poseidon::IsEquipment(MODEL_CELESTIAL_WINGS));
    CHECK_FALSE(Render::Items::Poseidon::IsEquipment(ItemModel(13, 207)));
    CHECK_FALSE(Render::Items::Poseidon::IsEquipment(ItemModel(12, 53)));
    CHECK_FALSE(Render::Items::Poseidon::IsEquipment(-1));
}

TEST_CASE("Cape cloth anchor and dedicated fabric slots follow the cloth contract")
{
    CHECK(Render::Items::Poseidon::CapeLinkBone == 19);
    CHECK(Render::Items::Poseidon::HasClothCape(MODEL_POSEIDON_CAPE));
    CHECK_FALSE(Render::Items::Poseidon::HasClothCape(MODEL_CAPE_OF_EMPEROR));
    CHECK_FALSE(Render::Items::Poseidon::HasClothCape(MODEL_CAPE_OF_LORD));

    // The authored fabric slots live inside the BITMAP_ROBE window but never
    // on the native Dark Lord slots (+6 shoulder, +9 main, +10 side panels).
    CHECK(Render::Items::Poseidon::CapeClothShoulderSlot == BITMAP_ROBE + 11);
    CHECK(Render::Items::Poseidon::CapeClothMainSlot == BITMAP_ROBE + 12);
    CHECK(Render::Items::Poseidon::CapeClothSideSlot == BITMAP_ROBE + 13);
    CHECK(Render::Items::Poseidon::CapeClothShoulderSlot < BITMAP_ROBE_END);
    CHECK(Render::Items::Poseidon::CapeClothMainSlot < BITMAP_ROBE_END);
    CHECK(Render::Items::Poseidon::CapeClothSideSlot < BITMAP_ROBE_END);
    for (const int native : {BITMAP_ROBE + 6, BITMAP_ROBE + 9, BITMAP_ROBE + 10})
    {
        CHECK(Render::Items::Poseidon::CapeClothShoulderSlot != native);
        CHECK(Render::Items::Poseidon::CapeClothMainSlot != native);
        CHECK(Render::Items::Poseidon::CapeClothSideSlot != native);
    }
}
