#pragma once

namespace UI::Voice
{
    // Draws a compact microphone glyph centered on (centerX, centerY).
    // scale=1.0 matches the world-space speaking indicator size.
    void DrawMicrophoneIcon(float centerX, float centerY, float scale, bool enabled);

    // Draws a compact speaker glyph centered on (centerX, centerY).
    void DrawSpeakerIcon(float centerX, float centerY, float scale, bool enabled);
}
