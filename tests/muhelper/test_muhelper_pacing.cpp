#include "doctest.h"

#include "MUHelper/MuHelperPacing.h"

#include <cstdint>

using namespace MUHelper::Pacing;

namespace
{
// SetAttackSpeed() (ZzzCharacter.cpp) builds a swing's PlaySpeed as
// base + AttackSpeed * factor, with factor 0.004 below 509 attack speed. These
// are the two shapes the helper actually meets: the one-handed sword/fist
// swings (base 0.25) and the fist swing (base 0.6).
float SwordSwingPlaySpeed(int attackSpeed)
{
    return 0.25f + static_cast<float>(attackSpeed) * 0.004f;
}

// Typical attack action key count in the player BMD.
constexpr int kSwingKeys = 14;
}

TEST_CASE("attack interval falls as attack speed rises")
{
    const std::uint32_t slow = AttackIntervalMs(SwordSwingPlaySpeed(50), kSwingKeys);
    const std::uint32_t medium = AttackIntervalMs(SwordSwingPlaySpeed(200), kSwingKeys);
    const std::uint32_t fast = AttackIntervalMs(SwordSwingPlaySpeed(400), kSwingKeys);

    CHECK(slow > medium);
    CHECK(medium > fast);
}

TEST_CASE("attack interval is never faster than the floor")
{
    // Far beyond any reachable attack speed.
    CHECK(AttackIntervalMs(100.0f, kSwingKeys) == kMinAttackIntervalMs);
    CHECK(AttackIntervalMs(SwordSwingPlaySpeed(400), kSwingKeys) >= kMinAttackIntervalMs);
}

TEST_CASE("attack interval is capped so the timer never stalls the bot")
{
    CHECK(AttackIntervalMs(0.01f, kSwingKeys) == kMaxAttackIntervalMs);
}

TEST_CASE("attack interval falls back when the animation is unknown")
{
    CHECK(AttackIntervalMs(0.0f, kSwingKeys) == kDefaultAttackIntervalMs);
    CHECK(AttackIntervalMs(-1.0f, kSwingKeys) == kDefaultAttackIntervalMs);
    CHECK(AttackIntervalMs(SwordSwingPlaySpeed(100), 1) == kDefaultAttackIntervalMs);
    CHECK(AttackIntervalMs(SwordSwingPlaySpeed(100), 0) == kDefaultAttackIntervalMs);
}

TEST_CASE("a mid-range character is paced well under the 250 ms helper tick rate")
{
    // The helper timer fires every 250 ms. Without this gate that tick rate was
    // the only pacing the attack loop had, which is faster than the swing a
    // 200-attack-speed character can actually play.
    CHECK(AttackIntervalMs(SwordSwingPlaySpeed(200), kSwingKeys) > 250);
}

TEST_CASE("stall backoff grows exponentially and then plateaus")
{
    CHECK(StallBackoffMs(0) == 0);
    CHECK(StallBackoffMs(1) == 1000);
    CHECK(StallBackoffMs(2) == 2000);
    CHECK(StallBackoffMs(3) == 4000);
    CHECK(StallBackoffMs(4) == 8000);
    CHECK(StallBackoffMs(5) == 8000);
    CHECK(StallBackoffMs(50) == 8000);
}

TEST_CASE("stall backoff ignores nonsense cycle counts")
{
    CHECK(StallBackoffMs(-1) == 0);
}
