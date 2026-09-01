#pragma once

namespace UI::Voice
{
    // World-space speaking indicator (metallic glyph).
    void DrawMicrophoneIcon(float centerX, float centerY, float scale, bool enabled);

    void DrawSpeakerIcon(float centerX, float centerY, float scale, bool enabled);

    // HUD: black silhouette only. Caller fills the gray button first.
    void DrawMicrophoneGlyph(float centerX, float centerY, float scale, bool enabled);

    void DrawSpeakerGlyph(float centerX, float centerY, float scale, bool enabled);
}
