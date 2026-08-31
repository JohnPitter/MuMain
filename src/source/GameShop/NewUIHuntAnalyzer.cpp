#include "stdafx.h"
#include "I18N/All.h"
#include "UI/NewUI/NewUISystem.h"
#include "GameShop/NewUIHuntAnalyzer.h"
#include "GameShop/NewUIAutoBattler.h"
#include "GameShop/NewUIInGameShop.h"
#include "GameShop/InGameShopSystem.h"
#include "UI/NewUI/NewUICommon.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Character/CharacterManager.h"
#include "GameLogic/Progression/ExperienceBounds.h"
#include "Network/Server/WSclient.h"
#include "Dotnet/Connection.h"
#include "Audio/DSPlaySound.h"
#include "Render/Sprites/GlobalBitmap.h"
#include "Core/Utilities/Log/ErrorReport.h"

#include <algorithm>
#include <cmath>

using namespace SEASON3B;

namespace
{
    // Lifecycle diagnostics for the hunt panel, transition-level only.
    void HaLog(const char* szFormat, ...)
    {
        char szAnsi[256];
        va_list args;
        va_start(args, szFormat);
        _vsnprintf_s(szAnsi, sizeof(szAnsi), _TRUNCATE, szFormat != nullptr ? szFormat : "", args);
        va_end(args);

        wchar_t szText[256];
        const int nLen = MultiByteToWideChar(CP_ACP, 0, szAnsi, -1, szText, std::size(szText) - 1);
        if (nLen <= 0)
            return;
        szText[nLen] = L'\0';
        g_ErrorReport.Write(L"HUNTANALYZER: %s\r\n", szText);
    }

    // Compact right-side panel; fits 640x480 and never overlaps the centered
    // Auto Battle window (the panel hides itself while that window is open).
    constexpr int kPanelW = 170;
    constexpr int kPanelH = 322;
    constexpr int kPanelX = 640 - kPanelW - 4;
    constexpr int kPanelY = 112;
    constexpr int kHeaderH = 26;        // native newui_item_back01.tga header strip
    constexpr int kSideW = 21;          // native newui_item_back02 side cap width
    constexpr int kBottomH = 45;        // native newui_item_back03 bottom strip height
    constexpr int kContentX = 10;       // left padding inside the frame
    constexpr int kRowH = 22;           // one label/value row
    constexpr int kBarW = kPanelW - kContentX * 2;
    constexpr int kBarH = 18;           // native newui_Bar_switch01.jpg progress size
    constexpr int kBarInnerW = kBarW - 10;
    constexpr int kBtnW = kPanelW - kContentX * 2;
    constexpr int kBtnH = 29;           // native newui_btn_empty.tga state height
    constexpr DWORD kSampleIntervalMs = 1000;
    constexpr DWORD kSecondsPerHour = 3600;
    constexpr wchar_t kPtBrGroupSeparator = L'.';

    struct ExperienceSnapshot
    {
        uint64_t current;
        uint64_t next;
        uint64_t lower;
        WORD level;
        short masterLevel;
        bool master;
    };

    bool ReadExperienceSnapshot(ExperienceSnapshot& snapshot)
    {
        if (CharacterAttribute == nullptr)
            return false;

        snapshot.level = CharacterAttribute->Level;
        snapshot.master = gCharacterManager.IsMasterExperienceActive(
            CharacterAttribute->Class, CharacterAttribute->Level);
        snapshot.masterLevel = snapshot.master ? Master_Level_Data.nMLevel : 0;
        if (snapshot.master)
        {
            snapshot.current = static_cast<uint64_t>(std::max<__int64>(
                Master_Level_Data.lMasterLevel_Experince, 0));
            snapshot.next = static_cast<uint64_t>(std::max<__int64>(
                Master_Level_Data.lNext_MasterLevel_Experince, 0));
            snapshot.lower = static_cast<uint64_t>(std::max<int64_t>(
                GameLogic::Progression::GetMasterLowerBound(snapshot.masterLevel), 0));
            return snapshot.next > snapshot.lower
                && snapshot.current >= snapshot.lower
                && snapshot.current <= snapshot.next;
        }

        snapshot.current = CharacterAttribute->Experience;
        snapshot.next = CharacterAttribute->NextExperience;
        snapshot.lower = GameLogic::Progression::GetNormalLowerBound(snapshot.level);
        return snapshot.next > snapshot.lower
            && snapshot.current >= snapshot.lower
            && snapshot.current <= snapshot.next;
    }

    void FormatHms(wchar_t* buffer, int seconds)
    {
        if (seconds < 0)
            seconds = 0;
        const int hours = seconds / 3600;
        const int minutes = (seconds % 3600) / 60;
        const int rest = seconds % 60;
        mu_swprintf(buffer, L"%02d:%02d:%02d", hours, minutes, rest);
    }

    // Grouped decimal formatting for pt-BR values ("1.234.567").
    void FormatGrouped(wchar_t* buffer, int64_t value)
    {
        wchar_t raw[32];
        const bool negative = value < 0;
        const unsigned long long magnitude =
            negative ? static_cast<unsigned long long>(-(value + 1)) + 1u
                     : static_cast<unsigned long long>(value);
        mu_swprintf(raw, L"%I64u", magnitude);

        const int len = static_cast<int>(wcslen(raw));
        int out = 0;
        if (negative)
            buffer[out++] = L'-';
        for (int i = 0; i < len; ++i)
        {
            buffer[out++] = raw[i];
            const int remaining = len - i - 1;
            if (remaining > 0 && remaining % 3 == 0)
                buffer[out++] = kPtBrGroupSeparator;
        }
        buffer[out] = L'\0';
    }

    // Rate per hour; elapsed is pre-checked >= 1 so no division by zero, and
    // int64 keeps the math overflow-free for any realistic session
    // (values would need to exceed ~2.6e16 before the *3600 term overflows).
    int64_t PerHour(int64_t total, int elapsedSeconds)
    {
        if (elapsedSeconds <= 0)
            return 0;
        return total * static_cast<int64_t>(kSecondsPerHour) / elapsedSeconds;
    }
}

CNewUIHuntAnalyzer::CNewUIHuntAnalyzer()
{
    m_pNewUIMng = nullptr;
    m_Pos.x = kPanelX;
    m_Pos.y = kPanelY;
    m_bSessionActive = false;
    m_bUserHidden = false;
    m_bShownLast = false;
    m_bImagesLoaded = false;
    m_bBaselinesReady = false;
    m_bLastMasterExp = false;
    m_dwStartTick = 0;
    m_dwLastSample = 0;
    m_iElapsedSeconds = 0;
    m_ullLastExp = 0;
    m_ullLastNextExp = 0;
    m_ullTotalExp = 0;
    m_wLastLevel = 0;
    m_sLastMasterLevel = 0;
    m_dwLastZen = 0;
    m_llTotalProfit = 0;
    m_iBaselineHeroKey = -1;
}

CNewUIHuntAnalyzer::~CNewUIHuntAnalyzer()
{
    Release();
}

bool CNewUIHuntAnalyzer::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_HUNTANALYZER, this);
    m_Pos.x = x;
    m_Pos.y = y;
    LoadImages();
    SetBtnInfo();
    Show(false);
    Enable(true);
    return true;
}

void CNewUIHuntAnalyzer::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CNewUIHuntAnalyzer::LoadImages()
{
    if (m_bImagesLoaded)
        return;
    m_bImagesLoaded = true;

    // Same file + same shared id as the owning windows: reload is a no-op on
    // the texture cache and ownership stays with the original loader.
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_HA_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_HA_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_HA_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_HA_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_btn_empty.tga", IMAGE_HA_BTN, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_table01(L).tga", IMAGE_HA_TABLE_TOP_LEFT);
    LoadBitmap(L"Interface\\newui_item_table01(R).tga", IMAGE_HA_TABLE_TOP_RIGHT);
    LoadBitmap(L"Interface\\newui_item_table02(L).tga", IMAGE_HA_TABLE_BOTTOM_LEFT);
    LoadBitmap(L"Interface\\newui_item_table02(R).tga", IMAGE_HA_TABLE_BOTTOM_RIGHT);
    LoadBitmap(L"Interface\\newui_item_table03(Up).tga", IMAGE_HA_TABLE_TOP_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(Dw).tga", IMAGE_HA_TABLE_BOTTOM_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(L).tga", IMAGE_HA_TABLE_LEFT_PIXEL);
    LoadBitmap(L"Interface\\newui_item_table03(R).tga", IMAGE_HA_TABLE_RIGHT_PIXEL);
    LoadBitmap(L"Interface\\newui_Bar_switch01.jpg", IMAGE_HA_PROGRESS_BG, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_Bar_switch02.jpg", IMAGE_HA_PROGRESS_BAR, GL_LINEAR);
}

void CNewUIHuntAnalyzer::SetBtnInfo()
{
    m_VipButton.ChangeButtonImgState(true, IMAGE_HA_BTN, true);
    m_VipButton.ChangeButtonInfo(m_Pos.x + kContentX, m_Pos.y + kPanelH - kBottomH - kBtnH - 4, kBtnW, kBtnH);
    m_VipButton.ChangeText(I18N::Game::HuntAnalyzerVip);
    m_VipButton.MoveTextPos(0, -1);
}

void CNewUIHuntAnalyzer::NotifySession(bool bActive, const char* szReason)
{
    HaLog("session %s source=%s", bActive ? "start" : "stop",
        szReason != nullptr ? szReason : "unknown");
    if (bActive)
    {
        if (!m_bSessionActive)
            StartSession();
        return;
    }
    if (m_bSessionActive)
        StopSession();
}

void CNewUIHuntAnalyzer::StartSession()
{
    // Called only after the Auto Battle session is confirmed active (server
    // status reply, or the local start-confirmation fallback when the server
    // has no D5 handler). A new start always re-arms the panel even if the
    // user closed it during a previous session.
    m_bSessionActive = true;
    m_bUserHidden = false;
    m_bBaselinesReady = false;
    m_iBaselineHeroKey = -1;
    m_iElapsedSeconds = 0;
    m_dwStartTick = GetTickCount();
    m_dwLastSample = m_dwStartTick;
    m_ullTotalExp = 0;
    m_llTotalProfit = 0;
    SampleOnce();
    SetBtnInfo();
}

void CNewUIHuntAnalyzer::StopSession()
{
    HaLog("hide reason=session-stop");
    m_bSessionActive = false;
    m_bUserHidden = false;
    m_bBaselinesReady = false;
    m_iBaselineHeroKey = -1;
    Show(false);
}

void CNewUIHuntAnalyzer::SampleOnce()
{
    // One monotonic 1 Hz sample. A reconnect/character change rebases absolute
    // counters without turning the loaded wallet/EXP values into fake gains.
    if (CharacterMachine == nullptr || Hero == nullptr)
    {
        m_bBaselinesReady = false;
        m_iBaselineHeroKey = -1;
        return;
    }

    ExperienceSnapshot snapshot{};
    if (!ReadExperienceSnapshot(snapshot))
        return; // Ignore transient/incomplete normal/master channel updates.

    if (!m_bBaselinesReady || m_iBaselineHeroKey != Hero->Key)
    {
        m_ullLastExp = snapshot.current;
        m_ullLastNextExp = snapshot.next;
        m_wLastLevel = snapshot.level;
        m_sLastMasterLevel = snapshot.masterLevel;
        m_bLastMasterExp = snapshot.master;
        m_dwLastZen = CharacterMachine->Gold;
        m_iBaselineHeroKey = Hero->Key;
        m_bBaselinesReady = true;
        return;
    }

    const bool resetOrChannelBack =
        (snapshot.master && m_bLastMasterExp && snapshot.masterLevel < m_sLastMasterLevel)
        || (!snapshot.master && !m_bLastMasterExp && snapshot.level < m_wLastLevel)
        || (!snapshot.master && m_bLastMasterExp);
    if (!resetOrChannelBack)
    {
        if (snapshot.master == m_bLastMasterExp && snapshot.current >= m_ullLastExp)
        {
            m_ullTotalExp += snapshot.current - m_ullLastExp;
        }
        else
        {
            const uint64_t previousRemainder =
                m_ullLastNextExp > m_ullLastExp ? m_ullLastNextExp - m_ullLastExp : 0;
            const uint64_t currentProgress =
                snapshot.current > snapshot.lower ? snapshot.current - snapshot.lower : 0;
            m_ullTotalExp += previousRemainder + currentProgress;
        }
    }

    m_ullLastExp = snapshot.current;
    m_ullLastNextExp = snapshot.next;
    m_wLastLevel = snapshot.level;
    m_sLastMasterLevel = snapshot.masterLevel;
    m_bLastMasterExp = snapshot.master;

    // Gold is the real wallet value: pickups are positive, repairs/purchases
    // are negative, and reconnects were rebased above.
    const int64_t zenDelta =
        static_cast<int64_t>(CharacterMachine->Gold) - static_cast<int64_t>(m_dwLastZen);
    m_llTotalProfit += zenDelta;
    m_dwLastZen = CharacterMachine->Gold;
}

bool CNewUIHuntAnalyzer::Update()
{
    if (!m_bSessionActive)
        return true;

    // Anti-overlap driver: while the Auto Battle window, marketplace or cash
    // shop is open the panel steps aside; it returns automatically.
    const bool blocked =
        g_pNewUISystem->IsVisible(INTERFACE_AUTOBATTLER)
        || g_pNewUISystem->IsVisible(INTERFACE_INGAMESHOP)
        || g_pNewUISystem->IsVisible(INTERFACE_MARKETPLACE)
        || g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)
        || m_bUserHidden;

    if (m_pNewUIMng != nullptr)
        m_pNewUIMng->ShowInterface(INTERFACE_HUNTANALYZER, !blocked);

    const bool shown = !blocked;
    if (shown != m_bShownLast)
    {
        m_bShownLast = shown;
        HaLog("%s reason=%s", shown ? "show" : "hide",
            m_bUserHidden ? "user-hidden" : (blocked ? "overlap" : "session-active"));
    }

    // Visibility never pauses the session clock or samples. Inventory/shop
    // overlap only hides rendering; it must not inflate or lose EXP/h.
    const DWORD now = GetTickCount();
    const DWORD elapsedMs = now - m_dwLastSample;
    if (elapsedMs >= kSampleIntervalMs)
    {
        const DWORD elapsedSamples = elapsedMs / kSampleIntervalMs;
        m_dwLastSample += elapsedSamples * kSampleIntervalMs;
        m_iElapsedSeconds += static_cast<int>(elapsedSamples);
        SampleOnce();
    }
    return true;
}

bool CNewUIHuntAnalyzer::UpdateKeyEvent()
{
    if (!IsVisible())
        return true;
    return true;
}

bool CNewUIHuntAnalyzer::UpdateMouseEvent()
{
    if (!IsVisible())
        return true;

    if (BtnProcess())
        return false;

    if (CheckMouseIn(m_Pos.x, m_Pos.y, kPanelW, kPanelH))
        return false;
    return true;
}

bool CNewUIHuntAnalyzer::BtnProcess()
{
    // Native header close: gold medallion at the right end of the shared
    // frame header, same 13x12 box every shared-frame window uses.
    constexpr int kCloseFromRight = 21;
    constexpr int kCloseY = 7;
    constexpr int kCloseW = 13;
    constexpr int kCloseH = 12;
    if (IsPress(VK_LBUTTON)
        && CheckMouseIn(m_Pos.x + kPanelW - kCloseFromRight, m_Pos.y + kCloseY, kCloseW, kCloseH))
    {
        m_bUserHidden = true;
        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        return true;
    }

    if (m_VipButton.UpdateMouseEvent())
    {
        PlayBuffer(SOUND_CLICK01);
        // Public cash shop open flow, exactly like the X key handler
        // (CNewUIHotKey::UpdateKeyEvent): the shop window must exist and be
        // initialized first, then request the open only when it is not
        // already visible and no request is in flight. No purchase is ever
        // sent from here.
        if (g_pInGameShop != nullptr && g_pInGameShop->IsInGameShopOpen()
            && g_InGameShopSystem != nullptr
            && g_pNewUISystem->IsVisible(INTERFACE_INGAMESHOP) == false
            && g_InGameShopSystem->GetIsRequestShopOpenning() == false
            && SocketClient != nullptr)
        {
            SocketClient->ToGameServer()->SendCashShopOpenState(0);
            g_InGameShopSystem->SetIsRequestShopOpenning(true);
        }
        return true;
    }
    return false;
}

bool CNewUIHuntAnalyzer::Render()
{
    if (!IsVisible())
        return true;

    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);
    RenderFrame();
    RenderTexts();
    m_VipButton.Render();
    DisableAlphaBlend();
    return true;
}

void CNewUIHuntAnalyzer::RenderFrame()
{
    const float x = static_cast<float>(m_Pos.x);
    const float y = static_cast<float>(m_Pos.y);
    const float w = static_cast<float>(kPanelW);
    const float h = static_cast<float>(kPanelH);

    // Same native frame composition as CNewUIAutoBattler::RenderFrame:
    // header cap/middle/cap from newui_item_back01.tga (the right cap keeps
    // the baked close-X medallion at its native position), side strips and
    // the bottom ornament from newui_item_back02/03.
    constexpr float kHeaderCapW = 28.f;
    constexpr float kHeaderMidSrcX = 60.f;
    constexpr float kHeaderMidSrcW = 70.f;
    RenderImageStretch(IMAGE_HA_TOP, x, y, kHeaderCapW, static_cast<float>(kHeaderH),
        0.f, 0.f, kHeaderCapW, static_cast<float>(kHeaderH));
    RenderImageStretch(IMAGE_HA_TOP, x + kHeaderCapW, y, w - kHeaderCapW * 2.f, static_cast<float>(kHeaderH),
        kHeaderMidSrcX, 0.f, kHeaderMidSrcW, static_cast<float>(kHeaderH));
    RenderImageStretch(IMAGE_HA_TOP, x + w - kHeaderCapW, y, kHeaderCapW, static_cast<float>(kHeaderH),
        190.f - kHeaderCapW, 0.f, kHeaderCapW, static_cast<float>(kHeaderH));
    RenderImageStretch(IMAGE_HA_LEFT, x, y + static_cast<float>(kHeaderH), static_cast<float>(kSideW),
        h - static_cast<float>(kHeaderH) - static_cast<float>(kBottomH), 0.f, 0.f, 21.f, 320.f);
    RenderImageStretch(IMAGE_HA_RIGHT, x + w - static_cast<float>(kSideW), y + static_cast<float>(kHeaderH),
        static_cast<float>(kSideW),
        h - static_cast<float>(kHeaderH) - static_cast<float>(kBottomH), 0.f, 0.f, 21.f, 320.f);
    RenderImageStretch(IMAGE_HA_BOTTOM, x, y + h - static_cast<float>(kBottomH), w,
        static_cast<float>(kBottomH), 0.f, 0.f, 190.f, static_cast<float>(kBottomH));
}

void CNewUIHuntAnalyzer::RenderPanel(float x, float y, float width, float height, const wchar_t* title)
{
    // Native groupbox pattern (NewUIPetInfoWindow::RenderGroupBox): darkened
    // title strip over a lighter body, framed by the shared item-table pieces.
    EnableAlphaTest();
    glColor4f(0.f, 0.f, 0.f, 0.9f);
    RenderColor(x + 3.f, y + 2.f, width - 7.f, 14.f);
    glColor4f(0.f, 0.f, 0.f, 0.6f);
    RenderColor(x + 3.f, y + 16.f, width - 7.f, height - 19.f);
    EndRenderColor();

    RenderImage(IMAGE_HA_TABLE_TOP_LEFT, x, y, 14.f, 14.f);
    RenderImage(IMAGE_HA_TABLE_TOP_RIGHT, x + width - 14.f, y, 14.f, 14.f);
    RenderImage(IMAGE_HA_TABLE_BOTTOM_LEFT, x, y + height - 14.f, 14.f, 14.f);
    RenderImage(IMAGE_HA_TABLE_BOTTOM_RIGHT, x + width - 14.f, y + height - 14.f, 14.f, 14.f);
    RenderImage(IMAGE_HA_TABLE_TOP_PIXEL, x + 6.f, y, width - 12.f, 14.f);
    RenderImage(IMAGE_HA_TABLE_RIGHT_PIXEL, x + width - 14.f, y + 6.f, 14.f, height - 14.f);
    RenderImage(IMAGE_HA_TABLE_BOTTOM_PIXEL, x + 6.f, y + height - 14.f, width - 12.f, 14.f);
    RenderImage(IMAGE_HA_TABLE_LEFT_PIXEL, x, y + 6.f, 14.f, height - 14.f);

    if (title != nullptr && title[0] != L'\0')
    {
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(255, 238, 161, 255);
        g_pRenderText->RenderText(static_cast<int>(x) + 8, static_cast<int>(y) + 2, title,
            static_cast<int>(width) - 16, 0, RT3_SORT_LEFT);
    }
}

void CNewUIHuntAnalyzer::RenderTexts()
{
    wchar_t buffer[96];
    const int textX = m_Pos.x + kContentX;
    const int textW = kPanelW - kContentX * 2;
    int y = m_Pos.y + kHeaderH + 4;

    // Session group: elapsed time and live next-level progress.
    RenderPanel(static_cast<float>(m_Pos.x + 4), static_cast<float>(y),
        static_cast<float>(kPanelW - 8), static_cast<float>(kRowH * 2 + kBarH + 10), nullptr);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(textX + 4, y + 5, I18N::Game::HuntAnalyzerSession, textW - 8, 0, RT3_SORT_LEFT);

    FormatHms(buffer, m_iElapsedSeconds);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText(textX + 4, y + 18, buffer, textW - 8, 0, RT3_SORT_LEFT);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 238, 161, 255);
    g_pRenderText->RenderText(textX + 4, y + kRowH + 4, I18N::Game::HuntAnalyzerNextLevel, textW - 8, 0, RT3_SORT_LEFT);

    // Native progress bar (message box family): switch02 fill over switch01 bg.
    // The textures are owned by the message box manager; skip the bar rather
    // than drawing an unloaded texture if that manager ever released them.
    if (Bitmaps[IMAGE_HA_PROGRESS_BG].TextureNumber != 0
        && Bitmaps[IMAGE_HA_PROGRESS_BAR].TextureNumber != 0)
    {
        const float barX = static_cast<float>(textX + 4);
        const float barY = static_cast<float>(y + kRowH + 18);
        float ratio = 0.f;
        if (CharacterAttribute != nullptr)
        {
            const bool master = gCharacterManager.IsMasterExperienceActive(
                CharacterAttribute->Class, CharacterAttribute->Level);
            ratio = static_cast<float>(master
                ? GameLogic::Progression::MasterExpBarRatio(
                    Master_Level_Data.nMLevel,
                    Master_Level_Data.lMasterLevel_Experince,
                    Master_Level_Data.lNext_MasterLevel_Experince)
                : GameLogic::Progression::NormalExpBarRatio(
                    CharacterAttribute->Level,
                    CharacterAttribute->Experience,
                    CharacterAttribute->NextExperience));
        }
        RenderImage(IMAGE_HA_PROGRESS_BG, barX, barY, static_cast<float>(kBarW - 8), static_cast<float>(kBarH - 8));
        RenderImage(IMAGE_HA_PROGRESS_BAR, barX + 5.f, barY + 5.f,
            (kBarW - 18) * ratio, static_cast<float>(kBarH - 18));

        mu_swprintf(buffer, L"%.2f%%", ratio * 100.f);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(200, 255, 180, 255);
        g_pRenderText->RenderText(textX + 4, static_cast<int>(barY) + kBarH - 8, buffer, textW - 8, 0, RT3_SORT_CENTER);
    }

    y += kRowH * 2 + kBarH + 14;

    // Rates group: EXP and Zen totals with hourly estimates.
    RenderPanel(static_cast<float>(m_Pos.x + 4), static_cast<float>(y),
        static_cast<float>(kPanelW - 8), static_cast<float>(kRowH * 4 + 12), nullptr);

    struct Row
    {
        const wchar_t* label;
        const wchar_t* value;
        int row;
    };
    wchar_t expTotal[32], expRate[32], profitTotal[32], profitRate[32];
    FormatGrouped(expTotal, static_cast<int64_t>(m_ullTotalExp));
    FormatGrouped(expRate, static_cast<int64_t>(PerHour(static_cast<int64_t>(m_ullTotalExp), m_iElapsedSeconds)));
    FormatGrouped(profitTotal, m_llTotalProfit);
    FormatGrouped(profitRate, PerHour(m_llTotalProfit, m_iElapsedSeconds));

    const Row rows[] = {
        { I18N::Game::HuntAnalyzerExpTotal, expTotal, 0 },
        { I18N::Game::HuntAnalyzerExpPerHour, expRate, 1 },
        { I18N::Game::HuntAnalyzerProfitTotal, profitTotal, 2 },
        { I18N::Game::HuntAnalyzerProfitPerHour, profitRate, 3 },
    };
    for (const Row& row : rows)
    {
        const int rowY = y + 5 + row.row * kRowH;
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(255, 238, 161, 255);
        g_pRenderText->RenderText(textX + 4, rowY, row.label, textW - 8, 0, RT3_SORT_LEFT);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->RenderText(textX + 4, rowY + 10, row.value, textW - 8, 0, RT3_SORT_LEFT);
    }

    y += kRowH * 4 + 16;

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(190, 190, 190, 255);
    g_pRenderText->RenderText(textX, y, I18N::Game::HuntAnalyzerHint, textW, 30, RT3_SORT_LEFT);
}
