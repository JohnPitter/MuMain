#pragma once

#include <cstdint>

namespace VoiceDsp
{
    // Pure (SDL-free) DSP helpers for the proximity voice chat, split out of
    // VoiceChat.cpp so the numeric behavior can be unit-tested in doctest
    // without dragging in the audio device or network layers.

    // Fixed software capture gain applied BEFORE the voice-activity gate and
    // the ADPCM encoder. Windows delivers most consumer microphones far below
    // full scale (the player complaint was "voice is way too quiet"), and the
    // old pipeline encoded the raw capture level, so listeners heard exactly
    // that anemic signal. Per frame:
    // - quiet (peak below ceiling/2.5): full 2.5x (+8 dB) boost;
    // - mid: normalized so the peak lands exactly on the ceiling (consistent
    //   loudness, still clip-free after the receiver's 2.5x playback gain);
    // - hot (peak above the ceiling): untouched — the speaker is already loud
    //   and the gain never attenuates.
    // 2.5x at capture x the 2.5x playback stream gain = 6.25x (+16 dB) end to
    // end for quiet microphones.
    void ApplyCaptureGain(std::int16_t* samples, int count);

    // True when the frame's mean absolute amplitude reaches the threshold.
    // Runs on the GAINED samples so quiet microphones are not gated off (the
    // old fixed threshold on raw capture chopped low-level speech into pieces).
    bool HasVoiceActivity(const std::int16_t* samples, int count, int threshold);
}
