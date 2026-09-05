// Unit tests for the pure Options-window level clamps in
// GameConfigConstants.h. Load() runs them over every config.ini value before
// it reaches the options window, the audio mixer, or the effect renderer, so
// a truncated or hand-edited ini can never push an out-of-range level in.

#include "doctest.h"

#include "Data/GameConfig/GameConfigConstants.h"

TEST_CASE("volume clamps to the 0..10 slider range")
{
    CHECK(CfgLimits::ClampVolumeLevel(0) == 0);
    CHECK(CfgLimits::ClampVolumeLevel(5) == 5);
    CHECK(CfgLimits::ClampVolumeLevel(10) == 10);
    CHECK(CfgLimits::ClampVolumeLevel(-1) == 0);
    CHECK(CfgLimits::ClampVolumeLevel(-100) == 0);
    CHECK(CfgLimits::ClampVolumeLevel(11) == 10);
    CHECK(CfgLimits::ClampVolumeLevel(999) == 10);
}

TEST_CASE("effect-limitation clamps to the 0..5 slider range")
{
    CHECK(CfgLimits::ClampRenderLevel(0) == 0);
    CHECK(CfgLimits::ClampRenderLevel(4) == 4);
    CHECK(CfgLimits::ClampRenderLevel(5) == 5);
    CHECK(CfgLimits::ClampRenderLevel(-1) == 0);
    CHECK(CfgLimits::ClampRenderLevel(-100) == 0);
    CHECK(CfgLimits::ClampRenderLevel(6) == 5);
    CHECK(CfgLimits::ClampRenderLevel(999) == 5);
}

TEST_CASE("options-window defaults match the pre-persistence hardcoded values")
{
    // The constructor of CNewUIOptionWindow used these literals before local
    // persistence existed; the config defaults must stay identical so a fresh
    // install behaves as before.
    CHECK(CfgDefaults::CfgDefaultAutoAttack == true);
    CHECK(CfgDefaults::CfgDefaultWhisperSound == false);
    CHECK(CfgDefaults::CfgDefaultSlideHelp == true);
    CHECK(CfgDefaults::CfgDefaultRenderLevel == 4);
    CHECK(CfgDefaults::CfgDefaultRenderAllEffects == true);
}
