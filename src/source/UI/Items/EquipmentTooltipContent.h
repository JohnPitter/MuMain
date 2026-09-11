#pragma once

#include <span>
#include "Character/EquipmentBonusCatalog.h"
#include "Character/EquipmentState.h"
#include "UI/Items/EquipmentTooltipLine.h"
#include "UI/Items/EquipmentTooltipStrings.h"

namespace UI::Items::EquipmentTooltip
{
    Line BuildStatus(const Character::Equipment::State& state, const Strings& strings);
    std::size_t BuildLines(const Character::Equipment::BonusCatalog& catalog,
        const Character::Equipment::State& state, const Strings& strings, std::span<Line> lines);
}
