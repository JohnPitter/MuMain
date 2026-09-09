#pragma once

#include <cmath>

// Pure mapping between the Options-window volume scale (0..10, 0 = off) and
// the audio backends' native units.  Header-only so both the client and the
// doctest suite share one definition of the curve.
namespace VolumeCurve
{
    // The UI slider (and config.ini) store 0..10.
    constexpr int kMinLevel = 0;
    constexpr int kMaxLevel = 10;

    // DirectSound volume is in centibels (hundredths of a decibel):
    // 0 = full output, -10000 = silence.  The effect curve is logarithmic,
    // matching the legacy -2000*log10(10/level) attenuation; level 10 now
    // maps to exactly 0 dB (the old 0..9 clamp made the slider's 100% play
    // 0.9 dB below full, incoherent with the music slider's 10 -> 1.0 gain).
    inline long EffectLevelToDsVolume(int level)
    {
        if (level < kMinLevel)
            level = kMinLevel;
        if (level > kMaxLevel)
            level = kMaxLevel;

        if (level == kMinLevel)
            return -10000;

        return static_cast<long>(-2000.0f * std::log10(10.0f / static_cast<float>(level)));
    }
}
