#pragma once

#include <array>
#include "Character/EquipmentBonusPhase.h"
#include "Character/EquipmentCategory.h"
#include "Character/EquipmentSetBonus.h"
#include "Character/EquipmentSetMember.h"

namespace Character::Equipment
{
    constexpr std::size_t MaximumCatalogEntries = 16;
    constexpr std::size_t MaximumBonusPhases = 3;

    struct BonusCatalog
    {
        bool Known = false;
        std::uint8_t RequiredItems = 0;
        std::uint8_t MinimumUpgrade = 0;
        std::uint8_t MemberCount = 0;
        std::uint8_t BonusCount = 0;
        Category ItemCategory = Category::Normal;
        std::uint16_t RequiredLevel = 0;
        std::uint8_t RequiredClass = 0;
        std::uint8_t PhaseCount = 0;
        std::array<SetMember, MaximumCatalogEntries> Members{};
        std::array<SetBonus, MaximumCatalogEntries> Bonuses{};
        std::array<BonusPhase, MaximumBonusPhases> Phases{};

        bool Contains(int itemType) const;
        bool IsUltimate(int itemType) const;
        unsigned DisplayLevel(int itemType, unsigned legacyLevel) const;
    };
}
