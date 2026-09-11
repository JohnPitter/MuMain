#pragma once

#include <cstdint>
#include "Character/EquipmentState.h"

namespace Character::Movement
{
    enum Effect : std::uint8_t
    {
        None = 0,
        Frozen = 1,
        DestructionSlow = 2,
        TempleQuickness = 4,
    };

    float ApplyEffects(float baseSpeed, const Equipment::State& state, std::uint8_t legacyEffects);
}
