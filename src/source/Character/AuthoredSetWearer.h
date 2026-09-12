#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include "Core/Globals/_enum.h"

// The E7 v2 catalog carries the server's CharacterClass.Number of the class
// that wears the family (see MUnique.OpenMU.GameLogic.Equipment.EquipmentSetPhases;
// the numbers themselves are SERVER_CLASS_TYPE in _enum.h). The client stores
// equip requirements in ITEM_ATTRIBUTE::RequireClass,
// indexed by *base* class, and compares the stored value against
// CCharacterManager::GetStepClass - so the server number has to be translated
// into a (base class, evolution step) pair before it can gate an equip.
//
// Kept as a pure header, like Character::Cloth::ClothBaseClass, so the mapping
// is provable without linking the character runtime. The base classes mirror
// the folds of CCharacterManager::GetBaseClass; if the server ever renumbers a
// class, this table moves with it.
namespace Character::Equipment
{
    // CCharacterManager::GetStepClass reports 3 for every third class, and all
    // three authored families are third-class only.
    constexpr std::uint8_t ThirdEvolution = 3;

    struct AuthoredWearer
    {
        SERVER_CLASS_TYPE CatalogClass;
        CLASS_TYPE BaseClass;
        std::uint8_t Step;
    };

    constexpr AuthoredWearer AuthoredWearers[] =
    {
        { GrandMaster, CLASS_WIZARD, ThirdEvolution },    // Celestial
        { DuelMaster, CLASS_DARK, ThirdEvolution },       // Zeus
        { LordEmperor, CLASS_DARK_LORD, ThirdEvolution }, // Poseidon
    };

    // Returns the wearer of the family the catalog describes, or nullptr when
    // the catalog belongs to no authored family (an ordinary set, or a class
    // this client build does not know).
    constexpr const AuthoredWearer* FindAuthoredWearer(unsigned catalogClass)
    {
        for (const auto& wearer : AuthoredWearers)
        {
            if (wearer.CatalogClass == catalogClass)
            {
                return &wearer;
            }
        }

        return nullptr;
    }

    // Rewrites an ITEM_ATTRIBUTE::RequireClass array so the family's wearer -
    // and only the wearer - can pass the equip gate. Leaves the array untouched
    // and answers false when the catalog belongs to no authored family.
    template <typename ByteType, std::size_t Count>
    constexpr bool ApplyAuthoredRequirement(ByteType (&requireClass)[Count], unsigned catalogClass)
    {
        const AuthoredWearer* wearer = FindAuthoredWearer(catalogClass);
        if (wearer == nullptr || static_cast<std::size_t>(wearer->BaseClass) >= Count)
        {
            return false;
        }

        for (std::size_t index = 0; index < Count; ++index)
        {
            requireClass[index] = ByteType{0};
        }

        requireClass[static_cast<std::size_t>(wearer->BaseClass)] = static_cast<ByteType>(wearer->Step);
        return true;
    }

    // The class half of the client-side equip gate (CNewUIMyInventory::IsEquipable
    // and IsRequireEquipItem both go through MatchesDisplayedClass, which calls
    // this). The hybrid rule is the historical Magic Gladiator allowance: it may
    // wear what both the wizard and the knight line can wear.
    template <typename ByteType, std::size_t Count>
    constexpr bool ClassPassesRequirement(const ByteType (&requireClass)[Count], int baseClass, int classStep)
    {
        if (baseClass < 0 || static_cast<std::size_t>(baseClass) >= Count)
        {
            return false;
        }

        const auto required = requireClass[static_cast<std::size_t>(baseClass)];
        const bool hybrid = baseClass == CLASS_DARK
            && requireClass[CLASS_WIZARD] != ByteType{0}
            && requireClass[CLASS_KNIGHT] != ByteType{0};
        return (required != ByteType{0} || hybrid) && required <= classStep;
    }
}
