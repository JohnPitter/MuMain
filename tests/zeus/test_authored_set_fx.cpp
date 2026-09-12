#include "doctest.h"

#include <windows.h>
#include <map>
#include "Core/Globals/_define.h"
#include "Render/Models/AuthorSetFX.h"

// Proves the motion policy that drives the authored-set effects of both
// families (Poseidon "Aura do Imperio / Ondas Oceanicas / Tridente" and Zeus
// "Aura Celestial / Trilha Celestial / Lamina de Raios"). The classification
// is pure header policy, so the suite exercises the exact predicates the
// render path uses without pulling in the render runtime.

using Render::Items::SetFX::ClassifyMotion;
using Render::Items::SetFX::IsRestAction;
using Render::Items::SetFX::IsStrikeAction;
using Render::Items::SetFX::IsTravelAction;
using Render::Items::SetFX::Motion;

TEST_CASE("Every swing animation classifies as a strike")
{
    for (int action = PLAYER_ATTACK_FIST; action <= PLAYER_RIDE_SKILL; ++action)
    {
        if (IsTravelAction(action))
        {
            // The Dark Lord stride and mounted run sit inside the native
            // attack block; travel owns them (see IsStrikeAction).
            continue;
        }

        CHECK(IsStrikeAction(action));
        CHECK(ClassifyMotion(action) == Motion::Strike);
        CHECK_FALSE(IsRestAction(action));
    }

    CHECK(IsStrikeAction(PLAYER_ATTACK_DARKHORSE));
    CHECK(ClassifyMotion(PLAYER_ATTACK_DARKHORSE) == Motion::Strike);
}

TEST_CASE("The Lord Emperor stride and mounted run drag the wake, never the swing")
{
    // Regression: both slots live inside PLAYER_ATTACK_FIST..PLAYER_RIDE_SKILL,
    // so a naive IsAttackAction fallback flashed the weapon on every step of
    // the class that actually wears the Poseidon set.
    for (const int action : { PLAYER_DARKLORD_WALK, PLAYER_RUN_RIDE_HORSE })
    {
        CHECK(IsTravelAction(action));
        CHECK_FALSE(IsStrikeAction(action));
        CHECK(ClassifyMotion(action) == Motion::Travel);
    }
}

TEST_CASE("Walks, runs and rides classify as travel")
{
    for (int action = PLAYER_WALK_MALE; action <= PLAYER_RUN_RIDE_WEAPON; ++action)
    {
        CHECK(IsTravelAction(action));
        CHECK(ClassifyMotion(action) == Motion::Travel);
        CHECK_FALSE(IsRestAction(action));
    }

    CHECK(IsTravelAction(PLAYER_DARKLORD_WALK));
    CHECK(IsTravelAction(PLAYER_RUN_RIDE_HORSE));
}

TEST_CASE("Standing and posing fall back to the resting aura")
{
    CHECK(IsRestAction(PLAYER_STOP_MALE));
    CHECK(ClassifyMotion(PLAYER_STOP_MALE) == Motion::Rest);
    CHECK(IsRestAction(PLAYER_SIT1));
    CHECK(ClassifyMotion(PLAYER_SIT1) == Motion::Rest);
    CHECK(IsRestAction(PLAYER_POSE1));
}

TEST_CASE("The three motion buckets never overlap")
{
    for (int action = 0; action < MAX_PLAYER_ACTION; ++action)
    {
        const int buckets = (IsStrikeAction(action) ? 1 : 0)
            + (IsTravelAction(action) ? 1 : 0)
            + (IsRestAction(action) ? 1 : 0);
        CHECK(buckets == 1);
    }
}

TEST_CASE("Effect policy keeps the Celestial quality and containment budget")
{
    // The authored sets never show below the quality the Celestial aura
    // already requires, and never leak past the shared containment radius.
    CHECK(Render::Items::SetFX::MinimumRenderLevel == 2);
    CHECK(Render::Items::SetFX::EffectMaximumDistance > 0.f);

    // Rest breathes slowest, a strike punctuates every swing.
    CHECK(Render::Items::SetFX::RestInterval > Render::Items::SetFX::TravelInterval);
    CHECK(Render::Items::SetFX::TravelInterval > Render::Items::SetFX::StrikeInterval);
    CHECK(Render::Items::SetFX::StrikeInterval >= 1);

    // The water ward only opens on the completed set.
    CHECK(Render::Items::SetFX::WardPhaseThreshold > 0.9f);
    CHECK(Render::Items::SetFX::WardPhaseThreshold <= 1.0f);
    CHECK(Render::Items::SetFX::WardScale > Render::Items::SetFX::AuraScale);
}
