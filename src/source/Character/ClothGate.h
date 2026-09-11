#pragma once

#include "Core/Globals/_enum.h"

// Pure policy of the bCloak class gate in RenderCharacter (ZzzCharacter.cpp).
// Historically the gate opened for the raw Magic Gladiator (CLASS_DARK
// literal) and for the Dark Lord and Rage Fighter families; the Duel Master
// evolution joins through its base class CLASS_DARK so it can wear the
// authored Zeus cape cloth (cape-cloth-contract.md, section 4).
//
// ClothBaseClass mirrors the CLASS_DARK-family rows of
// CCharacterManager::GetBaseClass (CharacterManager.cpp): DuelMaster folds to
// CLASS_DARK, LordEmperor to CLASS_DARK_LORD, TempleNight to
// CLASS_RAGEFIGHTER, everything else is identity. It is kept as a constexpr
// twin so tests can prove gate semantics without linking the character
// runtime; if GetBaseClass ever changes a fold, this twin must change with it
// (both sides carry this note).
namespace Character::Cloth
{
    constexpr CLASS_TYPE ClothBaseClass(CLASS_TYPE characterClass)
    {
        switch (characterClass)
        {
        case CLASS_DUELMASTER: return CLASS_DARK;
        case CLASS_LORDEMPEROR: return CLASS_DARK_LORD;
        case CLASS_TEMPLENIGHT: return CLASS_RAGEFIGHTER;
        default: return characterClass;
        }
    }

    // The class families that receive procedural cape cloth at all. Adding
    // CLASS_DARK here covers the raw Magic Gladiator (identity fold) and the
    // Duel Master; Dark Lord/Lord Emperor and Rage Fighter/Temple Night keep
    // their pre-existing branches unchanged.
    inline bool ClassOpensCloakGate(CLASS_TYPE characterClass)
    {
        const CLASS_TYPE base = ClothBaseClass(characterClass);
        return base == CLASS_DARK || base == CLASS_DARK_LORD || base == CLASS_RAGEFIGHTER;
    }
}
