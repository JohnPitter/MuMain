#include "Character/EquipmentBonusCatalog.h"

namespace Character::Equipment
{
    bool BonusCatalog::Contains(int itemType) const
    {
        if (!Known || itemType < 0 || MemberCount > Members.size())
            return false;
        for (std::size_t index = 0; index < MemberCount; ++index)
            if (Members[index].ItemType == itemType)
                return true;
        return false;
    }

    bool BonusCatalog::IsUltimate(int itemType) const
    {
        return ItemCategory == Category::Ultimate && Contains(itemType);
    }

    unsigned BonusCatalog::DisplayLevel(int itemType, unsigned legacyLevel) const
    {
        return RequiredLevel > 0 && Contains(itemType) ? RequiredLevel : legacyLevel;
    }
}
