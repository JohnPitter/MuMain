#pragma once

// Pure threshold rule shared by the HP and MP auto-potion checks in
// MuHelper.cpp (CMuHelper::TryUseHealthPotion / TryUseManaPotion).
//
// Everything here is a pure function of its inputs so it can be unit tested
// (tests/muhelper/test_muhelper_potion_threshold.cpp). MuHelper.cpp owns the
// game state (CharacterAttribute, inventory lookup, the packet, the shared
// request cooldown); this header owns only the percentage arithmetic and the
// enabled/threshold decision.

#include <cstdint>

namespace MUHelper::Potion
{
// Ceiling percentage remaining, matching the ConsumePotion() HP check this
// was extracted from: (current * 100 + max - 1) / max. Using a ceiling
// instead of plain truncating division means a threshold of e.g. 40% trips
// as soon as the bar visibly reads 40%, not only once it has already fallen
// to 39% because integer division rounded 40.0% down.
inline std::int64_t RemainingPercent(std::int64_t current, std::int64_t max)
{
    if (max <= 0)
    {
        return 100;
    }

    return (current * 100 + max - 1) / max;
}

// Whether a potion should be requested right now: the group's "Auto Potion"
// checkbox must be on, both stat values must be sane (a dead/uninitialized
// resource -- max <= 0 or current <= 0 -- never triggers a request), and the
// remaining percentage must have fallen to the configured threshold or below.
// HP and MP share this exact rule; only the inputs differ (see MuHelper.cpp).
inline bool ShouldUsePotion(bool enabled, std::int64_t current, std::int64_t max, int thresholdPercent)
{
    if (!enabled || max <= 0 || current <= 0)
    {
        return false;
    }

    return RemainingPercent(current, max) <= thresholdPercent;
}
}
