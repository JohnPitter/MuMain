#pragma once

#include <cstdint>

namespace Character::Equipment
{
    struct BonusPhase
    {
        std::uint16_t MemberMask = 0;
        std::uint8_t Percent = 0;
    };
}
