#pragma once

#include "Engine/Object/PlayerActionState.h"

class BMD;
class CHARACTER;
class OBJECT;

// Shared gates and emitters for the authored-set visual effects (Poseidon /
// Zeus). The client already knows the phased set bonus from the server
// (F3 E8 equipment state): HasCelestialAura() marks an active authored-set
// phase and ActivePercent() its cumulative tier. These helpers turn that state
// plus the player's motion into a single, testable decision per effect, and
// RenderSetEffects() emits the family's own look for that decision.
namespace Render::Items::SetFX
{
    // Visual policy: below this quality the authored set FX never show
    // (same threshold as the Celestial set aura and shimmer passes).
    constexpr int MinimumRenderLevel = 2;
    // Containment: no set FX beyond this distance from the camera.
    constexpr float EffectMaximumDistance = 1200.f;

    // Emission cadence, in reference frames, per motion state. Rest breathes
    // slowly, travel leaves a denser trail, a strike punctuates every swing.
    constexpr int RestInterval = 8;
    constexpr int TravelInterval = 4;
    constexpr int StrikeInterval = 2;

    // Ground-plane offsets, mirroring the Celestial aura geometry.
    constexpr float AuraHeight = 75.f;
    constexpr float AuraScale = 1.6f;
    constexpr float WakeHeight = 8.f;
    constexpr float WakeScale = 1.15f;
    // The water ward ("Proteção das Águas") only opens on the full phase.
    constexpr float WardPhaseThreshold = 0.99f;
    constexpr float WardScale = 2.4f;

    enum class Motion
    {
        Rest,
        Travel,
        Strike,
    };

    // Which authored family the character is wearing; decided by the armor
    // slot, which every phase of both sets requires.
    enum class Family
    {
        None,
        Poseidon,
        Zeus,
    };

    // Cumulative set phase in 0..1, or 0 when no authored-set bonus is active.
    float ActivePhase(const CHARACTER* character);

    Family FamilyOf(const CHARACTER* character);

    // Quality/distance/alpha/cloak gate shared by every set FX entry point.
    bool EffectsAllowed(OBJECT* object);

    // Motion classification is pure policy and lives in the header so the
    // suites can exercise the exact predicates without the render runtime
    // (same arrangement as the Zeus finish profile).
    inline bool IsTravelAction(int playerAction)
    {
        // Walks, runs, flight and mounts occupy the contiguous block between
        // the walk and attack ranges; the Dark Lord strides ride alongside.
        return (playerAction >= PLAYER_WALK_MALE && playerAction <= PLAYER_RUN_RIDE_WEAPON)
            || playerAction == PLAYER_DARKLORD_WALK
            || playerAction == PLAYER_RUN_RIDE_HORSE;
    }

    inline bool IsStrikeAction(int playerAction)
    {
        // PLAYER_DARKLORD_WALK and PLAYER_RUN_RIDE_HORSE were inserted INSIDE
        // the native attack block (PLAYER_ATTACK_FIST..PLAYER_RIDE_SKILL), so
        // IsAttackAction answers true for them. Travel therefore wins first,
        // or the Lord Emperor — the Poseidon set's own class — would flash the
        // weapon strike every step instead of dragging the ocean wake.
        // PLAYER_ATTACK_DARKHORSE sits outside the travel set and stays a swing.
        return !IsTravelAction(playerAction)
            && (Engine::Object::IsAttackAction(playerAction)
                || playerAction == PLAYER_ATTACK_DARKHORSE);
    }

    inline bool IsRestAction(int playerAction)
    {
        return !IsStrikeAction(playerAction) && !IsTravelAction(playerAction);
    }

    inline Motion ClassifyMotion(int playerAction)
    {
        if (IsStrikeAction(playerAction))
            return Motion::Strike;
        if (IsTravelAction(playerAction))
            return Motion::Travel;
        return Motion::Rest;
    }

    // Single per-frame entry point, called from the character render path
    // beside the Celestial aura.
    void RenderSetEffects(const CHARACTER* character, OBJECT* object, BMD* model);
}
