#include "stdafx.h"
#include "UI/Voice/VoiceIcons.h"

#include <iterator>

#include "Render/Textures/ZzzOpenglUtil.h"

// Voice icons are drawn from filled quads (RenderColor) in MU's steel-and-gold
// interface palette: a studio microphone in a yoke stand, and a speaker cone
// with sound waves. A whole-glyph "sticker" outline (the shape re-drawn dark at
// eight neighbor offsets) keeps them legible on any background. Coordinates are
// in icon units, origin at the icon center, +y downward (screen space).
namespace
{
    struct Rgb { float r, g, b; };
    struct Quad { float x, y, w, h; Rgb color; };

    constexpr Rgb kInk{ 0.07f, 0.08f, 0.10f };
    constexpr Rgb kSteelD{ 0.36f, 0.38f, 0.42f };
    constexpr Rgb kSteelM{ 0.55f, 0.57f, 0.61f };
    constexpr Rgb kSteelL{ 0.76f, 0.78f, 0.82f };
    constexpr Rgb kSteelH{ 0.93f, 0.95f, 0.97f };
    constexpr Rgb kGold{ 0.91f, 0.71f, 0.31f };
    constexpr Rgb kGoldL{ 0.96f, 0.86f, 0.58f };

    constexpr float kOutlineWidth = 1.f;   // icon units
    constexpr float kDisabledTone = 0.5f;
    constexpr float kWorldLift = 12.f;      // float the world icon above the anchor

    // Studio microphone: rounded capsule head, cylinder shading, gold cap ring,
    // grille slits, and a yoke stand.
    constexpr Quad kMicrophone[] = {
        // head capsule (rounded rows)
        { -3.f, -12.f, 6.f, 1.f, kSteelM }, { -4.f, -11.f, 8.f, 1.f, kSteelM },
        { -5.f, -10.f, 10.f, 1.f, kSteelM }, { -6.f, -9.f, 12.f, 1.f, kSteelM },
        { -6.f, -8.f, 12.f, 1.f, kSteelM }, { -6.f, -7.f, 12.f, 1.f, kSteelM },
        { -6.f, -6.f, 12.f, 1.f, kSteelM }, { -6.f, -5.f, 12.f, 1.f, kSteelM },
        { -6.f, -4.f, 12.f, 1.f, kSteelM }, { -6.f, -3.f, 12.f, 1.f, kSteelM },
        { -6.f, -2.f, 12.f, 1.f, kSteelM }, { -6.f, -1.f, 12.f, 1.f, kSteelM },
        { -5.f, 0.f, 10.f, 1.f, kSteelM }, { -4.f, 1.f, 8.f, 1.f, kSteelM },
        { -3.f, 2.f, 6.f, 1.f, kSteelM },
        // cylinder shading
        { -4.5f, -9.f, 2.4f, 10.f, kSteelH }, { 2.2f, -9.f, 2.6f, 10.f, kSteelD },
        { -1.6f, -9.f, 1.4f, 10.f, kSteelL },
        // gold cap ring
        { -5.f, -10.5f, 10.f, 1.6f, kGold }, { -5.f, -10.5f, 10.f, 0.6f, kGoldL },
        // grille slits
        { -4.f, -7.f, 8.f, 0.9f, kInk }, { -4.f, -4.4f, 8.f, 0.9f, kInk },
        { -4.f, -1.8f, 8.f, 0.9f, kInk },
        // yoke arms + bottom
        { -7.f, -2.f, 1.6f, 8.f, kSteelD }, { 5.4f, -2.f, 1.6f, 8.f, kSteelD },
        { -7.f, 6.f, 12.4f, 1.6f, kSteelM },
        // stem + base
        { -1.f, 7.6f, 2.f, 3.f, kSteelM }, { -4.f, 10.4f, 8.f, 1.8f, kSteelD },
        { -4.f, 10.4f, 8.f, 0.6f, kSteelM },
    };

    // Speaker cone. The first kSpeakerConeCount quads are the cone; the trailing
    // quads are the gold sound waves (drawn only when audio is active).
    constexpr Quad kSpeaker[] = {
        { -7.f, -2.6f, 3.f, 5.2f, kSteelD },   // magnet box
        { -4.f, -3.f, 2.f, 6.f, kSteelM }, { -2.f, -4.f, 2.f, 8.f, kSteelM },
        { 0.f, -5.f, 2.f, 10.f, kSteelL }, { 2.f, -6.f, 1.4f, 12.f, kSteelH },
        { -3.4f, -1.4f, 5.f, 2.8f, kSteelH },  // cone sheen
        // sound waves (gold)
        { 4.2f, -2.f, 1.4f, 4.f, kGold }, { 3.6f, -1.f, 0.9f, 2.f, kGoldL },
        { 6.2f, -4.f, 1.4f, 8.f, kGold }, { 5.6f, -2.6f, 0.9f, 5.2f, kGoldL },
    };
    constexpr int kSpeakerConeCount = 6;

    void Fill(float x, float y, float w, float h, Rgb color, float tone)
    {
        glColor4f(color.r * tone, color.g * tone, color.b * tone, 1.f);
        RenderColor(x, y, w, h);
    }

    // Sticker outline: every quad re-drawn dark at eight neighbor offsets, then
    // covered by the colored fills — leaves a clean 1px exterior edge only.
    void DrawOutline(const Quad* quads, int count, float ox, float oy, float scale)
    {
        const float o = kOutlineWidth * scale;
        const float offsets[8][2] = {
            { -o, 0.f }, { o, 0.f }, { 0.f, -o }, { 0.f, o },
            { -o, -o }, { o, -o }, { -o, o }, { o, o },
        };

        for (const auto& off : offsets)
        {
            for (int i = 0; i < count; ++i)
            {
                const Quad& q = quads[i];
                Fill(ox + (q.x * scale) + off[0], oy + (q.y * scale) + off[1],
                    q.w * scale, q.h * scale, kInk, 1.f);
            }
        }
    }

    void DrawTable(const Quad* quads, int count, float ox, float oy, float scale, float tone)
    {
        DrawOutline(quads, count, ox, oy, scale);
        for (int i = 0; i < count; ++i)
        {
            const Quad& q = quads[i];
            Fill(ox + (q.x * scale), oy + (q.y * scale), q.w * scale, q.h * scale, q.color, tone);
        }
    }

    void DrawMicrophone(float centerX, float centerY, float scale, bool enabled)
    {
        EnableAlphaTest();
        DrawTable(kMicrophone, static_cast<int>(std::size(kMicrophone)),
            centerX, centerY, scale, enabled ? 1.f : kDisabledTone);
        EndRenderColor();
    }

    void DrawSpeaker(float centerX, float centerY, float scale, bool enabled)
    {
        // Muted listening drops the sound waves; only the cone remains.
        const int count = enabled ? static_cast<int>(std::size(kSpeaker)) : kSpeakerConeCount;
        EnableAlphaTest();
        DrawTable(kSpeaker, count, centerX, centerY, scale, enabled ? 1.f : kDisabledTone);
        EndRenderColor();
    }
}

namespace UI::Voice
{
    void DrawMicrophoneIcon(float centerX, float centerY, float scale, bool enabled)
    {
        DrawMicrophone(centerX, centerY - (kWorldLift * scale), scale, enabled);
    }

    void DrawSpeakerIcon(float centerX, float centerY, float scale, bool enabled)
    {
        DrawSpeaker(centerX, centerY - (kWorldLift * scale), scale, enabled);
    }

    void DrawMicrophoneGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawMicrophone(centerX, centerY, scale, enabled);
    }

    void DrawSpeakerGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawSpeaker(centerX, centerY, scale, enabled);
    }
}
