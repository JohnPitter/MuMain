#include "stdafx.h"
#include "UI/NewUI/HUD/MiniMapCorner.h"

#include <algorithm>
#include <cmath>

#include "Audio/DSPlaySound.h"
#include "Audio/VoiceChat.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "World/MapInfra/MapManager.h"
#include "UI/NewUI/HUD/NewUIMiniMap.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Voice/VoiceIcons.h"
#include "GameLogic/Social/PartyManager.h"

namespace
{
    // Square map wrapped by the same 14px table chrome as MU Helper.
    // Voice buttons dock to the middle of the right screen edge.
    constexpr float kMapSize = 128.f;
    constexpr float kPad = 6.f;
    constexpr float kFrameW = kMapSize + (kPad * 2.f);
    constexpr float kFrameH = kMapSize + (kPad * 2.f);
    constexpr float kMarginTop = 0.f;
    constexpr float kMarginRight = 0.f;
    constexpr float kZoomSpan = 0.16f;
    constexpr float kTable = 14.f;

    constexpr float kVoiceBtnW = 36.f;
    constexpr float kVoiceBtnH = 23.f;
    constexpr float kVoiceGap = 4.f;
    constexpr float kVoiceIconScale = 0.58f;
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

    void VoiceOrigin(float* outX, float* outY)
    {
        // Glued to the right screen edge, vertically centered.
        *outX = REFERENCE_WIDTH - kVoiceBtnW;
        *outY = REFERENCE_HEIGHT / 2.f;
    }

    void DrawHelperFrame(float x, float y)
    {
        using Img = SEASON3B::CNewUIInventoryCtrl;

        EnableAlphaTest();
        glColor4f(0.f, 0.f, 0.f, 0.55f);
        RenderColor(x + 3.f, y + 2.f, kFrameW - 7.f, kFrameH - 7.f);
        EndRenderColor();

        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_TOP_LEFT, x, y, kTable, kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_TOP_RIGHT, x + kFrameW - kTable, y, kTable, kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_BOTTOM_LEFT, x, y + kFrameH - kTable, kTable, kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_BOTTOM_RIGHT,
            x + kFrameW - kTable, y + kFrameH - kTable, kTable, kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_TOP_PIXEL, x + 6.f, y, kFrameW - 12.f, kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_RIGHT_PIXEL,
            x + kFrameW - kTable, y + 6.f, kTable, kFrameH - kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_BOTTOM_PIXEL,
            x + 6.f, y + kFrameH - kTable, kFrameW - 12.f, kTable);
        SEASON3B::RenderImage(Img::IMAGE_ITEM_TABLE_LEFT_PIXEL, x, y + 6.f, kTable, kFrameH - kTable);
    }

    void EnsureVoiceButtons(float /*frameX*/, float /*frameY*/)
    {
        float voiceX = 0.f, voiceY = 0.f;
        VoiceOrigin(&voiceX, &voiceY);
        const float listenX = voiceX;
        const float micX = voiceX;
        const float footerY = voiceY;
        const float listenY = voiceY + kVoiceBtnH + kVoiceGap;

        if (!s_voiceReady)
        {
            const int nativeBtn = SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL;
            s_BtnVoiceMicrophone.ChangeButtonImgState(true, nativeBtn, true);
            s_BtnVoiceMicrophone.ChangeButtonInfo(static_cast<int>(micX), static_cast<int>(footerY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceMicrophone.ChangeToolTipText(&kVoiceMicrophoneTooltip, 0);
            s_BtnVoiceMicrophone.MoveTextTipPos(-90, 9);

            s_BtnVoiceListening.ChangeButtonImgState(true, nativeBtn, true);
            s_BtnVoiceListening.ChangeButtonInfo(static_cast<int>(listenX), static_cast<int>(listenY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceListening.ChangeToolTipText(&kVoiceListeningTooltip, 0);
            s_BtnVoiceListening.MoveTextTipPos(-90, 9);
            s_voiceReady = true;
        }
        else
        {
            s_BtnVoiceMicrophone.ChangeButtonInfo(static_cast<int>(micX), static_cast<int>(footerY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
            s_BtnVoiceListening.ChangeButtonInfo(static_cast<int>(listenX), static_cast<int>(listenY),
                static_cast<int>(kVoiceBtnW), static_cast<int>(kVoiceBtnH));
        }
    }

    void RenderVoiceButton(SEASON3B::CNewUIButton& button, bool isMicrophone, bool enabled)
    {
        button.ChangeAlpha(enabled ? 1.f : 0.72f, false);
        button.Render();
        const POINT pos = button.GetPos();
        const float centerX = static_cast<float>(pos.x) + (kVoiceBtnW * 0.5f);
        const float centerY = static_cast<float>(pos.y) + (kVoiceBtnH * 0.5f);
        if (isMicrophone)
            UI::Voice::DrawMicrophoneGlyph(centerX, centerY, kVoiceIconScale, enabled);
        else
            UI::Voice::DrawSpeakerGlyph(centerX, centerY, kVoiceIconScale, enabled);
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
        RenderVoiceButton(s_BtnVoiceMicrophone, true, VoiceChat::IsMicrophoneEnabled());
        RenderVoiceButton(s_BtnVoiceListening, false, VoiceChat::IsListeningEnabled());
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
        if (!s_visible)
        {
            if (outX) *outX = 0.f;
            if (outY) *outY = 0.f;
            if (outSize) *outSize = 0.f;
            return;
        }
        float x = 0.f, y = 0.f;
        FrameOrigin(&x, &y);
        if (outX) *outX = x;
        if (outY) *outY = y;
        if (outSize) *outSize = kFrameW;
    }

    void Render()
    {
        if (!s_visible)
            return;
        if (!Hero || !g_pNewUIMiniMap || !g_pNewUIMiniMap->m_bSuccess)
            return;

        float frameX = 0.f, frameY = 0.f;
        FrameOrigin(&frameX, &frameY);
        const float mapX = frameX + kPad;
        const float mapY = frameY + kPad;

        DrawHelperFrame(frameX, frameY);

        const int clipX = static_cast<int>(ConvertPosX(mapX));
        const int clipY = static_cast<int>(ConvertPosY(mapY));
        const int clipW = static_cast<int>(ConvertX(kMapSize));
        const int clipH = static_cast<int>(ConvertY(kMapSize));
        EnableScissorTest();
        SetScissor(clipX, static_cast<int>(WindowHeight) - clipY - clipH, clipW, clipH);

        const float halfSpan = kZoomSpan / 2.f;
        const float centerU = std::clamp(static_cast<float>(Hero->PositionY) / 256.f, halfSpan, 1.f - halfSpan);
        const float centerV = std::clamp(static_cast<float>(Hero->PositionX) / 256.f, halfSpan, 1.f - halfSpan);

        EnableAlphaTest();
        glColor4f(1.f, 1.f, 1.f, 1.f);
        SEASON3B::RenderImage(SEASON3B::CNewUIMiniMap::IMAGE_MINIMAP_INTERFACE, mapX, mapY, kMapSize, kMapSize,
            centerU - halfSpan, centerV - halfSpan, kZoomSpan, kZoomSpan);

        if (PartyNumber > 0)
        {
            g_pPartyManager->SyncLivePartyPositions();
            g_pPartyManager->RequestPartyListIfDue();
            for (int i = 0; i < PartyNumber; ++i)
            {
                const PARTY_t* member = &Party[i];
                if (member->Name[0] == 0)
                    continue;
                if (g_pPartyManager->IsLocalHero(member))
                    continue;
                if (member->Map != static_cast<BYTE>(gMapManager.WorldActive))
                    continue;

                const float memberU = static_cast<float>(member->y) / 256.f;
                const float memberV = static_cast<float>(member->x) / 256.f;
                const float dx = (memberU - centerU) / kZoomSpan;
                const float dy = (memberV - centerV) / kZoomSpan;
                const float px = mapX + (kMapSize / 2.f) + (dx * kMapSize);
                const float py = mapY + (kMapSize / 2.f) + (dy * kMapSize);
                if (px < mapX || py < mapY || px > mapX + kMapSize || py > mapY + kMapSize)
                    continue;

                glColor4f(0.05f, 0.15f, 0.2f, 1.f);
                RenderColor(px - 2.5f, py - 2.5f, 5.f, 5.f);
                glColor4f(0.25f, 0.85f, 1.f, 1.f);
                RenderColor(px - 2.f, py - 2.f, 4.f, 4.f);
            }
        }

        glColor4f(0.1f, 0.1f, 0.1f, 1.f);
        RenderColor(mapX + (kMapSize / 2.f) - 2.5f, mapY + (kMapSize / 2.f) - 2.5f, 5.f, 5.f);
        glColor4f(1.f, 0.9f, 0.2f, 1.f);
        RenderColor(mapX + (kMapSize / 2.f) - 2.f, mapY + (kMapSize / 2.f) - 2.f, 4.f, 4.f);
        EndRenderColor();
        DisableScissorTest();

        EnableAlphaTest();
    }

    void RenderCommands()
    {
        // Voice buttons live outside the map scissor; input + draw happen here.
        float voiceX = 0.f, voiceY = 0.f;
        VoiceOrigin(&voiceX, &voiceY);
        HandleVoiceInput(voiceX, voiceY);
        DrawVoiceActions(voiceX, voiceY);
        EnableAlphaTest();
    }
}
