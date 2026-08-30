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
    constexpr float kBoxSize = 136.f;
    constexpr float kMarginTop = 0.f;
    constexpr float kMarginRight = 0.f;
    constexpr float kZoomSpan = 0.16f;

    // Thin edge line only — skip the 35x35 corner ornaments. Those TGAs are
    // black RGB + alpha and intermittently draw as opaque black squares on
    // this small box (blend-state leaks from other HUD passes).
    constexpr float kFrameUv = 41.7f / 64.f;
    constexpr float kFrameUvLine = 1.f;
    constexpr float kFrameTileWidth = 35.f;
    constexpr float kFrameTileHeight = 6.f;

    constexpr float kVoiceBtnW = 18.f;
    constexpr float kVoiceBtnH = 13.f;
    constexpr float kVoiceGap = 4.f;
    constexpr float kVoiceBelow = 4.f;
    constexpr unsigned char kVoiceEnabledColor = 126;
    constexpr unsigned char kVoiceDisabledColor = 164;
    const wchar_t* const kVoiceMicrophoneTooltip = L"Voz: ligar ou desligar o microfone";
    const wchar_t* const kVoiceListeningTooltip = L"Voz: ligar ou desligar a escuta";

    bool s_visible = true;
    bool s_voiceReady = false;
    SEASON3B::CNewUIButton s_BtnVoiceMicrophone;
    SEASON3B::CNewUIButton s_BtnVoiceListening;

    void DrawEdgeFrame(float boxX, float boxY)
    {
        const int horizontalTiles = static_cast<int>(std::ceil(kBoxSize / kFrameTileWidth)) + 1;
        for (int i = 0; i < horizontalTiles; ++i)
        {
            SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2, boxX + (i * kFrameTileWidth), boxY,
                kFrameTileWidth, kFrameTileHeight, 0.f, 1.f, kFrameUv, -kFrameUvLine);
            SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2, boxX + (i * kFrameTileWidth), boxY + kBoxSize - kFrameTileHeight,
                kFrameTileWidth, kFrameTileHeight, 0.f, 0.f, kFrameUv, kFrameUvLine);
        }

        const int verticalTiles = static_cast<int>(std::ceil(kBoxSize / (kFrameTileWidth - 3.f))) + 1;
        for (int i = 0; i < verticalTiles; ++i)
        {
            RenderBitmapRotate(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2, boxX + (kFrameTileHeight / 2.f), boxY + (i * (kFrameTileWidth - 3.f)),
                kFrameTileWidth, kFrameTileHeight, -90.f, 0.f, 0.f, kFrameUv, kFrameUvLine);
            RenderBitmapRotate(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE + 2, boxX + kBoxSize - (kFrameTileHeight / 2.f), boxY + (i * (kFrameTileWidth - 3.f)),
                kFrameTileWidth, kFrameTileHeight, 90.f, 0.f, 0.f, kFrameUv, kFrameUvLine);
        }
    }

    void EnsureVoiceButtons(float boxX, float boxY)
    {
        const float rowY = boxY + kBoxSize + kVoiceBelow;
        const float micX = boxX + kBoxSize - (kVoiceBtnW * 2.f) - kVoiceGap;
        const float listenX = boxX + kBoxSize - kVoiceBtnW;

        if (!s_voiceReady)
        {
            // Reuse MacroUI_Setup.tga already loaded by the position bar.
            s_BtnVoiceMicrophone.ChangeButtonImgState(1, BITMAP_HERO_POSITION_INFO_BEGIN + 3, 1, 0, 1);
            s_BtnVoiceMicrophone.ChangeButtonInfo(static_cast<int>(micX), static_cast<int>(rowY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceMicrophone.ChangeToolTipText(&kVoiceMicrophoneTooltip, 0);
            s_BtnVoiceMicrophone.MoveTextTipPos(-20, 9);

            s_BtnVoiceListening.ChangeButtonImgState(1, BITMAP_HERO_POSITION_INFO_BEGIN + 3, 1, 0, 1);
            s_BtnVoiceListening.ChangeButtonInfo(static_cast<int>(listenX), static_cast<int>(rowY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceListening.ChangeToolTipText(&kVoiceListeningTooltip, 0);
            s_BtnVoiceListening.MoveTextTipPos(-20, 9);
            s_voiceReady = true;
        }
        else
        {
            s_BtnVoiceMicrophone.ChangeButtonInfo(static_cast<int>(micX), static_cast<int>(rowY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceListening.ChangeButtonInfo(static_cast<int>(listenX), static_cast<int>(rowY),
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

    void RenderVoiceActions(float boxX, float boxY)
    {
        EnsureVoiceButtons(boxX, boxY);

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
        if (outX) *outX = REFERENCE_WIDTH - kBoxSize - kMarginRight;
        if (outY) *outY = kMarginTop;
        if (outSize) *outSize = kBoxSize;
    }

    void Render()
    {
        if (!Hero || !g_pNewUIMiniMap || !g_pNewUIMiniMap->m_bSuccess)
            return;

        const float boxX = REFERENCE_WIDTH - kBoxSize - kMarginRight;
        const float boxY = kMarginTop;

        // Voice actions stay below the minimap slot even when the map itself
        // is hidden (Caps Lock), so players can still mute/unmute.
        EnableAlphaTest();
        RenderVoiceActions(boxX, boxY);

        if (!s_visible)
            return;

        EnableAlphaTest();

        const int clipX = static_cast<int>(ConvertPosX(boxX));
        const int clipY = static_cast<int>(ConvertPosY(boxY));
        const int clipW = static_cast<int>(ConvertX(kBoxSize));
        const int clipH = static_cast<int>(ConvertY(kBoxSize));
        EnableScissorTest();
        SetScissor(clipX, static_cast<int>(WindowHeight) - clipY - clipH, clipW, clipH);

        const float halfSpan = kZoomSpan / 2.f;
        const float centerU = std::clamp(static_cast<float>(Hero->PositionY) / 256.f, halfSpan, 1.f - halfSpan);
        const float centerV = std::clamp(static_cast<float>(Hero->PositionX) / 256.f, halfSpan, 1.f - halfSpan);

        glColor4f(1.f, 1.f, 1.f, 1.f);
        SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE, boxX, boxY, kBoxSize, kBoxSize,
            centerU - halfSpan, centerV - halfSpan, kZoomSpan, kZoomSpan);

        EnableAlphaTest();
        glColor4f(1.f, 1.f, 1.f, 1.f);
        DrawEdgeFrame(boxX, boxY);

        glColor4f(0.1f, 0.1f, 0.1f, 1.f);
        RenderColor(boxX + (kBoxSize / 2.f) - 2.5f, boxY + (kBoxSize / 2.f) - 2.5f, 5.f, 5.f);
        glColor4f(1.f, 0.9f, 0.2f, 1.f);
        RenderColor(boxX + (kBoxSize / 2.f) - 2.f, boxY + (kBoxSize / 2.f) - 2.f, 4.f, 4.f);
        glColor4f(1.f, 1.f, 1.f, 1.f);

        DisableScissorTest();
        EnableAlphaTest();
    }
}
