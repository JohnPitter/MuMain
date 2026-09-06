#pragma once

// Pure pacing rules for the Mu Helper: how far apart two outgoing attack
// requests have to be, and how long the bot backs off from a locked target it
// keeps failing to reach.
//
// Everything here is a pure function of its inputs so it can be unit tested
// (tests/muhelper/test_muhelper_pacing.cpp). MuHelper.cpp owns the game state
// and the packets; this header owns only the arithmetic.

#include <cstdint>

namespace MUHelper::Pacing
{
// BMD::PlayAnimation() (ZzzBMD.cpp) advances an action by
// PlaySpeed * FPS_ANIMATION_FACTOR per rendered frame, and FPS_ANIMATION_FACTOR
// is normalized against this reference rate (DefaultCamera.cpp:
// dt = FPS_ANIMATION_FACTOR / 25). A swing therefore takes the same wall time
// whatever the frame rate, which makes the animation a usable clock.
inline constexpr float kAnimationFps = 25.0f;

// Bounds for the derived attack interval. The floor mirrors the fastest swing
// the client can play at the top of the attack-speed range; the ceiling bounds
// only the *timer*, because a slower character is still held back by the swing
// animation itself, which MuHelper checks separately.
inline constexpr std::uint32_t kMinAttackIntervalMs = 120;
inline constexpr std::uint32_t kMaxAttackIntervalMs = 1200;

// Used until the first swing animation of this character has been observed.
// Roughly a 100-attack-speed one-handed swing, i.e. deliberately slow.
inline constexpr std::uint32_t kDefaultAttackIntervalMs = 400;

// Minimum wall time between two attack requests, taken from the very animation
// the manual attack path plays. SetAttackSpeed() (ZzzCharacter.cpp) rescales
// every swing action's PlaySpeed from CharacterAttribute->AttackSpeed /
// MagicSpeed, so the swing length *is* the character's real attack cadence.
// Attack actions are LockPositions actions, which stop one key short of the
// end, hence animationKeys - 1.
inline std::uint32_t AttackIntervalMs(float playSpeed, int animationKeys)
{
    if (playSpeed <= 0.0f || animationKeys <= 1)
    {
        return kDefaultAttackIntervalMs;
    }

    const float fKeys = static_cast<float>(animationKeys - 1);
    const float fMs = (fKeys / (playSpeed * kAnimationFps)) * 1000.0f;

    if (fMs <= static_cast<float>(kMinAttackIntervalMs))
    {
        return kMinAttackIntervalMs;
    }
    if (fMs >= static_cast<float>(kMaxAttackIntervalMs))
    {
        return kMaxAttackIntervalMs;
    }
    return static_cast<std::uint32_t>(fMs);
}

// Exponential backoff applied after each exhausted recovery cycle on a locked
// target that cannot be reached: 1 s, 2 s, 4 s, then 8 s for every further
// cycle. Short enough that a mob merely body-blocked for a moment costs almost
// nothing, long enough that a permanently unreachable mob stops producing
// packets while the give-up rule runs its course.
inline constexpr std::uint32_t kStallBackoffBaseMs = 1000;
inline constexpr int kMaxStallBackoffShift = 3;

inline std::uint32_t StallBackoffMs(int stallCycles)
{
    if (stallCycles <= 0)
    {
        return 0;
    }

    const int iShift = (stallCycles - 1) < kMaxStallBackoffShift
        ? (stallCycles - 1)
        : kMaxStallBackoffShift;
    return kStallBackoffBaseMs << iShift;
}
}
