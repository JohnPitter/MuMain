#include "doctest.h"

#include <cstdint>
#include <map>
#include <windows.h>
#include "Core/Globals/_define.h"
#include "Character/AuthoredSetWearer.h"

// The equip gate the client applies before any packet reaches the server
// (CNewUIMyInventory::IsEquipable and IsRequireEquipItem both call
// MatchesDisplayedClass, which is these two functions). Item.bmd carries no
// rows for the authored families, so without the catalog rewrite their
// RequireClass is all zeros and nobody can wear them - which is exactly how
// Poseidon and Zeus shipped: granted, visible in the vault, impossible to
// equip, and with no trace in the server log because the refusal is local.
using Character::Equipment::ApplyAuthoredRequirement;
using Character::Equipment::ClassPassesRequirement;
using Character::Equipment::FindAuthoredWearer;
using Character::Equipment::ThirdEvolution;

namespace
{
    // CCharacterManager::GetStepClass answers 1 for a first class and 3 for a
    // third class; the second classes sit in between.
    constexpr int FirstClass = 1;
    constexpr int SecondClass = 2;
    constexpr int ThirdClass = 3;

    struct Requirements
    {
        BYTE RequireClass[MAX_CLASS] = {};
    };

    Requirements CatalogOf(SERVER_CLASS_TYPE catalogClass)
    {
        Requirements requirements{};
        ApplyAuthoredRequirement(requirements.RequireClass, catalogClass);
        return requirements;
    }
}

TEST_CASE("Each authored family names the class that wears it")
{
    REQUIRE(FindAuthoredWearer(GrandMaster) != nullptr);
    CHECK(FindAuthoredWearer(GrandMaster)->BaseClass == CLASS_WIZARD);

    REQUIRE(FindAuthoredWearer(DuelMaster) != nullptr);
    CHECK(FindAuthoredWearer(DuelMaster)->BaseClass == CLASS_DARK);

    REQUIRE(FindAuthoredWearer(LordEmperor) != nullptr);
    CHECK(FindAuthoredWearer(LordEmperor)->BaseClass == CLASS_DARK_LORD);

    for (const auto wearer : { GrandMaster, DuelMaster, LordEmperor })
    {
        CHECK(FindAuthoredWearer(wearer)->Step == ThirdEvolution);
    }
}

TEST_CASE("A catalog of no authored family leaves the requirements untouched")
{
    Requirements requirements{};
    requirements.RequireClass[CLASS_KNIGHT] = 1;

    CHECK_FALSE(ApplyAuthoredRequirement(requirements.RequireClass, 0));
    CHECK_FALSE(ApplyAuthoredRequirement(requirements.RequireClass, 4));
    CHECK_FALSE(ApplyAuthoredRequirement(requirements.RequireClass, 25));
    CHECK(requirements.RequireClass[CLASS_KNIGHT] == 1);
}

TEST_CASE("The Celestial gate keeps opening for the Grand Master alone")
{
    const auto celestial = CatalogOf(GrandMaster);

    CHECK(ClassPassesRequirement(celestial.RequireClass, CLASS_WIZARD, ThirdClass));
    // Dark Wizard and Soul Master are the same base class at a lower step.
    CHECK_FALSE(ClassPassesRequirement(celestial.RequireClass, CLASS_WIZARD, FirstClass));
    CHECK_FALSE(ClassPassesRequirement(celestial.RequireClass, CLASS_WIZARD, SecondClass));
    CHECK_FALSE(ClassPassesRequirement(celestial.RequireClass, CLASS_KNIGHT, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(celestial.RequireClass, CLASS_DARK_LORD, ThirdClass));
}

TEST_CASE("The Zeus gate opens for the Duel Master and no earlier")
{
    const auto zeus = CatalogOf(DuelMaster);

    CHECK(ClassPassesRequirement(zeus.RequireClass, CLASS_DARK, ThirdClass));
    // A Magic Gladiator that has not taken the third class stays out, the same
    // way a Soul Master stays out of the Celestial set.
    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, CLASS_DARK, FirstClass));
    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, CLASS_DARK, SecondClass));
    // The hybrid allowance must not leak the set to the Magic Gladiator: it
    // only applies when both the wizard and the knight line qualify.
    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, CLASS_WIZARD, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, CLASS_KNIGHT, ThirdClass));
}

TEST_CASE("The Poseidon gate opens for the Lord Emperor and no earlier")
{
    const auto poseidon = CatalogOf(LordEmperor);

    CHECK(ClassPassesRequirement(poseidon.RequireClass, CLASS_DARK_LORD, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(poseidon.RequireClass, CLASS_DARK_LORD, FirstClass));
    CHECK_FALSE(ClassPassesRequirement(poseidon.RequireClass, CLASS_DARK_LORD, SecondClass));
    CHECK_FALSE(ClassPassesRequirement(poseidon.RequireClass, CLASS_DARK, ThirdClass));
}

TEST_CASE("The families never open each other's gates")
{
    const auto celestial = CatalogOf(GrandMaster);
    const auto zeus = CatalogOf(DuelMaster);
    const auto poseidon = CatalogOf(LordEmperor);

    CHECK_FALSE(ClassPassesRequirement(celestial.RequireClass, CLASS_DARK, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(celestial.RequireClass, CLASS_DARK_LORD, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, CLASS_DARK_LORD, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(poseidon.RequireClass, CLASS_WIZARD, ThirdClass));
}

TEST_CASE("An out-of-range base class is refused instead of read")
{
    const auto zeus = CatalogOf(DuelMaster);

    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, -1, ThirdClass));
    CHECK_FALSE(ClassPassesRequirement(zeus.RequireClass, MAX_CLASS, ThirdClass));
}

TEST_CASE("Without the rewrite nobody can wear an authored item")
{
    // This is the shipped state: Item.bmd has no row for the new indices, so
    // every class requirement is zero.
    const Requirements untouched{};

    for (int baseClass = 0; baseClass < MAX_CLASS; ++baseClass)
    {
        CHECK_FALSE(ClassPassesRequirement(untouched.RequireClass, baseClass, ThirdClass));
    }
}
