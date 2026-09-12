#include "doctest.h"

#include <windows.h>
#include <cstddef>
#include <map>
#include "Core/Globals/_define.h"
#include "Render/Models/CelestialModels.h"
#include "Render/Models/PoseidonModels.h"
#include "Render/Models/PoseidonPets.h"

// Backend item IDs (update 248): cavalo 13/205 and águia 13/206 live on the
// helper group. The model contracts below freeze the activation wiring:
// structural model addresses, the item-id space the equipment switches use,
// and the action slots proven in the BMD action reports.
namespace
{
    constexpr int ItemModelOffset(int group, int index) { return group * MAX_ITEM_INDEX + index; }

    // player.bmd skeleton (60 bones, native-reference audit): the perch
    // anchors must address bones that exist on every rider.
    constexpr int PlayerBoneCount = 60;
}

TEST_CASE("Poseidon mount and pet resolve at their structural addresses")
{
    CHECK(MODEL_POSEIDON_HORSE_ITEM == MODEL_HELPER + 205);
    CHECK(MODEL_POSEIDON_EAGLE_ITEM == MODEL_HELPER + 206);
    CHECK(MODEL_POSEIDON_HORSE_ITEM == MODEL_ITEM + ItemModelOffset(13, 205));
    CHECK(MODEL_POSEIDON_EAGLE_ITEM == MODEL_ITEM + ItemModelOffset(13, 206));

    CHECK(Render::Items::Poseidon::IsMountModel(MODEL_POSEIDON_HORSE_ITEM));
    CHECK(Render::Items::Poseidon::IsPetModel(MODEL_POSEIDON_EAGLE_ITEM));
    CHECK_FALSE(Render::Items::Poseidon::IsMountModel(MODEL_POSEIDON_EAGLE_ITEM));
    CHECK_FALSE(Render::Items::Poseidon::IsPetModel(MODEL_POSEIDON_HORSE_ITEM));
    CHECK_FALSE(Render::Items::Poseidon::IsMountModel(MODEL_DARK_HORSE));
    CHECK_FALSE(Render::Items::Poseidon::IsPetModel(MODEL_DARK_SPIRIT));
}

TEST_CASE("Pet item IDs match the equipment switch space")
{
    CHECK(Render::Items::Poseidon::HorseItemId == ITEM_HELPER + 205);
    CHECK(Render::Items::Poseidon::EagleItemId == ITEM_HELPER + 206);
    // ITEM::Type lives in the offset space, not the model space.
    CHECK(Render::Items::Poseidon::HorseItemId == MODEL_POSEIDON_HORSE_ITEM - MODEL_ITEM);
    CHECK(Render::Items::Poseidon::EagleItemId == MODEL_POSEIDON_EAGLE_ITEM - MODEL_ITEM);
    CHECK(Render::Items::Poseidon::HorseItemId != Render::Items::Poseidon::EagleItemId);
}

TEST_CASE("Horse action slots preserve the native DarkHorse contract")
{
    // mount-actions-report.json: 0 stand, 1 gallop, 2 rider strike,
    // 3 earthshake, 5/6 idles (slots the GOBoid MODEL_DARK_HORSE branch maps).
    CHECK(Render::Items::Poseidon::HorseActionStand == 0);
    CHECK(Render::Items::Poseidon::HorseActionGallop == 1);
    CHECK(Render::Items::Poseidon::HorseActionRiderStrike == 2);
    CHECK(Render::Items::Poseidon::HorseActionEarthshake == 3);
    CHECK(Render::Items::Poseidon::HorseActionIdle1 == 5);
    CHECK(Render::Items::Poseidon::HorseActionIdle2 == 6);
}

TEST_CASE("Eagle action slots follow the CSPetSystem flight states")
{
    // eagle-actions-report.json: 0 fly, 1 flying, 2 stand, 3 escape — the
    // states CSPetDarkSpirit maps, and CSPetPoseidonEagle with it.
    CHECK(Render::Items::Poseidon::EagleActionFly == 0);
    CHECK(Render::Items::Poseidon::EagleActionFlying == 1);
    CHECK(Render::Items::Poseidon::EagleActionStand == 2);
    CHECK(Render::Items::Poseidon::EagleActionEscape == 3);
}

TEST_CASE("The authored horse rides exactly like the native Dark Horse")
{
    CHECK(Render::Items::Poseidon::IsDarkHorseRideHelper(MODEL_DARK_HORSE_ITEM));
    CHECK(Render::Items::Poseidon::IsDarkHorseRideHelper(MODEL_POSEIDON_HORSE_ITEM));
    // Other helpers must never trigger the DarkHorse ride animations.
    CHECK_FALSE(Render::Items::Poseidon::IsDarkHorseRideHelper(MODEL_HORN_OF_FENRIR));
    CHECK_FALSE(Render::Items::Poseidon::IsDarkHorseRideHelper(MODEL_HORN_OF_UNIRIA));
    CHECK_FALSE(Render::Items::Poseidon::IsDarkHorseRideHelper(MODEL_HORN_OF_DINORANT));
    CHECK_FALSE(Render::Items::Poseidon::IsDarkHorseRideHelper(MODEL_GUARDIAN_ANGEL));
    CHECK_FALSE(Render::Items::Poseidon::IsDarkHorseRideHelper(-1));
}

TEST_CASE("Eagle perch anchors address player bones")
{
    CHECK(Render::Items::Poseidon::EaglePerchBone >= 0);
    CHECK(Render::Items::Poseidon::EaglePerchReturnBone >= 0);
    CHECK(Render::Items::Poseidon::EaglePerchBone < PlayerBoneCount);
    CHECK(Render::Items::Poseidon::EaglePerchReturnBone < PlayerBoneCount);
}

TEST_CASE("Mount and pet models stay inside the model table and off the Celestial set")
{
    CHECK(MODEL_POSEIDON_HORSE_ITEM >= 0);
    CHECK(MODEL_POSEIDON_HORSE_ITEM < MAX_MODELS);
    CHECK(MODEL_POSEIDON_EAGLE_ITEM >= 0);
    CHECK(MODEL_POSEIDON_EAGLE_ITEM < MAX_MODELS);
    CHECK(MODEL_POSEIDON_HORSE_ITEM != MODEL_DARK_HORSE);
    CHECK(MODEL_POSEIDON_EAGLE_ITEM != MODEL_DARK_SPIRIT);
    CHECK_FALSE(Render::Items::Celestial::IsAuthoredEquipment(MODEL_POSEIDON_HORSE_ITEM));
    CHECK_FALSE(Render::Items::Celestial::IsAuthoredEquipment(MODEL_POSEIDON_EAGLE_ITEM));
}
