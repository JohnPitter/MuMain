#include "stdafx.h"
#include "UI/NewUI/HUD/MiniMapCorner.h"

#include <algorithm>
#include <cmath>

#include "Audio/DSPlaySound.h"
#include "Audio/VoiceChat.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/HUD/NewUIMiniMap.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace
{
    // Map viewport size; frame wraps map + voice footer like MacroUI / MU Helper.
    constexpr float kMapSize = 128.f;
    constexpr float kFooterH = 22.f;
    constexpr float kPad = 4.f;
    constexpr float kFrameW = kMapSize + (kPad * 2.f);
    constexpr float kFrameH = kMapSize + kFooterH + (kPad * 2.f);
    constexpr float kMarginTop = 0.f;
    constexpr float kMarginRight = 0.f;
    constexpr float kZoomSpan = 0.16f;

    // Native mini_map_ui_* frame (same UVs as CNewUIMiniMap::Render).
    constexpr float kFrameUv = 41.7f / 64.f;
    constexpr float kFrameUvLine = 1.f;
    constexpr float kCornerSize = 35.f;
    constexpr float kLineW = 35.f;
    constexpr float kLineH = 6.f;

    constexpr float kVoiceBtnW = 20.f;
    constexpr float kVoiceBtnH = 14.f;
    constexpr float kVoiceGap = 4.f;
    constexpr unsigned char kVoiceEnabledColor = 126;
    constexpr unsigned char kVoiceDisabledColor = 164;
    const wchar_t* const kVoiceMicrophoneTooltip = L"Voz: ligar ou desligar o microfone";
    const wchar_t* const kVoiceListeningTooltip = L"Voz: ligar ou desligar a escuta";

    bool s_visible = true;
    bool s_voiceReady = false;
    SEASON3B::CNewUIButton s_BtnVoiceMicrophone;
    SEASON3B::CNewUIButton s_BtnVoiceListening;

    void FrameOrigin(float* outX, float* outY)
    {
        *outX = REFERENCE_WIDTH - kFrameW - kMarginRight;
        *outY = kMarginTop;
    }

    void DrawNativeFrame(float frameX, float frameY)
    {
        EnableAlphaTest();
        glColor4f(1.f, 1.f, 1.f, 1.f);

        const int horiz = static_cast<int>(std::ceil(kFrameW / kLineW)) + 1;
        for (int i = 0; i < horiz; ++i)
        {
            SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2,
                frameX + (i * kLineW), frameY,
                kLineW, kLineH, 0.f, 1.f, kFrameUv, -kFrameUvLine);
            SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2,
                frameX + (i * kLineW), frameY + kFrameH - kLineH,
                kLineW, kLineH, 0.f, 0.f, kFrameUv, kFrameUvLine);
        }

        const int vert = static_cast<int>(std::ceil(kFrameH / (kLineW - 3.f))) + 1;
        for (int i = 0; i < vert; ++i)
        {
            RenderBitmapRotate(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2,
                frameX + (kLineH / 2.f), frameY + (i * (kLineW - 3.f)),
                kLineW, kLineH, -90.f, 0.f, 0.f, kFrameUv, kFrameUvLine);
            RenderBitmapRotate(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2,
                frameX + kFrameW - (kLineH / 2.f), frameY + (i * (kLineW - 3.f)),
                kLineW, kLineH, 90.f, 0.f, 0.f, kFrameUv, kFrameUvLine);
        }

        // Corner ornaments (native mini_map_ui_corner.tga).
        SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 1,
            frameX, frameY, kCornerSize, kCornerSize, 0.f, 0.f, kFrameUv, kFrameUv);
        SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 1,
            frameX + kFrameW - kCornerSize, frameY, kCornerSize, kCornerSize,
            kFrameUv, 0.f, -kFrameUv, kFrameUv);
        SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 1,
            frameX, frameY + kFrameH - kCornerSize, kCornerSize, kCornerSize,
            0.f, kFrameUv, kFrameUv, -kFrameUv);
        SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 1,
            frameX + kFrameW - kCornerSize, frameY + kFrameH - kCornerSize, kCornerSize, kCornerSize,
            kFrameUv, kFrameUv, -kFrameUv, -kFrameUv);
    }

    void EnsureVoiceButtons(float frameX, float frameY)
    {
        // Inside the frame footer — same idea as MU Helper controls in its bar.
        const float footerY = frameY + kPad + kMapSize + 3.f;
        const float listenX = frameX + kFrameW - kPad - kVoiceBtnW;
        const float micX = listenX - kVoiceGap - kVoiceBtnW;

        if (!s_voiceReady)
        {
            s_BtnVoiceMicrophone.ChangeButtonImgState(1, BITMAP_HERO_POSITION_INFO_BEGIN + 3, 1, 0, 1);
            s_BtnVoiceMicrophone.ChangeButtonInfo(static_cast<int>(micX), static_cast<int>(footerY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceMicrophone.ChangeToolTipText(&kVoiceMicrophoneTooltip, 0);
            s_BtnVoiceMicrophone.MoveTextTipPos(-20, 9);

            s_BtnVoiceListening.ChangeButtonImgState(1, BITMAP_HERO_POSITION_INFO_BEGIN + 3, 1, 0, 1);
            s_BtnVoiceListening.ChangeButtonInfo(static_cast<int>(listenX), static_cast<int>(footerY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceListening.ChangeToolTipText(&kVoiceListeningTooltip, 0);
            s_BtnVoiceListening.MoveTextTipPos(-20, 9);
            s_voiceReady = true;
        }
        else
        {
            s_BtnVoiceMicrophone.ChangeButtonInfo(static_cast<int>(micX), static_cast<int>(footerY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceListening.ChangeButtonInfo(static_cast<int>(listenX), static_cast<int>(footerY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
        }
    }

    void RenderVoiceButton(SEASON3B::CNewUIButton& button, const wchar_t* icon, bool enabled)
    {
        button.Render();
        const POINT pos = button.GetPos();
        g_pRenderText->SetFont(g_hFontBold);
        const unsigned char color = enabled ? kVoiceEnabledColor : kVoiceDisabledColor;
        g_pRenderText->SetTextColor(color, enabled ? 255 : color, color, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(pos.x, pos.y + 1, icon, static_cast<int>(kVoiceBtnW), 11, RT3_SORT_CENTER);
    }

    void HandleVoiceInput(float frameX, float frameY)
    {
        EnsureVoiceButtons(frameX, frameY);
        if (s_BtnVoiceMicrophone.UpdateMouseEvent())
        {
            VoiceChat::ToggleMicrophone();
            PlayBuffer(SOUND_CLICK01);
        }
        if (s_BtnVoiceListening.UpdateMouseEvent())
        {
            VoiceChat::ToggleListening();
            PlayBuffer(SOUND_CLICK01);
        }
    }

    void DrawVoiceActions(float frameX, float frameY)
    {
        EnsureVoiceButtons(frameX, frameY);
        RenderVoiceButton(s_BtnVoiceMicrophone, L"M", VoiceChat::IsMicrophoneEnabled());
        RenderVoiceButton(s_BtnVoiceListening, L"S", VoiceChat::IsListeningEnabled());
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
    }
}

namespace UI::HUD::MiniMap
{
    void ToggleVisible()
    {
        s_visible = !s_visible;
    }

    bool IsVisible()
    {
        return s_visible;
    }

    void GetBoxRect(float* outX, float* outY, float* outSize)
    {
        float x = 0.f, y = 0.f;
        FrameOrigin(&x, &y);
        if (outX) *outX = x;
        if (outY) *outY = y;
        if (outSize) *outSize = kFrameW;
    }

    void Render()
    {
        if (!Hero || !g_pNewUIMiniMap || !g_pNewUIMiniMap->m_bSuccess)
            return;

        float frameX = 0.f, frameY = 0.f;
        FrameOrigin(&frameX, &frameY);
        const float mapX = frameX + kPad;
        const float mapY = frameY + kPad;

        EnableAlphaTest();
        glColor4f(1.f, 1.f, 1.f, 1.f);

        // Dim panel behind map+footer (MU Helper style filled chrome).
        EnableAlphaBlend();
        glColor4f(0.f, 0.f, 0.f, 0.55f);
        RenderColor(frameX, frameY, kFrameW, kFrameH);
        glColor4f(1.f, 1.f, 1.f, 1.f);
        EnableAlphaTest();

        HandleVoiceInput(frameX, frameY);

        if (s_visible)
        {
            const int clipX = static_cast<int>(ConvertPosX(mapX));
            const int clipY = static_cast<int>(ConvertPosY(mapY));
            const int clipW = static_cast<int>(ConvertX(kMapSize));
            const int clipH = static_cast<int>(ConvertY(kMapSize));
            EnableScissorTest();
            SetScissor(clipX, static_cast<int>(WindowHeight) - clipY - clipH, clipW, clipH);

            const float halfSpan = kZoomSpan / 2.f;
            const float centerU = std::clamp(static_cast<float>(Hero->PositionY) / 256.f, halfSpan, 1.f - halfSpan);
            const float centerV = std::clamp(static_cast<float>(Hero->PositionX) / 256.f, halfSpan, 1.f - halfSpan);

            glColor4f(1.f, 1.f, 1.f, 1.f);
            SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE, mapX, mapY, kMapSize, kMapSize,
                centerU - halfSpan, centerV - halfSpan, kZoomSpan, kZoomSpan);

            glColor4f(0.1f, 0.1f, 0.1f, 1.f);
            RenderColor(mapX + (kMapSize / 2.f) - 2.5f, mapY + (kMapSize / 2.f) - 2.5f, 5.f, 5.f);
            glColor4f(1.f, 0.9f, 0.2f, 1.f);
            RenderColor(mapX + (kMapSize / 2.f) - 2.f, mapY + (kMapSize / 2.f) - 2.f, 4.f, 4.f);
            glColor4f(1.f, 1.f, 1.f, 1.f);
            DisableScissorTest();
        }

        // Frame + voice on top (corners over map; M/S inside footer).
        DrawNativeFrame(frameX, frameY);
        DrawVoiceActions(frameX, frameY);
        EnableAlphaTest();
    }
}
