#pragma once

#include <array>
#include "Character/EquipmentSetBonus.h"
#include "Character/EquipmentSetMember.h"

namespace Character::Equipment
{
    constexpr std::size_t MaximumCatalogEntries = 16;

    struct BonusCatalog
    {
        bool Known = false;
        std::uint8_t RequiredItems = 0;
        std::uint8_t MinimumUpgrade = 0;
        std::uint8_t MemberCount = 0;
        std::uint8_t BonusCount = 0;
        std::array<SetMember, MaximumCatalogEntries> Members{};
        std::array<SetBonus, MaximumCatalogEntries> Bonuses{};

        bool Contains(int itemType) const;
    };
}
