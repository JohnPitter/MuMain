#pragma once

#include "Core/Globals/_enum.h"
#include "Core/Globals/_define.h"

// Authored Poseidon mount and pet contracts (cavalo 13/205, águia 13/206).
// Both models ship under their structural item addresses (MODEL_HELPER + 205 /
// +206) and are activated by behaviour instead of renames: the horse mounts
// through the GOBoid mount table with the native DarkHorse ride contract, the
// eagle follows the CSPetSystem flight states.
namespace Render::Items::Poseidon
{
    // Item IDs on the helper group (13). Numerically equal to the model
    // offsets (MODEL_POSEIDON_HORSE_ITEM - MODEL_ITEM), which is the space
    // ITEM::Type and the legacy equipment-change handler switch on.
    constexpr int HorseItemId = ITEM_HELPER + 205;
    constexpr int EagleItemId = ITEM_HELPER + 206;

    // Horse action slots. The authored BMD preserves the native
    // MODEL_DARK_HORSE animation block bit for bit; the slot names are the
    // ones evidenced by the GOBoid.cpp MODEL_DARK_HORSE branch and frozen in
    // art-source/poseidon/prototype/mount-actions-report.json.
    enum HorseAction
    {
        HorseActionStand = 0,
        HorseActionGallop = 1,
        HorseActionRiderStrike = 2,
        HorseActionEarthshake = 3,
        HorseActionIdle1 = 5,
        HorseActionIdle2 = 6,
    };

    // Eagle action slots per the CSPetSystem (darkspirit) semantics, frozen in
    // art-source/poseidon/prototype/eagle-actions-report.json.
    enum EagleAction
    {
        EagleActionFly = 0,
        EagleActionFlying = 1,
        EagleActionStand = 2,
        EagleActionEscape = 3,
    };

    // Player-skeleton anchors for the shoulder perch (the darkspirit perches
    // on bone 37 and glides home to bone 42; the eagle reuses the exact
    // contract so the pet never intersects the rider's body).
    constexpr int EaglePerchBone = 37;
    constexpr int EaglePerchReturnBone = 42;

    inline bool IsMountModel(int model)
    {
        return model == MODEL_POSEIDON_HORSE_ITEM;
    }

    inline bool IsPetModel(int model)
    {
        return model == MODEL_POSEIDON_EAGLE_ITEM;
    }

    // The authored horse rides exactly like the native Dark Horse: the rider
    // plays the same stop/run/attack ride animations and the earthshake skill
    // keeps its dedicated rider swing. Native behaviour keyed on
    // MODEL_DARK_HORSE_ITEM therefore accepts the authored item too.
    inline bool IsDarkHorseRideHelper(int helperType)
    {
        return helperType == MODEL_DARK_HORSE_ITEM || helperType == MODEL_POSEIDON_HORSE_ITEM;
    }
}
