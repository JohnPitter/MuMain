#pragma once

#include <cstdint>

namespace Character::Equipment
{
    struct SetMember
    {
        std::uint16_t ItemType = 0;
        std::uint8_t Quantity = 0;
    };
}
