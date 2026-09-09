// Unit tests for the Options-window slider input gate and the volume scale
// mappings.  Regression coverage for the volume-decay bug (2026-09): the
// slider handlers used to key their drag on KEY_REPEAT alone, so ANY held
// left button (MU walk/attack hold) whose cursor crossed an open Options
// window silently rewrote the volume and saved it to config.ini — players
// saw their configured volume "come back at a fraction" after exit/re-enter,
// and live installs decayed all the way to SoundVolume=0 (mute).

#include "doctest.h"

#include "UI/NewUI/Options/VolumeSliderInput.h"
#include "Audio/VolumeCurve.h"

TEST_CASE("a press that started elsewhere must not move the slider (the decay bug)")
{
    // Walk/attack hold: button went down somewhere else (isRepeat=true, no
    // press on the track this frame) and the cursor is merely crossing the
    // track.  This used to rewrite the level; it must do nothing now.
    CHECK(VolumeSliderInput::ShouldTrackDrag(true, false, true, false) == false);
}

TEST_CASE("a fresh press on the track starts a drag")
{
    CHECK(VolumeSliderInput::ShouldTrackDrag(true, true, false, false) == true);
    // Press frame transitions to repeat on the next frame: drag continues.
    CHECK(VolumeSliderInput::ShouldTrackDrag(true, false, true, true) == true);
}

TEST_CASE("releasing the button or leaving the track ends the drag")
{
    CHECK(VolumeSliderInput::ShouldTrackDrag(true, false, false, true) == false);
    CHECK(VolumeSliderInput::ShouldTrackDrag(false, false, true, true) == false);
    CHECK(VolumeSliderInput::ShouldTrackDrag(false, true, false, false) == false);
}

TEST_CASE("cursor offset maps to the 0..10 slider level with round-to-nearest")
{
    constexpr int kWidth = 124;  // Options-window track width in reference px
    constexpr int kMax = 10;

    CHECK(VolumeSliderInput::MapOffsetToLevel(-1, kWidth, kMax) == 0);
    CHECK(VolumeSliderInput::MapOffsetToLevel(0, kWidth, kMax) == 0);
    // Left edge region: the whole first 1/20th of the track is level 0.
    CHECK(VolumeSliderInput::MapOffsetToLevel(6, kWidth, kMax) == 0);
    CHECK(VolumeSliderInput::MapOffsetToLevel(kWidth / 2, kWidth, kMax) == 5);
    // 80% of the track is level 8 (the owner's "configure 80%" case).
    CHECK(VolumeSliderInput::MapOffsetToLevel((kWidth * 8) / 10, kWidth, kMax) == 8);
    CHECK(VolumeSliderInput::MapOffsetToLevel(kWidth, kWidth, kMax) == 10);
    // Beyond the right edge saturates via the caller's clamp; the pure map
    // rounds (124*10/124+0.5) — callers clamp to the same 10.
    CHECK(VolumeSliderInput::MapOffsetToLevel(kWidth + 30, kWidth, kMax) == 12);
}

TEST_CASE("effect level maps to DirectSound centibels (log curve, 0 = mute)")
{
    CHECK(VolumeCurve::EffectLevelToDsVolume(0) == -10000);
    // Legacy curve: -2000*log10(10/level), truncated toward zero.
    CHECK(VolumeCurve::EffectLevelToDsVolume(1) == -2000);
    CHECK(VolumeCurve::EffectLevelToDsVolume(5) == -602);   // -2000*log10(2)
    CHECK(VolumeCurve::EffectLevelToDsVolume(9) == -91);    // -2000*log10(10/9)
    // Slider 100% now plays at full output (the old 0..9 clamp cost 0.9 dB).
    CHECK(VolumeCurve::EffectLevelToDsVolume(10) == 0);
    // Out-of-range slider values clamp into the curve's domain.
    CHECK(VolumeCurve::EffectLevelToDsVolume(-3) == -10000);
    CHECK(VolumeCurve::EffectLevelToDsVolume(11) == 0);
    CHECK(VolumeCurve::EffectLevelToDsVolume(999) == 0);
}
