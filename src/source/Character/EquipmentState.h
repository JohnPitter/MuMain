#pragma once

namespace Character::Equipment
{
    struct State
    {
        bool Known = false;
        bool CelestialActive = false;
        float MovementFactor = 1.0f;

        bool HasCelestialAura() const { return Known && CelestialActive; }
    };
}
