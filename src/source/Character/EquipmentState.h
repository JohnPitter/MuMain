#pragma once

#include <cstdint>

namespace Character::Equipment
{
    struct State
    {
        bool Known = false;
        bool CelestialActive = false;
        float MovementFactor = 1.0f;
        std::uint8_t BonusPercent = 100;

        bool HasCelestialAura() const { return Known && CelestialActive && BonusPercent > 0; }
        unsigned ActivePercent() const { return HasCelestialAura() ? BonusPercent : 0; }
    };
}
