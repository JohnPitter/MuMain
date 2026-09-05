#include "stdafx.h"
#include "UI/Voice/VoiceIcons.h"

#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/NewUI/NewUICommon.h"

// The voice glyphs used to be built out of ~30 solid RenderColor quads each,
// with the outline faked by redrawing the whole table at eight neighbour
// offsets. That is the same technique the ten HUD button faces used before
// fd08a307, and it has the same two problems: no anti-aliasing (every edge is
// a hard staircase, and every sub-pixel detail either snaps to a full pixel or
// disappears) and an outline that smears instead of ringing the silhouette.
//
// They are now the same kind of asset as the other ten LuxUI icons: an OZT
// rasterized 8x oversampled by tools/make_hud_button_icons.py and box-filtered
// down, so the alpha is fractional and the ink outline is a true dilation of
// the glyph mask. Frames are stacked vertically, frame 0 active and frame 1
// muted (dimmed metal plus a slash) — the muted state is baked into the art
// rather than being the live glyph at half brightness, which at 14 px was the
// only thing distinguishing "mic on" from "mic off".
//
// The four public entry points, their signatures, their centre-of-icon anchor
// and the meaning of `enabled` are unchanged, so MiniMapCorner's buttons,
// Chat's bubble marker and VoiceSpeakingIndicator's world overlay keep their
// existing hitboxes, positions and toggle logic.
namespace
{
    // Frame sizes must match VOICE_ICONS in tools/make_hud_button_icons.py.
    constexpr float kMicW = 16.f;
    constexpr float kMicH = 23.f;
    constexpr float kSoundW = 20.f;
    constexpr float kSoundH = 17.f;
    constexpr int kFrameCount = 2;   // 0 = active, 1 = muted

    constexpr float kWorldLift = 12.f;  // float the world icon above the anchor

    bool s_loaded = false;
    bool s_micOk = false;
    bool s_soundOk = false;

    // One frame of a vertically stacked sheet, centred on (centerX, centerY)
    // and scaled about that centre — the anchor the old quad tables used.
    void DrawFrame(GLuint image, float texW, float texH, int frame,
        float centerX, float centerY, float scale)
    {
        const float w = texW * scale;
        const float h = texH * scale;

        EnableAlphaTest();
        SEASON3B::RenderImageStretch(image,
            centerX - (w * 0.5f), centerY - (h * 0.5f), w, h,
            0.f, texH * static_cast<float>(frame), texW, texH);
        EndRenderColor();
    }

    void DrawMicrophone(float centerX, float centerY, float scale, bool enabled)
    {
        if (!s_micOk)
            return;
        DrawFrame(BITMAP_LUXUI_VOICE_MIC, kMicW, kMicH, enabled ? 0 : 1,
            centerX, centerY, scale);
    }

    void DrawSpeaker(float centerX, float centerY, float scale, bool enabled)
    {
        if (!s_soundOk)
            return;
        DrawFrame(BITMAP_LUXUI_VOICE_SOUND, kSoundW, kSoundH, enabled ? 0 : 1,
            centerX, centerY, scale);
    }
}

namespace UI::Voice
{
    void LoadIcons()
    {
        if (s_loaded)
            return;
        s_loaded = true;
        s_micOk = LoadBitmap(L"Interface\\LuxUI\\voice_mic.tga",
            BITMAP_LUXUI_VOICE_MIC, GL_LINEAR, GL_CLAMP_TO_EDGE);
        s_soundOk = LoadBitmap(L"Interface\\LuxUI\\voice_sound.tga",
            BITMAP_LUXUI_VOICE_SOUND, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void UnloadIcons()
    {
        if (!s_loaded)
            return;
        s_loaded = false;
        if (s_micOk)
            DeleteBitmap(BITMAP_LUXUI_VOICE_MIC);
        if (s_soundOk)
            DeleteBitmap(BITMAP_LUXUI_VOICE_SOUND);
        s_micOk = false;
        s_soundOk = false;
    }

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
