#pragma once

#include "Character/EquipmentBonusCatalog.h"

namespace Character::Equipment
{
    const BonusCatalog& GetCatalog();
    void SetCatalog(const BonusCatalog& catalog);
    void ResetCatalog();
}
