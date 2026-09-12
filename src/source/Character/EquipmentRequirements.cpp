#include "stdafx.h"
#include "Character/EquipmentRequirements.h"

#include "Character/AuthoredSetWearer.h"
#include "Character/EquipmentCatalogCache.h"
#include "Engine/Object/ZzzInfomation.h"

namespace Character::Equipment
{
    ITEM_ATTRIBUTE DisplayRequirements(int itemType)
    {
        auto attributes = ItemAttribute[itemType];
        const auto& catalog = GetCatalog();
        // Item.bmd has no rows for the authored families, so their RequireClass
        // arrives as all zeros and MatchesDisplayedClass answers false for every
        // class - the client refuses the equip before the server ever sees it.
        // The catalog knows who wears the family; rewrite the requirement from
        // it. This used to be hard-coded to the Grand Master, which is why only
        // Celestial could be worn.
        if (catalog.Contains(itemType))
        {
            ApplyAuthoredRequirement(attributes.RequireClass, catalog.RequiredClass);
        }
        attributes.RequireLevel = static_cast<WORD>(catalog.DisplayLevel(itemType, attributes.RequireLevel));
        return attributes;
    }

    bool MatchesDisplayedClass(int itemType, int baseClass, int classStep)
    {
        const auto attributes = DisplayRequirements(itemType);
        return ClassPassesRequirement(attributes.RequireClass, baseClass, classStep);
    }

    unsigned DisplayLevel(int itemType, unsigned legacyLevel)
    {
        return GetCatalog().DisplayLevel(itemType, legacyLevel);
    }
}
