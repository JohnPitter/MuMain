#include "stdafx.h"

#include "UI/Radio/RadioWindow.h"

#include <algorithm>
#include <cwchar>

#include "Audio/DSPlaySound.h"
#include "Audio/Radio/RadioPlayer.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/Radio/RadioStatusText.h"

using namespace SEASON3B;

namespace
{
    // Title / header ink, matching the other LuxView pt-BR windows.
    constexpr BYTE kTitleRed = 255;
    constexpr BYTE kTitleGreen = 220;
    constexpr BYTE kTitleBlue = 120;

    constexpr BYTE kLabelRed = 200;
    constexpr BYTE kLabelGreen = 200;
    constexpr BYTE kLabelBlue = 200;

    constexpr BYTE kStatusRed = 150;
    constexpr BYTE kStatusGreen = 180;
    constexpr BYTE kStatusBlue = 215;

    // Background tiling grid and header 3-slice geometry — the validated house
    // pattern shared with the Novidades popup and the event schedule window.
    constexpr float kBackSrcW = 190.f;
    constexpr float kBackSrcH = 429.f;
    constexpr float kHeaderCapW = 28.f;
    constexpr float kHeaderMidSrcX = 60.f;
    constexpr float kHeaderMidSrcW = 70.f;

    const wchar_t* const kWindowTitle = L"R\u00e1dio";
    const wchar_t* const kCloseText = L"Fechar (ESC)";
    const wchar_t* const kNoStations = L"(sem esta\u00e7\u00f5es \u2014 edite Data\\Local\\RadioStations.ini)";
    const wchar_t* const kPowerOnText = L"Desligar";
    const wchar_t* const kPowerOffText = L"Ligar";
}

namespace SEASON3B
{
    CNewUIRadioWindow::CNewUIRadioWindow()
        : m_pNewUIMng(NULL)
    {
        m_Pos.x = 0;
        m_Pos.y = 0;
    }

    CNewUIRadioWindow::~CNewUIRadioWindow()
    {
        Release();
    }

    bool CNewUIRadioWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
    {
        if (pNewUIMng == NULL)
        {
            return false;
        }

        m_pNewUIMng = pNewUIMng;
        m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_RADIO, this);

        LoadImages();
        SetPos(x, y);
        InitButtons();
        InitStationCombo();
        m_iVolumeLevel = Audio::Radio::GetVolume();
        RefreshPowerButton();
        Show(false);
        return true;
    }

    void CNewUIRadioWindow::Release()
    {
        UnloadImages();
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = NULL;
        }
    }

    void CNewUIRadioWindow::SetPos(int x, int y)
    {
        m_Pos.x = x;
        m_Pos.y = y;
        m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y,
            EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
        m_BtnPower.ChangeButtonInfo(m_Pos.x + POWER_BUTTON_X, m_Pos.y + POWER_BUTTON_Y,
            POWER_BUTTON_WIDTH, POWER_BUTTON_HEIGHT);
        m_StationCombo.SetPos(m_Pos.x + CONTENT_LEFT, m_Pos.y + COMBO_Y);
    }

    void CNewUIRadioWindow::InitButtons()
    {
        wchar_t closeText[256] = {};
        mu_swprintf(closeText, L"%ls", kCloseText);
        m_BtnExit.ChangeButtonImgState(true, IMAGE_RADIO_BTN_EXIT);
        m_BtnExit.ChangeToolTipText(closeText, true);

        m_BtnPower.ChangeButtonImgState(true, IMAGE_RADIO_BTN_POWER, true);
        m_BtnPower.SetFont(g_hFontBold);
    }

    void CNewUIRadioWindow::RefreshPowerButton()
    {
        m_BtnPower.ChangeText(Audio::Radio::IsEnabled() ? kPowerOnText : kPowerOffText);
    }

    void CNewUIRadioWindow::InitStationCombo()
    {
        m_StationNames.clear();
        m_StationLabels.clear();

        const int count = Audio::Radio::GetStationCount();
        for (int i = 0; i < count; ++i)
        {
            m_StationNames.push_back(Audio::Radio::GetStationName(i));
        }
        for (const std::wstring& name : m_StationNames)
        {
            m_StationLabels.push_back(name.c_str());
        }

        if (m_StationLabels.empty())
        {
            m_StationNames.push_back(kNoStations);
            m_StationLabels.push_back(m_StationNames.back().c_str());
        }

        m_StationCombo.Setup(m_Pos.x + CONTENT_LEFT, m_Pos.y + COMBO_Y, CONTENT_WIDTH, COMBO_HEIGHT,
            m_StationLabels.data(), static_cast<int>(m_StationLabels.size()),
            Audio::Radio::GetSelectedStation(), COMBO_MAX_VISIBLE);
    }

    void CNewUIRadioWindow::LoadImages()
    {
        LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_RADIO_BACK, GL_LINEAR);
        LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_RADIO_TOP, GL_LINEAR);
        LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_RADIO_LEFT, GL_LINEAR);
        LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_RADIO_RIGHT, GL_LINEAR);
        LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_RADIO_BOTTOM, GL_LINEAR);
        LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_RADIO_BTN_EXIT, GL_LINEAR);
    }

    void CNewUIRadioWindow::UnloadImages()
    {
        DeleteBitmap(IMAGE_RADIO_BACK);
        DeleteBitmap(IMAGE_RADIO_TOP);
        DeleteBitmap(IMAGE_RADIO_LEFT);
        DeleteBitmap(IMAGE_RADIO_RIGHT);
        DeleteBitmap(IMAGE_RADIO_BOTTOM);
        DeleteBitmap(IMAGE_RADIO_BTN_EXIT);
    }

    float CNewUIRadioWindow::GetLayerDepth()
    {
        // Above the regular windows (event schedule 4.6), below the news popup (5.2).
        return 4.7f;
    }

    float CNewUIRadioWindow::GetKeyEventOrder()
    {
        return 10.f;
    }

    // Same frame pieces and 3-slice geometry as the Novidades popup.
    void CNewUIRadioWindow::RenderWindowFrame(float x, float y, float w, float h)
    {
        EnableAlphaTest();
        glColor4f(1.f, 1.f, 1.f, 1.f);

        for (float oy = 0.f; oy < h; oy += kBackSrcH)
        {
            const float th = (oy + kBackSrcH > h) ? (h - oy) : kBackSrcH;
            for (float ox = 0.f; ox < w; ox += kBackSrcW)
            {
                const float tw = (ox + kBackSrcW > w) ? (w - ox) : kBackSrcW;
                RenderImage(IMAGE_RADIO_BACK, x + ox, y + oy, tw, th);
            }
        }

        RenderImageStretch(IMAGE_RADIO_TOP, x, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
            0.f, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));
        RenderImageStretch(IMAGE_RADIO_TOP, x + kHeaderCapW, y, w - kHeaderCapW * 2.f, float(FRAME_TOP_HEIGHT),
            kHeaderMidSrcX, 0.f, kHeaderMidSrcW, float(FRAME_TOP_HEIGHT));
        RenderImageStretch(IMAGE_RADIO_TOP, x + w - kHeaderCapW, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
            190.f - kHeaderCapW, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));

        const float middleHeight = h - float(FRAME_TOP_HEIGHT) - float(FRAME_BOTTOM_HEIGHT);
        RenderImageStretch(IMAGE_RADIO_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
            0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
        RenderImageStretch(IMAGE_RADIO_RIGHT, x + w - float(FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
            0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));

        RenderImageStretch(IMAGE_RADIO_BOTTOM, x, y + h - float(FRAME_BOTTOM_HEIGHT), w, float(FRAME_BOTTOM_HEIGHT),
            0.f, 0.f, 190.f, float(FRAME_BOTTOM_HEIGHT));
    }

    bool CNewUIRadioWindow::UpdateMouseEvent()
    {
        // A combo selects on mouse-PRESS; swallow the rest of the hold so the
        // release cannot fall through to the power button (options-window pattern).
        if (m_bSwallowClickHold)
        {
            if (!SEASON3B::IsRepeat(VK_LBUTTON))
                m_bSwallowClickHold = false;
            return false;
        }

        // The combo runs before everything: its open dropdown overflows the
        // window and must win the click.
        const bool wasOpen = m_StationCombo.IsOpen();
        if (m_StationCombo.UpdateMouseEvent())
        {
            m_bSwallowClickHold = true;
            Audio::Radio::SelectStation(m_StationCombo.GetSelectedIndex());
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
        if (m_StationCombo.IsMouseOverWidget())
        {
            return false;
        }
        if (wasOpen && !m_StationCombo.IsOpen() && IsPress(VK_LBUTTON))
        {
            m_bSwallowClickHold = true;
            return false;
        }

        if (IsPress(VK_LBUTTON)
            && CheckMouseIn(m_Pos.x + WINDOW_WIDTH - CLOSE_X_FROM_RIGHT, m_Pos.y + CLOSE_Y, CLOSE_W, CLOSE_H))
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_RADIO);
            MouseLButton = false;
            MouseLButtonPop = false;
            MouseLButtonPush = false;
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (HandleVolumeSlider())
        {
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (m_BtnPower.UpdateMouseEvent())
        {
            Audio::Radio::SetEnabled(!Audio::Radio::IsEnabled());
            RefreshPowerButton();
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (m_BtnExit.UpdateMouseEvent())
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_RADIO);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (IsPress(VK_LBUTTON))
        {
            if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
            {
                g_pNewUISystem->Hide(SEASON3B::INTERFACE_RADIO);
                PlayBuffer(SOUND_CLICK01);
            }

            MouseLButton = false;
            MouseLButtonPop = false;
            MouseLButtonPush = false;
            return false;
        }

        return false;
    }

    bool CNewUIRadioWindow::UpdateKeyEvent()
    {
        if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_RADIO))
        {
            return true;
        }

        if (IsPress(VK_ESCAPE))
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_RADIO);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        return true;
    }

    bool CNewUIRadioWindow::Update()
    {
        // The UI system is created before Winmain loads the station file, so
        // the first build sees zero stations; rebuild once they arrive (one
        // int compare per frame on hidden windows).
        const int stationCount = Audio::Radio::GetStationCount();
        if (stationCount != m_cachedStationCount)
        {
            m_cachedStationCount = stationCount;
            InitStationCombo();
            m_StationCombo.SetSelectedIndex(Audio::Radio::GetSelectedStation());
        }

        if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_RADIO))
        {
            // ClosingProcess parity with the Options window: a dropdown left
            // open while the window hides must not reappear floating over the
            // world the next time the open list outlives the window.
            m_StationCombo.Close();
            return true;
        }

        // The power button caption mirrors live state changes made elsewhere
        // (e.g. a future hotkey), same save-on-change philosophy.
        RefreshPowerButton();
        return true;
    }

    bool CNewUIRadioWindow::HandleVolumeSlider()
    {
        if (!CheckMouseIn(m_Pos.x + CONTENT_LEFT - VOLUME_HIT_PADDING,
                          m_Pos.y + VOLUME_TRACK_Y - VOLUME_HIT_PADDING,
                          VOLUME_TRACK_WIDTH + VOLUME_HIT_PADDING,
                          VOLUME_TRACK_HEIGHT + VOLUME_HIT_PADDING * 2))
        {
            return false;
        }

        const int oldValue = m_iVolumeLevel;

        if (MouseWheel > 0)
        {
            MouseWheel = 0;
            m_iVolumeLevel++;
        }
        else if (MouseWheel < 0)
        {
            MouseWheel = 0;
            m_iVolumeLevel--;
        }

        if (IsRepeat(VK_LBUTTON))
        {
            const int x = MouseX - (m_Pos.x + CONTENT_LEFT);
            if (x < 0)
            {
                m_iVolumeLevel = 0;
            }
            else
            {
                m_iVolumeLevel = static_cast<int>(
                    (static_cast<float>(Audio::Radio::kMaxVolume) * x) / static_cast<float>(VOLUME_TRACK_WIDTH) + 0.5f);
            }
        }

        m_iVolumeLevel = std::clamp(m_iVolumeLevel, Audio::Radio::kMinVolume, Audio::Radio::kMaxVolume);
        if (m_iVolumeLevel == oldValue)
        {
            return false;
        }

        Audio::Radio::SetVolume(m_iVolumeLevel);
        return true;
    }

    void CNewUIRadioWindow::RenderVolumeSlider()
    {
        // Native-primitive slider in the Options window's layout language: a
        // dark well, the gold fill up to the level, and a bright head mark.
        const float x = static_cast<float>(m_Pos.x + CONTENT_LEFT);
        const float y = static_cast<float>(m_Pos.y + VOLUME_TRACK_Y);

        EnableAlphaTest();
        glColor4f(0.f, 0.f, 0.f, 0.55f);
        RenderColor(x, y, float(VOLUME_TRACK_WIDTH), float(VOLUME_TRACK_HEIGHT));

        const float fill = float(VOLUME_TRACK_WIDTH) * m_iVolumeLevel / float(Audio::Radio::kMaxVolume);
        glColor4f(1.f, 220.f / 255.f, 120.f / 255.f, 0.95f);
        if (fill >= 1.f)
        {
            RenderColor(x + 1.f, y + 1.f, fill - 1.f, float(VOLUME_TRACK_HEIGHT) - 2.f);
        }
        glColor4f(1.f, 240.f / 255.f, 180.f / 255.f, 1.f);
        RenderColor(x + fill - 1.f, y, 2.f, float(VOLUME_TRACK_HEIGHT));
        EndRenderColor();
    }

    void CNewUIRadioWindow::RenderFooterButtons()
    {
        // Footer button contrast socket (house pattern: the dark footer strip
        // vanishes on the near-black tile, so buttons sit in a gold-rimmed well).
        {
            const POINT pos = m_BtnPower.GetPos();
            const POINT size = m_BtnPower.GetSize();
            const bool hot = m_BtnPower.GetBTState() != BUTTON_STATE_UP;
            const float x = static_cast<float>(pos.x);
            const float y = static_cast<float>(pos.y);
            const float w = static_cast<float>(size.x);
            const float h = static_cast<float>(size.y);

            EnableAlphaTest();
            glColor4f(0.f, 0.f, 0.f, hot ? 0.55f : 0.35f);
            RenderColor(x + 2.f, y + 2.f, w - 4.f, h - 4.f);
            glColor4f(1.f, 220.f / 255.f, 120.f / 255.f, hot ? 1.f : 0.8f);
            RenderColor(x, y, w, 1.f);
            RenderColor(x, y + h - 1.f, w, 1.f);
            RenderColor(x, y + 1.f, 1.f, h - 2.f);
            RenderColor(x + w - 1.f, y + 1.f, 1.f, h - 2.f);
            EndRenderColor();
        }
        m_BtnPower.Render();

        {
            const POINT pos = m_BtnExit.GetPos();
            const POINT size = m_BtnExit.GetSize();
            const bool hot = m_BtnExit.GetBTState() != BUTTON_STATE_UP;
            const float x = static_cast<float>(pos.x);
            const float y = static_cast<float>(pos.y);
            const float w = static_cast<float>(size.x);
            const float h = static_cast<float>(size.y);

            EnableAlphaTest();
            glColor4f(0.f, 0.f, 0.f, hot ? 0.55f : 0.35f);
            RenderColor(x + 2.f, y + 2.f, w - 4.f, h - 4.f);
            glColor4f(1.f, 220.f / 255.f, 120.f / 255.f, hot ? 1.f : 0.8f);
            RenderColor(x, y, w, 1.f);
            RenderColor(x, y + h - 1.f, w, 1.f);
            RenderColor(x, y + 1.f, 1.f, h - 2.f);
            RenderColor(x + w - 1.f, y + 1.f, 1.f, h - 2.f);
            EndRenderColor();
        }
        m_BtnExit.Render();
    }

    bool CNewUIRadioWindow::Render()
    {
        EnableAlphaTest();
        glColor4f(1.f, 1.f, 1.f, 1.f);

        RenderWindowFrame(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y),
            static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
        g_pRenderText->RenderText(m_Pos.x, m_Pos.y + TITLE_Y, kWindowTitle, WINDOW_WIDTH, 0, RT3_SORT_CENTER);

        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(kLabelRed, kLabelGreen, kLabelBlue, 255);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, m_Pos.y + STATION_LABEL_Y,
            L"Esta\u00e7\u00e3o:", CONTENT_WIDTH, LINE_HEIGHT, RT3_SORT_LEFT);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, m_Pos.y + VOLUME_LABEL_Y,
            L"Volume:", CONTENT_WIDTH, LINE_HEIGHT, RT3_SORT_LEFT);

        wchar_t volumeValue[32] = {};
        swprintf_s(volumeValue, L"%d", m_iVolumeLevel);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT + VOLUME_TRACK_WIDTH + 8, m_Pos.y + VOLUME_TRACK_Y + 1,
            volumeValue, 40, LINE_HEIGHT, RT3_SORT_LEFT);

        // Live status line under the controls. Same builder as the HUD
        // marquee — one source of truth for the state wording (tocando /
        // conectando / offline-reconectando).
        Audio::Radio::RadioEngine::Snapshot status;
        Audio::Radio::GetStatus(status);
        const wchar_t* station = status.stationName[0] != L'\0' ? status.stationName
            : Audio::Radio::GetStationName(Audio::Radio::GetSelectedStation());

        UI::Radio::RadioStatusKind kind = UI::Radio::RadioStatusKind::Off;
        switch (status.state)
        {
        case Audio::Radio::RadioEngine::State::Playing:
            kind = UI::Radio::RadioStatusKind::Playing;
            break;
        case Audio::Radio::RadioEngine::State::Connecting:
            kind = UI::Radio::RadioStatusKind::Connecting;
            break;
        case Audio::Radio::RadioEngine::State::Reconnecting:
            kind = UI::Radio::RadioStatusKind::Reconnecting;
            break;
        case Audio::Radio::RadioEngine::State::Off:
        default:
            kind = UI::Radio::RadioStatusKind::Off;
            break;
        }

        wchar_t nowLine[256] = {};
        UI::Radio::BuildRadioStatusText(kind, station, status.nowPlaying,
            nowLine, 256);
        g_pRenderText->SetTextColor(kStatusRed, kStatusGreen, kStatusBlue, 255);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, m_Pos.y + NOW_PLAYING_Y,
            nowLine, CONTENT_WIDTH, LINE_HEIGHT * NOW_PLAYING_LINES, RT3_SORT_LEFT_CLIP);

        RenderVolumeSlider();
        m_StationCombo.Render();
        RenderFooterButtons();
        DisableAlphaBlend();
        return true;
    }
}
