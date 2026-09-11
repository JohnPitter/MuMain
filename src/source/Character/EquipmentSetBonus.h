#pragma once

#include <cstdint>

namespace Character::Equipment
{
    enum class BonusKind : std::uint8_t
    {
        Damage = 1, Defense, Health, Mana, AttackSpeed, MagicSpeed, Movement,
        IgnoreDefense, CriticalChance, HealthRecovery, ManaRecovery, ElementalProtection
    };

    enum class BonusUnit : std::uint8_t { Flat, Percent, PercentagePoints };

    struct SetBonus
    {
        BonusKind Kind{};
        BonusUnit Unit{};
        float Value = 0;
    };
}
