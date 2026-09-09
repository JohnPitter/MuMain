#include "doctest.h"

#include "Audio/VoiceDsp.h"

#include <cstdint>
#include <vector>

namespace
{
    TEST_CASE("gain: silence stays silent")
    {
        std::vector<std::int16_t> samples(160, 0);
        VoiceDsp::ApplyCaptureGain(samples.data(), static_cast<int>(samples.size()));
        for (const std::int16_t sample : samples)
            CHECK(sample == 0);
    }

    TEST_CASE("gain: guards on null or empty input")
    {
        std::int16_t dummy = 100;
        VoiceDsp::ApplyCaptureGain(nullptr, 10);
        VoiceDsp::ApplyCaptureGain(&dummy, 0);
        VoiceDsp::ApplyCaptureGain(&dummy, -5);
        CHECK(dummy == 100);
    }

    TEST_CASE("gain: quiet signal gets the fixed 2.5x boost")
    {
        // Mean abs 80/32768 - a typical quiet microphone well below the old
        // VAD threshold of 300; this is the "voice is way too quiet" input.
        std::vector<std::int16_t> samples(160, 80);
        VoiceDsp::ApplyCaptureGain(samples.data(), static_cast<int>(samples.size()));
        for (const std::int16_t sample : samples)
            CHECK(sample == 200);
    }

    TEST_CASE("gain: mid frame is normalized up to the limiter ceiling")
    {
        // Peak 8000 sits between ceiling/2.5 (5242) and the ceiling (13106):
        // the frame is scaled so its peak lands exactly on the ceiling
        // (gain 13106/8000 = 1.63825), consistent loudness, no clipping.
        std::vector<std::int16_t> samples { 8000, -4000, 0, 2000 };
        VoiceDsp::ApplyCaptureGain(samples.data(), static_cast<int>(samples.size()));
        CHECK(samples[0] == 13106);
        CHECK(samples[1] == -6553);
        CHECK(samples[2] == 0);
        CHECK(samples[3] == 3277); // 2000 x 1.63825 = 3276.5 -> lroundf = 3277 (banker-free rounding)
    }

    TEST_CASE("gain: hot frame passes through untouched (never attenuated)")
    {
        // Peak 26212 already exceeds the ceiling: normalizing down would make
        // an already-loud speaker quieter than their raw mic, so the frame is
        // left alone (the receiver clips only these over-loud peaks).
        std::vector<std::int16_t> samples { 0, 13106, -26212, 1000, -500 };
        VoiceDsp::ApplyCaptureGain(samples.data(), static_cast<int>(samples.size()));
        CHECK(samples[0] == 0);
        CHECK(samples[1] == 13106);
        CHECK(samples[2] == -26212);
        CHECK(samples[3] == 1000);
        CHECK(samples[4] == -500);
    }

    TEST_CASE("gain: full-scale extremes survive without overflow")
    {
        std::vector<std::int16_t> samples(160, 32767);
        VoiceDsp::ApplyCaptureGain(samples.data(), static_cast<int>(samples.size()));
        CHECK(samples[0] == 32767);

        std::vector<std::int16_t> negative(160, -32768);
        VoiceDsp::ApplyCaptureGain(negative.data(), static_cast<int>(negative.size()));
        CHECK(negative[0] == -32768);
    }

    TEST_CASE("vad: below threshold is silence, at or above is voice")
    {
        std::vector<std::int16_t> quiet(160, 100);
        std::vector<std::int16_t> loud(160, 300);
        CHECK_FALSE(VoiceDsp::HasVoiceActivity(quiet.data(), static_cast<int>(quiet.size()), 300));
        CHECK(VoiceDsp::HasVoiceActivity(loud.data(), static_cast<int>(loud.size()), 300));
    }

    TEST_CASE("vad: guards on null or empty input")
    {
        std::int16_t dummy = 30000;
        CHECK_FALSE(VoiceDsp::HasVoiceActivity(nullptr, 10, 300));
        CHECK_FALSE(VoiceDsp::HasVoiceActivity(&dummy, 0, 300));
        CHECK_FALSE(VoiceDsp::HasVoiceActivity(&dummy, -1, 300));
    }

    TEST_CASE("vad: mixed frame uses the mean, so a lone spike does not gate open")
    {
        // Mean abs = (32000 * 2 + 0 * 158) / 160 = 400 >= 300.
        std::vector<std::int16_t> mostlyQuiet(160, 0);
        mostlyQuiet[0] = 32000;
        mostlyQuiet[1] = -32000;
        CHECK(VoiceDsp::HasVoiceActivity(mostlyQuiet.data(), static_cast<int>(mostlyQuiet.size()), 300));
    }
}
