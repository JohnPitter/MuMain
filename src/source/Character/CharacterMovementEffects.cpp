#include "Character/CharacterMovementEffects.h"

#include <cmath>

namespace Character::Movement
{
    namespace
    {
        constexpr float FrozenFactor = 0.5f;
        constexpr float DestructionFactor = 0.33f;
        constexpr float QuicknessSpeed = 20.0f;
    }

    float ApplyEffects(float baseSpeed, const Equipment::State& state, std::uint8_t legacyEffects)
    {
        if ((legacyEffects & TempleQuickness) != 0)
            baseSpeed = QuicknessSpeed;

        if (state.Known)
        {
            // The server's final factor already contains freeze/slow and equipment bonuses.
            const float speed = baseSpeed * state.MovementFactor;
            return std::isfinite(speed) && speed > 0 ? speed : baseSpeed;
        }

        if ((legacyEffects & TempleQuickness) != 0)
            return baseSpeed;
        if ((legacyEffects & Frozen) != 0)
            return baseSpeed * FrozenFactor;
        if ((legacyEffects & DestructionSlow) != 0)
            return baseSpeed * DestructionFactor;
        return baseSpeed;
    }
}
