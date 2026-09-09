// Deliberately no stdafx.h: this TU is compiled both into Main and into the
// doctest tree (tests/voice), whose include path only covers src/source.
#include "Audio/VoiceDsp.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace VoiceDsp
{
    namespace
    {
        constexpr float kCaptureGain = 2.5f;
        // Target peak after the gain stage. 32767 / kPlaybackGain (2.5, the
        // receiver-side stream gain), so any frame at or below the ceiling is
        // guaranteed to stay inside int16 after the playback boost.
        constexpr float kCaptureLimiterPeak = 13106.0f;
    }

    void ApplyCaptureGain(std::int16_t* samples, int count)
    {
        if (!samples || count <= 0)
            return;

        int peak = 0;
        for (int i = 0; i < count; ++i)
            peak = std::max(peak, std::abs(static_cast<int>(samples[i])));

        // Three regimes, per frame:
        // - quiet (peak < ceiling/2.5): the full fixed 2.5x boost - the "voice
        //   is way too quiet" case;
        // - mid: normalized up so the peak lands exactly on the ceiling
        //   (consistent loudness, still clip-free after playback gain);
        // - hot (peak > ceiling): untouched - the speaker is already loud,
        //   and attenuating them below their raw level would be worse than
        //   the receiver-side peak clipping of an over-loud signal.
        // The gain never attenuates (minimum 1.0) and never over-boosts.
        const float frameGain = std::clamp(kCaptureLimiterPeak / static_cast<float>(peak),
            1.0f, kCaptureGain);

        for (int i = 0; i < count; ++i)
        {
            const float value = static_cast<float>(samples[i]) * frameGain;
            samples[i] = static_cast<std::int16_t>(std::clamp(std::lroundf(value),
                static_cast<long>(INT16_MIN), static_cast<long>(INT16_MAX)));
        }
    }

    bool HasVoiceActivity(const std::int16_t* samples, int count, int threshold)
    {
        if (!samples || count <= 0)
            return false;

        std::int64_t total = 0;
        for (int i = 0; i < count; ++i)
            total += std::abs(static_cast<int>(samples[i]));
        return total / count >= threshold;
    }
}
