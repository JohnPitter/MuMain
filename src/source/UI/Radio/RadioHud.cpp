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
#include "UI/Radio/RadioMarquee.h"
#include "UI/Radio/RadioStatusText.h"

namespace
{
    // Button: the same native empty plate + size as MiniMapCorner's voice
    // mic/sound buttons, mirrored to the left screen edge, vertically
    // centered. Stands alone: the "tocando agora" text moved to the marquee
    // strip above the bottom functionality bar.
    constexpr float kButtonWidth = 36.f;
    constexpr float kButtonHeight = 23.f;

    // The Radio_icon.OZT sheet stacks two 256x256 frames vertically
    // (0 = on, 1 = off). The glyph is drawn at a fixed on-screen size, so the
    // draw scale is a fraction of the art frame.
    constexpr float kIconFrame = 256.f;
    constexpr float kIconDrawSize = 15.f;
    constexpr float kIconScale = kIconDrawSize / kIconFrame;

    // "Tocando agora" marquee strip: spans the FULL width of the bottom
    // functionality bar (the potions/skills bar — Q/W/E/R + slots — is drawn
    // by CNewUIMainFrameWindow::RenderFrame as IMAGE_MENU_1 + IMAGE_MENU_2 +
    // IMAGE_MENU_3 = 256 + 128 + 256 = REFERENCE_WIDTH wide, kBarHeight tall,
    // anchored at the bottom edge). The strip sits directly ABOVE the bar,
    // aligned to it, and the scrolling text is clipped to exactly this
    // rectangle — it spans the whole screen width, so the GL viewport clips
    // the slide at both strip edges and the loop never bleeds outside the
    // bar band.
    constexpr float kBarHeight = 51.f;
    constexpr float kStripWidth = static_cast<float>(REFERENCE_WIDTH);  // 640 = full bar width
    constexpr float kStripHeight = 14.f;
    constexpr float kStripGap = 1.f;
    constexpr float kMarqueeSpeedPxPerSec = 64.f;

    constexpr BYTE kLabelRed = 220;
    constexpr BYTE kLabelGreen = 200;
    constexpr BYTE kLabelBlue = 140;

    const wchar_t* const kRadioTooltip = L"R\u00e1dio: esta\u00e7\u00f5es, volume e liga/desliga";

    bool s_iconReady = false;
    SEASON3B::CNewUIButton s_BtnRadio;
    bool s_buttonReady = false;

    // Snapshot of the status text the marquee is showing. Rebuilt only when
    // the engine state/wording actually changes, so an ordinary frame does
    // zero string work and zero allocation — the render step is just an
    // offset computed from a timestamp plus a cached text width.
    wchar_t s_labelText[160] = L"";
    float s_labelWidthPx = 0.f;
    bool s_labelValid = false;

    void ButtonOrigin(float* outX, float* outY)
    {
        // Mirrored anchor of the voice dock (right edge, centered): glued to
        // the LEFT screen edge at the same height.
        *outX = 0.f;
        *outY = REFERENCE_HEIGHT / 2.f;
    }

    UI::Radio::RadioStatusKind MapStatusKind(Audio::Radio::RadioEngine::State state)
    {
        switch (state)
        {
        case Audio::Radio::RadioEngine::State::Playing:
            return UI::Radio::RadioStatusKind::Playing;
        case Audio::Radio::RadioEngine::State::Connecting:
            return UI::Radio::RadioStatusKind::Connecting;
        case Audio::Radio::RadioEngine::State::Buffering:
            return UI::Radio::RadioStatusKind::Buffering;
        case Audio::Radio::RadioEngine::State::Reconnecting:
            return UI::Radio::RadioStatusKind::Reconnecting;
        case Audio::Radio::RadioEngine::State::Off:
        default:
            return UI::Radio::RadioStatusKind::Off;
        }
    }

    void MeasureLabelWidth()
    {
        SIZE size {};
        const int length = static_cast<int>(wcslen(s_labelText));
        if (length > 0)
        {
            GetTextExtentPoint32(g_pRenderText->GetFontDC(), s_labelText, length, &size);
        }
        s_labelWidthPx = static_cast<float>(size.cx) / g_fScreenRate_x;
    }

    void UpdateLabel(const Audio::Radio::RadioEngine::Snapshot& status,
        UI::Radio::RadioStatusKind kind)
    {
        const wchar_t* station = status.stationName[0] != L'\0' ? status.stationName
            : Audio::Radio::GetStationName(Audio::Radio::GetSelectedStation());

        wchar_t text[160] = {};
        UI::Radio::BuildRadioStatusText(kind, station, status.nowPlaying, text, std::size(text));

        if (!s_labelValid || wcsncmp(s_labelText, text, std::size(text)) != 0)
        {
            wcsncpy_s(s_labelText, text, _TRUNCATE);
            MeasureLabelWidth();
            s_labelValid = true;
        }
    }

    void RenderMarquee()
    {
        const float stripY = static_cast<float>(REFERENCE_HEIGHT) - kBarHeight
            - kStripHeight - kStripGap;

        // Translucent band along the whole bar, in the old label's style.
        EnableAlphaTest();
        glColor4f(0.f, 0.f, 0.f, 0.55f);
        RenderColor(0.f, stripY, kStripWidth, kStripHeight);
        EndRenderColor();

        if (s_labelText[0] == L'\0')
        {
            return;
        }

        // Infinite left-to-right loop: the text enters at the strip's left
        // edge and slides right until it fully exits at the right edge, then
        // repeats. The strip spans the entire screen width, so the viewport
        // clips the text at exactly the bar's rectangle.
        const float x = UI::Radio::MarqueeOffsetPx(timeGetTime(), s_labelWidthPx,
            kStripWidth, kMarqueeSpeedPxPerSec);

        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(kLabelRed, kLabelGreen, kLabelBlue, 255);
        g_pRenderText->RenderText(static_cast<int>(x), static_cast<int>(stripY + 1),
            s_labelText, 0, 0, RT3_SORT_LEFT);
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
            // The native empty plate mirrored to face the left screen edge
            // (BITMAP_LUXUI_RADIO_PLATE): same 54x69 3-frame sheet, flipped,
            // so the "abertura" reads as coming out of the screen edge.
            s_BtnRadio.ChangeButtonImgState(true, BITMAP_LUXUI_RADIO_PLATE, true);
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
        // Mirrored plate: same 3-frame sheet as the native empty button, but
        // with the bevel facing the left screen edge (owner request).
        LoadBitmap(L"Interface\\Radio_btn_plate.tga",
            BITMAP_LUXUI_RADIO_PLATE, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    void UnloadIcon()
    {
        if (!s_iconReady)
        {
            return;
        }
        DeleteBitmap(BITMAP_LUXUI_RADIO);
        DeleteBitmap(BITMAP_LUXUI_RADIO_PLATE);
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

        // Marquee gate (owner request): the "tocando agora" strip exists ONLY
        // while the radio is enabled AND actually playing. Desligada/offline a
        // faixa some — no band, no text.
        Audio::Radio::RadioEngine::Snapshot status;
        Audio::Radio::GetStatus(status);
        const UI::Radio::RadioStatusKind kind = MapStatusKind(status.state);
        if (UI::Radio::MarqueeVisible(enabled, kind))
        {
            UpdateLabel(status, kind);
            RenderMarquee();
        }
        EnableAlphaTest();
    }
}
