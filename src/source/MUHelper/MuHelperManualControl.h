#pragma once

// Manual-control arbitration for the Mu Helper: who owns the hero right now,
// the player or the bot.
//
// The helper tick (250 ms, Winmain.cpp MUHELPER_TIMER) writes Hero->Path,
// Hero->Movement and the hero's action state directly, exactly like the mouse
// handler in MoveHero() does. Nothing told the two apart, so a click issued by
// the player was cancelled and replaced by the bot's own chase path within a
// quarter of a second: the client restarted the walk from a different origin
// while the server was still animating the path it had already accepted, which
// is the "floating"/rubberband desync reported in game.
//
// The rule implemented here is the official one: manual input always wins.
// A real click claims the hero; the claim lasts for the whole walk it started
// and a short grace period afterwards, and the helper emits nothing and touches
// neither the path nor the action state while it holds.
//
// Everything is a pure function of its inputs so it can be unit tested
// (tests/muhelper/test_muhelper_manual.cpp); MuHelper.cpp owns the game state.

#include <cstdint>

namespace MUHelper::Manual
{
// Kept after the player's walk ends. Two helper ticks plus change: long enough
// that a chain of short clicks (the usual way a player steers around a mob) is
// not chopped up by the bot between them, short enough that the hunt resumes
// as soon as the player stops driving.
inline constexpr std::uint32_t kGraceMs = 1500;

// Upper bound for a single claim. Hero->Movement is a plain flag that other
// code can leave set (a path dropped from under it, a walk the server never
// acknowledges); without a cap such a flag would disable the helper for the
// rest of the session. Every new click restarts this budget, so a player who
// keeps steering is never cut off.
inline constexpr std::uint32_t kMaxHoldMs = 20000;

struct State
{
    // Tick of the most recent manual order.
    std::uint32_t claimedAt = 0;
    // Tick at which the walk that order started was seen to end.
    std::uint32_t releasedAt = 0;
    // A manual order is still in charge (its walk has not finished).
    bool claimed = false;
    // Distinguishes "never claimed" from "claimed long ago", so a fresh state
    // is not treated as a claim that expired at tick 0.
    bool everClaimed = false;
};

// GetTickCount() wraps every ~49.7 days. Unsigned subtraction is the standard
// way to stay correct across the wrap; never compare the raw tick values.
inline std::uint32_t Elapsed(std::uint32_t now, std::uint32_t since)
{
    return static_cast<std::uint32_t>(now - since);
}

inline void Reset(State& state)
{
    state = State{};
}

// The player personally ordered something (a click on the world, a follow).
inline void Claim(State& state, std::uint32_t now)
{
    state.claimed = true;
    state.everClaimed = true;
    state.claimedAt = now;
    state.releasedAt = 0;
}

// One helper tick. `heroWalking` is Hero->Movement: the claim holds while the
// walk the player started is still running, and is released as soon as it ends
// (or when the hold budget runs out, whichever comes first).
inline void Update(State& state, bool heroWalking, std::uint32_t now)
{
    if (!state.claimed)
    {
        return;
    }

    if (heroWalking && Elapsed(now, state.claimedAt) < kMaxHoldMs)
    {
        return;
    }

    state.claimed = false;
    state.releasedAt = now;
}

// True while the helper must keep its hands off the hero.
inline bool IsSuppressed(const State& state, std::uint32_t now)
{
    if (!state.everClaimed)
    {
        return false;
    }

    if (state.claimed)
    {
        return Elapsed(now, state.claimedAt) < kMaxHoldMs;
    }

    return Elapsed(now, state.releasedAt) < kGraceMs;
}
}
