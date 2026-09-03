#include "stdafx.h"
#include "UI/Voice/VoiceSpeakingIndicator.h"

#include <cmath>

#include "Audio/VoiceChat.h"
#include "Camera/CameraProjection.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Models/ZzzBMD.h"
#include "UI/Voice/VoiceIcons.h"

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
    constexpr float kWorldIconScale = 1.f;

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
    // Must run before RenderName()/text output in CNewUINameWindow::Render:
    // the text pass leaves GL state behind that makes the RenderColor quads
    // land on the last drawn name label (or vanish), which is how the glyph
    // ended up over NPC names instead of the speaker's head.
    void RenderSpeakingIndicators()
    {
        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            CHARACTER* character = &CharactersClient[i];
            if (!character->Object.Live || !character->Object.Visible)
                continue;
            if (!VoiceChat::IsSpeakerActive(static_cast<unsigned short>(character->Key)))
                continue;

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
                UI::Voice::DrawMicrophoneIcon(originX, originY, kWorldIconScale, true);
            else
                UI::Voice::DrawSpeakerIcon(originX, originY, kWorldIconScale, true);
        }
    }
}
