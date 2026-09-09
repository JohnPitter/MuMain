#include "stdafx.h"
#include "UI/Radio/RadioHud.h"

#include <cwchar>

#include "Audio/DSPlaySound.h"
#include "Audio/Radio/RadioPlayer.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace
{
    // Button: the same native empty plate + size as MiniMapCorner's voice
    // mic/sound buttons, mirrored to the left screen edge, vertically
    // centered.
    constexpr float kButtonWidth = 36.f;
    constexpr float kButtonHeight = 23.f;

    // The Radio_icon.OZT sheet stacks two 256x256 frames vertically
    // (0 = on, 1 = off). The glyph is drawn at a fixed on-screen size, so the
    // draw scale is a fraction of the art frame.
    constexpr float kIconFrame = 256.f;
    constexpr float kIconDrawSize = 15.f;
    constexpr float kIconScale = kIconDrawSize / kIconFrame;

    // Now-playing label above the button.
    constexpr float kLabelWidth = 170.f;
    constexpr float kLabelHeight = 14.f;
    constexpr float kLabelGap = 3.f;

    constexpr BYTE kLabelRed = 220;
    constexpr BYTE kLabelGreen = 200;
    constexpr BYTE kLabelBlue = 140;

    const wchar_t* const kRadioTooltip = L"R\u00e1dio: esta\u00e7\u00f5es, volume e liga/desliga";
    const wchar_t* const kRadioOffLabel = L"R\u00e1dio desligada";

    bool s_iconReady = false;
    SEASON3B::CNewUIButton s_BtnRadio;
    bool s_buttonReady = false;

    // Last label text shipped to the renderer; only overwritten when the
    // radio state actually changes, so a frame with no update does zero work
    // beyond the copy-free draw.
    wchar_t s_labelText[160] = L"";
    bool s_labelInitialized = false;

    void ButtonOrigin(float* outX, float* outY)
    {
        // Mirrored anchor of the voice dock (right edge, centered): glued to
        // the LEFT screen edge at the same height.
        *outX = 0.f;
        *outY = REFERENCE_HEIGHT / 2.f;
    }

    void BuildLabelText(const Audio::Radio::RadioEngine::Snapshot& status, wchar_t* out, std::size_t outChars)
    {
        const wchar_t* station = status.stationName[0] != L'\0' ? status.stationName
            : Audio::Radio::GetStationName(Audio::Radio::GetSelectedStation());

        switch (status.state)
        {
        case Audio::Radio::RadioEngine::State::Playing:
            if (status.nowPlaying[0] != L'\0')
            {
                wcsncpy_s(out, outChars, status.nowPlaying, _TRUNCATE);
                return;
            }
            if (station[0] != L'\0')
            {
                wcsncpy_s(out, outChars, station, _TRUNCATE);
                return;
            }
            wcsncpy_s(out, outChars, L"Ao vivo", _TRUNCATE);
            return;

        case Audio::Radio::RadioEngine::State::Connecting:
            wcsncpy_s(out, outChars, L"Conectando...", _TRUNCATE);
            return;

        case Audio::Radio::RadioEngine::State::Reconnecting:
            if (station[0] != L'\0')
            {
                // "offline" suffix keeps the station visible while the engine
                // silently retries in the background.
                swprintf_s(out, outChars, L"%ls (offline)", station);
                return;
            }
            wcsncpy_s(out, outChars, L"Offline", _TRUNCATE);
            return;

        case Audio::Radio::RadioEngine::State::Off:
        default:
            wcsncpy_s(out, outChars, kRadioOffLabel, _TRUNCATE);
            return;
        }
    }

    void UpdateLabel()
    {
        Audio::Radio::RadioEngine::Snapshot status;
        Audio::Radio::GetStatus(status);

        wchar_t text[160] = {};
        BuildLabelText(status, text, std::size(text));

        if (!s_labelInitialized || wcsncmp(s_labelText, text, std::size(text)) != 0)
        {
            wcsncpy_s(s_labelText, text, _TRUNCATE);
            s_labelInitialized = true;
        }
    }

    void RenderLabel(float buttonX, float buttonY)
    {
        float labelX = buttonX + (kButtonWidth - kLabelWidth) * 0.5f;
        if (labelX < 2.f)
        {
            labelX = 2.f;
        }
        const float labelY = buttonY - kLabelHeight - kLabelGap;

        EnableAlphaTest();
        glColor4f(0.f, 0.f, 0.f, 0.55f);
        RenderColor(labelX, labelY, kLabelWidth, kLabelHeight);
        EndRenderColor();

        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(kLabelRed, kLabelGreen, kLabelBlue, 255);
        g_pRenderText->RenderText(static_cast<int>(labelX), static_cast<int>(labelY),
            s_labelText, static_cast<int>(kLabelWidth), static_cast<int>(kLabelHeight), RT3_SORT_CENTER);
    }

    void RenderIcon(float centerX, float centerY, bool enabled)
    {
        if (!s_iconReady)
        {
            return;
        }

        const float drawSize = kIconFrame * kIconScale;
        const float srcY = kIconFrame * (enabled ? 0.f : 1.f);

        EnableAlphaTest();
        SEASON3B::RenderImageStretch(BITMAP_LUXUI_RADIO,
            centerX - (drawSize * 0.5f), centerY - (drawSize * 0.5f), drawSize, drawSize,
            0.f, srcY, kIconFrame, kIconFrame);
        EndRenderColor();
    }

    void EnsureButton()
    {
        float x = 0.f, y = 0.f;
        ButtonOrigin(&x, &y);

        if (!s_buttonReady)
        {
            const int nativeBtn = SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL;
            s_BtnRadio.ChangeButtonImgState(true, nativeBtn, true);
            s_BtnRadio.ChangeButtonInfo(static_cast<int>(x), static_cast<int>(y),
                static_cast<int>(kButtonWidth), static_cast<int>(kButtonHeight));
            s_BtnRadio.ChangeToolTipText(&kRadioTooltip, 0);
            // Push the tooltip to the RIGHT: the button sits on the left screen
            // edge, so the voice dock's leftward offset would clip off-screen.
            s_BtnRadio.MoveTextTipPos(90, 9);
            s_buttonReady = true;
        }
        else
        {
            s_BtnRadio.ChangeButtonInfo(static_cast<int>(x), static_cast<int>(y),
                static_cast<int>(kButtonWidth), static_cast<int>(kButtonHeight));
        }
    }
}

namespace UI::Radio
{
    void LoadIcon()
    {
        if (s_iconReady)
        {
            return;
        }
        s_iconReady = LoadBitmap(L"Interface\\Radio_icon.tga",
            BITMAP_LUXUI_RADIO, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void UnloadIcon()
    {
        if (!s_iconReady)
        {
            return;
        }
        DeleteBitmap(BITMAP_LUXUI_RADIO);
        s_iconReady = false;
    }

    void RenderHud()
    {
        EnsureButton();

        const bool enabled = Audio::Radio::IsEnabled();

        if (s_BtnRadio.UpdateMouseEvent())
        {
            g_pNewUISystem->Toggle(SEASON3B::INTERFACE_RADIO);
            PlayBuffer(SOUND_CLICK01);
        }

        s_BtnRadio.ChangeAlpha(enabled ? 1.f : 0.72f, false);
        s_BtnRadio.Render();

        const POINT pos = s_BtnRadio.GetPos();
        RenderIcon(pos.x + (kButtonWidth * 0.5f), pos.y + (kButtonHeight * 0.5f), enabled);

        UpdateLabel();
        RenderLabel(static_cast<float>(pos.x), static_cast<float>(pos.y));
        EnableAlphaTest();
    }
}
