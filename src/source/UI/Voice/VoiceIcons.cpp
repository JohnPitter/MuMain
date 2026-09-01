#include "stdafx.h"
#include "UI/Voice/VoiceIcons.h"

#include <array>

#include "Render/Textures/ZzzOpenglUtil.h"

namespace
{
    struct Rgb { float r, g, b; };

    Rgb ScaleRgb(Rgb color, float factor)
    {
        return { color.r * factor, color.g * factor, color.b * factor };
    }

    void FillRect(float x, float y, float width, float height, Rgb color)
    {
        glColor4f(color.r, color.g, color.b, 1.f);
        RenderColor(x, y, width, height);
    }

    void DrawOutlinedQuad(float x, float y, float width, float height, Rgb color, float scale)
    {
        const float outline = 0.75f * scale;
        FillRect(x - outline, y - outline, width + (outline * 2.f), height + (outline * 2.f), { 0.f, 0.f, 0.f });
        FillRect(x, y, width, height, color);
    }

    void DrawMicrophoneAt(float originX, float originY, float scale, bool enabled)
    {
        const float tone = enabled ? 1.f : 0.55f;
        const Rgb kMetalHighlight = ScaleRgb({ 0.96f, 0.97f, 0.99f }, tone);
        const Rgb kMetalBaseLight = ScaleRgb({ 0.83f, 0.85f, 0.89f }, tone);
        const Rgb kMetalBase = ScaleRgb({ 0.68f, 0.70f, 0.75f }, tone);
        const Rgb kMetalShadow = ScaleRgb({ 0.42f, 0.44f, 0.50f }, tone);
        const Rgb kGoldAccent = ScaleRgb({ 0.95f, 0.74f, 0.24f }, tone);

        struct Row { float yOffset, height, halfWidth; Rgb color; };
        const std::array<Row, 5> kCapsuleRows = { {
            { 0.f,  3.f, 2.f, kMetalHighlight },
            { 3.f,  3.f, 4.f, kMetalBaseLight },
            { 6.f,  6.f, 5.f, kMetalBase },
            { 12.f, 3.f, 4.f, kMetalBaseLight },
            { 15.f, 3.f, 2.f, kMetalShadow },
        } };

        for (const auto& row : kCapsuleRows)
        {
            DrawOutlinedQuad(originX - (row.halfWidth * scale), originY + (row.yOffset * scale),
                row.halfWidth * 2.f * scale, row.height * scale, row.color, scale);
        }

        DrawOutlinedQuad(originX - (5.2f * scale), originY + (7.f * scale), 10.4f * scale, 1.4f * scale, kGoldAccent, scale);
        DrawOutlinedQuad(originX - (1.f * scale), originY + (18.f * scale), 2.f * scale, 3.f * scale, kMetalShadow, scale);
        DrawOutlinedQuad(originX - (5.f * scale), originY + (21.f * scale), 10.f * scale, 2.f * scale, kMetalBase, scale);

        const Rgb grille = ScaleRgb({ 0.15f, 0.15f, 0.18f }, tone);
        FillRect(originX - (7.f * scale), originY + (8.f * scale), 1.5f * scale, 8.f * scale, grille);
        FillRect(originX + (5.5f * scale), originY + (8.f * scale), 1.5f * scale, 8.f * scale, grille);
        FillRect(originX - (7.f * scale), originY + (16.f * scale), 13.f * scale, 1.5f * scale, grille);
    }

    void DrawSpeakerAt(float originX, float originY, float scale, bool enabled)
    {
        const float tone = enabled ? 1.f : 0.55f;
        const Rgb kMetalHighlight = ScaleRgb({ 0.96f, 0.97f, 0.99f }, tone);
        const Rgb kMetalBaseLight = ScaleRgb({ 0.83f, 0.85f, 0.89f }, tone);
        const Rgb kMetalBase = ScaleRgb({ 0.68f, 0.70f, 0.75f }, tone);
        const Rgb kGoldAccent = ScaleRgb({ 0.95f, 0.74f, 0.24f }, tone);

        struct Slice { float xOffset, yOffset, width, height; Rgb color; };
        const std::array<Slice, 5> kConeSlices = { {
            { -9.f, 4.f,  3.f, 6.f,  kMetalBase },
            { -6.f, 4.f,  2.f, 6.f,  kMetalBaseLight },
            { -4.f, 2.5f, 2.f, 9.f,  kMetalBase },
            { -2.f, 1.f,  2.f, 12.f, kMetalBaseLight },
            {  0.f, 0.f,  2.f, 14.f, kMetalHighlight },
        } };

        for (const auto& slice : kConeSlices)
        {
            DrawOutlinedQuad(originX + (slice.xOffset * scale), originY + (slice.yOffset * scale),
                slice.width * scale, slice.height * scale, slice.color, scale);
        }

        DrawOutlinedQuad(originX + (0.5f * scale), originY + (6.f * scale), 1.4f * scale, 2.f * scale, kGoldAccent, scale);

        struct ArcPiece { float xOffset, yOffset, width, height; Rgb color; };
        const std::array<ArcPiece, 6> kSoundWaves = { {
            { 2.5f, 3.f,  1.5f, 3.f, kMetalHighlight },
            { 3.5f, 6.f,  1.5f, 2.f, kMetalHighlight },
            { 2.5f, 8.f,  1.5f, 3.f, kMetalHighlight },
            { 6.5f, 1.f,  1.5f, 4.f, kMetalBaseLight },
            { 7.5f, 6.f,  1.5f, 2.f, kMetalBaseLight },
            { 6.5f, 11.f, 1.5f, 4.f, kMetalBaseLight },
        } };

        for (const auto& arc : kSoundWaves)
        {
            DrawOutlinedQuad(originX + (arc.xOffset * scale), originY + (arc.yOffset * scale),
                arc.width * scale, arc.height * scale, arc.color, scale);
        }
    }

    void DrawMicrophoneGlyphAt(float originX, float originY, float scale, bool enabled)
    {
        const float ink = enabled ? 0.04f : 0.18f;
        const Rgb black{ ink, ink, ink };

        FillRect(originX - (4.f * scale), originY + (0.5f * scale), 8.f * scale, 14.f * scale, black);
        FillRect(originX - (3.f * scale), originY, 6.f * scale, 1.2f * scale, black);
        FillRect(originX - (3.f * scale), originY + (14.2f * scale), 6.f * scale, 1.2f * scale, black);
        FillRect(originX - (5.5f * scale), originY + (6.f * scale), 1.6f * scale, 8.f * scale, black);
        FillRect(originX + (3.9f * scale), originY + (6.f * scale), 1.6f * scale, 8.f * scale, black);
        FillRect(originX - (5.5f * scale), originY + (13.4f * scale), 11.f * scale, 1.6f * scale, black);
        FillRect(originX - (1.f * scale), originY + (15.f * scale), 2.f * scale, 4.f * scale, black);
        FillRect(originX - (4.5f * scale), originY + (18.6f * scale), 9.f * scale, 2.f * scale, black);
    }

    void DrawSpeakerGlyphAt(float originX, float originY, float scale, bool enabled)
    {
        const float ink = enabled ? 0.04f : 0.18f;
        const Rgb black{ ink, ink, ink };

        FillRect(originX - (7.5f * scale), originY + (4.5f * scale), 3.2f * scale, 7.f * scale, black);
        FillRect(originX - (4.5f * scale), originY + (3.f * scale), 2.2f * scale, 10.f * scale, black);
        FillRect(originX - (2.4f * scale), originY + (1.4f * scale), 2.4f * scale, 13.2f * scale, black);
        FillRect(originX, originY, 2.2f * scale, 16.f * scale, black);
        FillRect(originX + (3.2f * scale), originY + (3.2f * scale), 1.6f * scale, 3.2f * scale, black);
        FillRect(originX + (4.2f * scale), originY + (6.6f * scale), 1.6f * scale, 2.6f * scale, black);
        FillRect(originX + (3.2f * scale), originY + (9.4f * scale), 1.6f * scale, 3.2f * scale, black);
        FillRect(originX + (6.4f * scale), originY + (1.6f * scale), 1.6f * scale, 4.2f * scale, black);
        FillRect(originX + (7.4f * scale), originY + (6.4f * scale), 1.6f * scale, 3.f * scale, black);
        FillRect(originX + (6.4f * scale), originY + (10.2f * scale), 1.6f * scale, 4.2f * scale, black);
    }

    constexpr float kMicrophoneCenterY = 11.5f;
    constexpr float kSpeakerCenterY = 7.f;
    constexpr float kGlyphMicCenterY = 10.f;
    constexpr float kGlyphSpeakerCenterY = 8.f;
}

namespace UI::Voice
{
    void DrawMicrophoneIcon(float centerX, float centerY, float scale, bool enabled)
    {
        EnableAlphaTest();
        DrawMicrophoneAt(centerX, centerY - (kMicrophoneCenterY * scale), scale, enabled);
        glColor4f(1.f, 1.f, 1.f, 1.f);
    }

    void DrawSpeakerIcon(float centerX, float centerY, float scale, bool enabled)
    {
        EnableAlphaTest();
        DrawSpeakerAt(centerX - (4.f * scale), centerY - (kSpeakerCenterY * scale), scale, enabled);
        glColor4f(1.f, 1.f, 1.f, 1.f);
    }

    void DrawMicrophoneGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        EnableAlphaTest();
        DrawMicrophoneGlyphAt(centerX, centerY - (kGlyphMicCenterY * scale), scale, enabled);
        glColor4f(1.f, 1.f, 1.f, 1.f);
    }

    void DrawSpeakerGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        EnableAlphaTest();
        DrawSpeakerGlyphAt(centerX - (1.5f * scale), centerY - (kGlyphSpeakerCenterY * scale), scale, enabled);
        glColor4f(1.f, 1.f, 1.f, 1.f);
    }
}
