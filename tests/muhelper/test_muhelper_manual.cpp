#include "doctest.h"

#include "MUHelper/MuHelperManualControl.h"

#include <cstdint>

using namespace MUHelper::Manual;

namespace
{
// The helper work loop runs on a fixed 250 ms timer (Winmain.cpp,
// MUHELPER_TIMER), so a tick is the natural unit for these tests.
constexpr std::uint32_t kTickMs = 250;

// Runs `ticks` helper ticks from `now`, reporting the hero as walking or not,
// and returns the tick the last one happened at.
std::uint32_t RunTicks(State& state, bool heroWalking, std::uint32_t now, int ticks)
{
    for (int i = 0; i < ticks; ++i)
    {
        now += kTickMs;
        Update(state, heroWalking, now);
    }
    return now;
}
}

TEST_CASE("a fresh state never suppresses the helper")
{
    State state;

    CHECK_FALSE(IsSuppressed(state, 0));
    CHECK_FALSE(IsSuppressed(state, 10 * 1000));

    // A tick without any manual input changes nothing.
    Update(state, false, 1000);
    CHECK_FALSE(IsSuppressed(state, 1000));

    // Not even a walk the bot itself started counts as manual input.
    Update(state, true, 2000);
    CHECK_FALSE(IsSuppressed(state, 2000));
}

TEST_CASE("a click suppresses the helper immediately")
{
    State state;
    Claim(state, 1000);

    CHECK(IsSuppressed(state, 1000));
    CHECK(IsSuppressed(state, 1000 + kTickMs));
}

TEST_CASE("the claim holds for the whole walk the click started")
{
    State state;
    Claim(state, 1000);

    // Ten seconds of walking: far longer than the grace period on its own.
    const std::uint32_t now = RunTicks(state, true, 1000, 40);

    CHECK(IsSuppressed(state, now));
}

TEST_CASE("the helper resumes a grace period after the walk ends")
{
    State state;
    Claim(state, 1000);
    std::uint32_t now = RunTicks(state, true, 1000, 4);
    REQUIRE(IsSuppressed(state, now));

    // The path ends: the hero stops walking on this tick.
    now += kTickMs;
    Update(state, false, now);
    const std::uint32_t released = now;

    CHECK(IsSuppressed(state, released));
    CHECK(IsSuppressed(state, released + kGraceMs - 1));
    CHECK_FALSE(IsSuppressed(state, released + kGraceMs));
    CHECK_FALSE(IsSuppressed(state, released + kGraceMs + kTickMs));
}

TEST_CASE("a click that starts no walk still yields the grace period")
{
    // Clicking an NPC or a mob in range moves nothing, but the player is still
    // driving and must not be interrupted by the next helper tick.
    State state;
    Claim(state, 1000);

    const std::uint32_t now = 1000 + kTickMs;
    Update(state, false, now);

    CHECK(IsSuppressed(state, now));
    CHECK(IsSuppressed(state, now + kGraceMs - 1));
    CHECK_FALSE(IsSuppressed(state, now + kGraceMs));
}

TEST_CASE("a new click during the grace period extends the claim")
{
    State state;
    Claim(state, 1000);
    Update(state, false, 1000 + kTickMs);
    const std::uint32_t released = 1000 + kTickMs;
    REQUIRE(IsSuppressed(state, released + kGraceMs - 1));

    // The player clicks again just before the helper would have resumed.
    const std::uint32_t reclaimed = released + kGraceMs - 1;
    Claim(state, reclaimed);

    CHECK(IsSuppressed(state, reclaimed + kGraceMs));
    Update(state, false, reclaimed + kTickMs);
    CHECK(IsSuppressed(state, reclaimed + kTickMs + kGraceMs - 1));
    CHECK_FALSE(IsSuppressed(state, reclaimed + kTickMs + kGraceMs));
}

TEST_CASE("a stuck movement flag cannot disable the helper forever")
{
    // Hero->Movement is a plain flag; a desync can leave it set with no path.
    // The hold budget is what guarantees the bot comes back.
    State state;
    Claim(state, 1000);

    const std::uint32_t stillHeld = 1000 + kMaxHoldMs - kTickMs;
    Update(state, true, stillHeld);
    CHECK(IsSuppressed(state, stillHeld));

    const std::uint32_t expired = 1000 + kMaxHoldMs;
    Update(state, true, expired);
    CHECK_FALSE(state.claimed);
    // Releasing on the cap still grants the ordinary grace period.
    CHECK(IsSuppressed(state, expired));
    CHECK_FALSE(IsSuppressed(state, expired + kGraceMs));
}

TEST_CASE("holding the button keeps restarting the hold budget")
{
    // A held left button re-claims on every frame, so a player who keeps
    // steering is never cut off by the cap.
    State state;
    std::uint32_t now = 1000;
    for (int i = 0; i < 200; ++i)
    {
        Claim(state, now);
        now += kTickMs;
        Update(state, true, now);
        REQUIRE(IsSuppressed(state, now));
    }

    CHECK(now - 1000 > kMaxHoldMs);
    CHECK(IsSuppressed(state, now));
}

TEST_CASE("stopping the helper clears the claim")
{
    State state;
    Claim(state, 1000);
    REQUIRE(IsSuppressed(state, 1000));

    Reset(state);

    CHECK_FALSE(IsSuppressed(state, 1000));
    CHECK_FALSE(IsSuppressed(state, 1000 + kGraceMs));
}

TEST_CASE("the claim survives the tick counter wrapping around")
{
    // GetTickCount() wraps every ~49.7 days; the arithmetic must stay unsigned.
    constexpr std::uint32_t kBeforeWrap = 0xFFFFFF00u;
    State state;
    Claim(state, kBeforeWrap);

    const std::uint32_t afterWrap = kBeforeWrap + 2 * kTickMs; // wraps past 0
    REQUIRE(afterWrap < kBeforeWrap);

    Update(state, true, afterWrap);
    CHECK(IsSuppressed(state, afterWrap));

    Update(state, false, afterWrap);
    CHECK(IsSuppressed(state, afterWrap));
    CHECK_FALSE(IsSuppressed(state, afterWrap + kGraceMs));
}
