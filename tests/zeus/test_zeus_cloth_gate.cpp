#include "doctest.h"

#include "Character/ClothGate.h"

// Proves the bCloak class gate (RenderCharacter, ZzzCharacter.cpp) keeps its
// historical behavior for every pre-existing class and opens only for the Duel
// Master evolution. The runtime gate is Character::Cloth::ClassOpensCloakGate;
// the constexpr ClothBaseClass fold mirrors the CLASS_DARK-family rows of
// CCharacterManager::GetBaseClass (see both notes) so the policy is provable
// without linking the character runtime.
using Character::Cloth::ClassOpensCloakGate;
using Character::Cloth::ClothBaseClass;

TEST_CASE("The cloth gate keeps the historical families unchanged")
{
    // Raw Magic Gladiator: CLASS_DARK literal, gate open since forever.
    CHECK(ClassOpensCloakGate(CLASS_DARK));
    // Dark Lord family: base class CLASS_DARK_LORD.
    CHECK(ClassOpensCloakGate(CLASS_DARK_LORD));
    CHECK(ClassOpensCloakGate(CLASS_LORDEMPEROR));
    // Rage Fighter family: base class CLASS_RAGEFIGHTER.
    CHECK(ClassOpensCloakGate(CLASS_RAGEFIGHTER));
    CHECK(ClassOpensCloakGate(CLASS_TEMPLENIGHT));
}

TEST_CASE("Every other class stays outside the cloth gate")
{
    CHECK_FALSE(ClassOpensCloakGate(CLASS_WIZARD));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_KNIGHT));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_ELF));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_SUMMONER));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_SOULMASTER));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_BLADEKNIGHT));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_MUSEELF));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_BLOODYSUMMONER));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_GRANDMASTER));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_BLADEMASTER));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_HIGHELF));
    CHECK_FALSE(ClassOpensCloakGate(CLASS_DIMENSIONMASTER));
}

TEST_CASE("The gate opens for the Duel Master through its CLASS_DARK base")
{
    CHECK(ClothBaseClass(CLASS_DUELMASTER) == CLASS_DARK);
    CHECK(ClassOpensCloakGate(CLASS_DUELMASTER));
}

TEST_CASE("The base class fold mirrors GetBaseClass on the cloth-relevant rows only")
{
    CHECK(ClothBaseClass(CLASS_LORDEMPEROR) == CLASS_DARK_LORD);
    CHECK(ClothBaseClass(CLASS_TEMPLENIGHT) == CLASS_RAGEFIGHTER);
    // Identity rows: GetBaseClass returns those classes unchanged, so the gate
    // verdict is identical whether asked directly or through the fold.
    for (const CLASS_TYPE identity : {CLASS_WIZARD, CLASS_KNIGHT, CLASS_ELF,
        CLASS_DARK, CLASS_DARK_LORD, CLASS_SUMMONER, CLASS_RAGEFIGHTER})
    {
        CHECK(ClothBaseClass(identity) == identity);
    }
}
