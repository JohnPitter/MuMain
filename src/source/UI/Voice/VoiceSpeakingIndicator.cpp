#include "stdafx.h"
#include "UI/Voice/VoiceSpeakingIndicator.h"

#include <array>
#include <cmath>

#include "Audio/VoiceChat.h"
#include "Camera/CameraProjection.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"

namespace
{
    // Same anchor as PARTS_WEBZEN (GM "mu logo") in CSParts.cpp:
    // bone 20 with local offset (70, -5, 0). Using the character's own
    // BoneTransform + Position/Scale (BoneManager-style) so this 2D UI pass
    // does not depend on the shared Models[].BodyOrigin left by whoever
    // rendered last.
    constexpr int kGmMarkBone = 20;
    constexpr float kGmMarkOffsetX = 70.f;
    constexpr float kGmMarkOffsetY = -5.f;
    constexpr float kGmMarkOffsetZ = 0.f;
    // Fallback when BoneTransform is missing (rare) — old flat height offset.
    constexpr float kFallbackHeightOffset = 420.0f;
    constexpr float kPulseAmplitude = 1.5f;
    constexpr float kOutlineMargin = 0.75f;
    // DrawMicrophone/DrawSpeakerIcon grow downward from originY; shift so the
    // glyph's visual center sits on the projected GM-mark point.
    constexpr float kMicrophoneCenterY = 11.5f;
    constexpr float kSpeakerCenterY = 7.f;

    struct Rgb { float r, g, b; };
    constexpr Rgb kMetalHighlight { 0.96f, 0.97f, 0.99f };
    constexpr Rgb kMetalBaseLight { 0.83f, 0.85f, 0.89f };
    constexpr Rgb kMetalBase      { 0.68f, 0.70f, 0.75f };
    constexpr Rgb kMetalShadow    { 0.42f, 0.44f, 0.50f };
    constexpr Rgb kGoldAccent     { 0.95f, 0.74f, 0.24f };

    void DrawOutlinedQuad(float x, float y, float width, float height, Rgb color)
    {
        glColor4f(0.f, 0.f, 0.f, 1.f);
        RenderColor(x - kOutlineMargin, y - kOutlineMargin, width + (kOutlineMargin * 2.f), height + (kOutlineMargin * 2.f));
        glColor4f(color.r, color.g, color.b, 1.f);
        RenderColor(x, y, width, height);
    }

    void DrawMicrophone(float originX, float originY)
    {
        struct Row { float yOffset, height, halfWidth; Rgb color; };
        const std::array<Row, 5> kCapsuleRows = { {
            { 0.f,  3.f, 2.f, kMetalHighlight },
            { 3.f,  3.f, 4.f, kMetalBaseLight },
            { 6.f,  6.f, 5.f, kMetalBase },
            { 12.f, 3.f, 4.f, kMetalBaseLight },
            { 15.f, 3.f, 2.f, kMetalShadow },
        } };

        for (const auto& row : kCapsuleRows)
            DrawOutlinedQuad(originX - row.halfWidth, originY + row.yOffset, row.halfWidth * 2.f, row.height, row.color);

        DrawOutlinedQuad(originX - 5.2f, originY + 7.f, 10.4f, 1.4f, kGoldAccent);
        DrawOutlinedQuad(originX - 1.f, originY + 18.f, 2.f, 3.f, kMetalShadow);
        DrawOutlinedQuad(originX - 5.f, originY + 21.f, 10.f, 2.f, kMetalBase);

        glColor4f(0.15f, 0.15f, 0.18f, 1.f);
        RenderColor(originX - 7.f, originY + 8.f, 1.5f, 8.f);
        RenderColor(originX + 5.5f, originY + 8.f, 1.5f, 8.f);
        RenderColor(originX - 7.f, originY + 16.f, 13.f, 1.5f);
    }

    void DrawSpeakerIcon(float originX, float originY)
    {
        struct Slice { float xOffset, yOffset, width, height; Rgb color; };
        const std::array<Slice, 5> kConeSlices = { {
            { -9.f, 4.f,  3.f, 6.f,  kMetalBase },
            { -6.f, 4.f,  2.f, 6.f,  kMetalBaseLight },
            { -4.f, 2.5f, 2.f, 9.f,  kMetalBase },
            { -2.f, 1.f,  2.f, 12.f, kMetalBaseLight },
            {  0.f, 0.f,  2.f, 14.f, kMetalHighlight },
        } };

        for (const auto& slice : kConeSlices)
            DrawOutlinedQuad(originX + slice.xOffset, originY + slice.yOffset, slice.width, slice.height, slice.color);

        DrawOutlinedQuad(originX + 0.5f, originY + 6.f, 1.4f, 2.f, kGoldAccent);

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
            DrawOutlinedQuad(originX + arc.xOffset, originY + arc.yOffset, arc.width, arc.height, arc.color);
    }

    bool ResolveGmMarkWorldPosition(CHARACTER* character, vec3_t outPosition)
    {
        OBJECT* o = &character->Object;
        if (o->BoneTransform == nullptr)
            return false;

        BMD* b = &Models[o->Type];
        if (b == nullptr || b->NumBones <= kGmMarkBone)
            return false;

        vec3_t offset {};
        Vector(kGmMarkOffsetX, kGmMarkOffsetY, kGmMarkOffsetZ, offset);

        vec3_t localPos {};
        b->TransformPosition(o->BoneTransform[kGmMarkBone], offset, localPos, false);
        VectorScale(localPos, o->Scale, localPos);
        VectorAdd(localPos, o->Position, outPosition);
        return true;
    }
}

namespace UI::Voice
{
    void RenderSpeakingIndicators()
    {
        bool anySpeaking = false;
        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            CHARACTER* character = &CharactersClient[i];
            if (!character->Object.Live || !character->Object.Visible)
                continue;
            if (!VoiceChat::IsSpeakerActive(static_cast<unsigned short>(character->Key)))
                continue;

            anySpeaking = true;

            vec3_t position {};
            if (!ResolveGmMarkWorldPosition(character, position))
            {
                VectorCopy(character->Object.Position, position);
                position[2] += kFallbackHeightOffset;
            }

            int screenX = 0;
            int screenY = 0;
            CameraProjection::WorldToScreen(g_Camera, position, &screenX, &screenY);

            // WorldToScreen already returns reference-space coords. RenderColor
            // applies ConvertPosX/Y itself — do NOT convert here (that was
            // pushing the glyph off the GM-mark spot).
            const float pulse = std::sin(WorldTime * 0.01f) * kPulseAmplitude;
            const float originX = static_cast<float>(screenX);
            const float originY = static_cast<float>(screenY) + pulse;

            if (character == Hero)
                DrawMicrophone(originX, originY - kMicrophoneCenterY);
            else
                DrawSpeakerIcon(originX, originY - kSpeakerCenterY);
        }

        if (anySpeaking)
        {
            glColor4f(1.f, 1.f, 1.f, 1.f);
            EnableAlphaTest();
        }
    }
}
