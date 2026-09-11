#pragma once

#include <cstdint>
#include "Character/EquipmentState.h"

namespace Network::Equipment
{
    struct StateUpdate
    {
        std::uint16_t CharacterId = 0;
        Character::Equipment::State Equipment;
        std::uint16_t AttackSpeed = 0;
        std::uint16_t MagicSpeed = 0;
    };
}
