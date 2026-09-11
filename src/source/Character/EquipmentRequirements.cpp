#include "stdafx.h"
#include "Character/EquipmentRequirements.h"

#include <algorithm>
#include "Character/EquipmentCatalogCache.h"
#include "Engine/Object/ZzzInfomation.h"

namespace Character::Equipment
{
    ITEM_ATTRIBUTE DisplayRequirements(int itemType)
    {
        auto attributes = ItemAttribute[itemType];
        const auto& catalog = GetCatalog();
        constexpr unsigned GrandMasterCatalogClass = 3;
        constexpr BYTE ThirdEvolution = 3;
        if (catalog.Contains(itemType) && catalog.RequiredClass == GrandMasterCatalogClass)
        {
            std::fill(std::begin(attributes.RequireClass), std::end(attributes.RequireClass), BYTE{0});
            attributes.RequireClass[CLASS_WIZARD] = ThirdEvolution;
        }
        attributes.RequireLevel = static_cast<WORD>(catalog.DisplayLevel(itemType, attributes.RequireLevel));
        return attributes;
    }

    bool MatchesDisplayedClass(int itemType, int baseClass, int classStep)
    {
        if (baseClass < 0 || baseClass >= MAX_CLASS)
            return false;
        const auto attributes = DisplayRequirements(itemType);
        const auto required = attributes.RequireClass[baseClass];
        const bool hybrid = baseClass == CLASS_DARK && attributes.RequireClass[CLASS_WIZARD]
            && attributes.RequireClass[CLASS_KNIGHT];
        return (required != 0 || hybrid) && required <= classStep;
    }

    unsigned DisplayLevel(int itemType, unsigned legacyLevel)
    {
        return GetCatalog().DisplayLevel(itemType, legacyLevel);
    }
}
