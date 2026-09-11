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
}
