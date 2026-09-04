#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Events/NewUIEventScheduleWindow.h"

#include "Audio/DSPlaySound.h"
#include "Dotnet/Connection.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Events/EventScheduleTime.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

using namespace SEASON3B;

namespace
{
    // Title / header ink, matching the other LuxView pt-BR windows.
    constexpr BYTE kTitleRed = 255;
    constexpr BYTE kTitleGreen = 220;
    constexpr BYTE kTitleBlue = 120;

    // Background tiling grid and header 3-slice geometry, copied from the
    // AutoBattler window (CNewUIAutoBattler::TileWindowBack / RenderFrame), the
    // validated house pattern for wide windows built from the shared 190 px
    // inventory frame textures.
    constexpr float kBackSrcW = 190.f;  // msgbox_back tile grid width
    constexpr float kBackSrcH = 429.f;  // msgbox_back tile grid height
    constexpr float kHeaderCapW = 28.f;     // header end cap (texture texels)
    constexpr float kHeaderMidSrcX = 60.f;  // stretchable middle source rect
    constexpr float kHeaderMidSrcW = 70.f;

    // Baked close "X" in the header right cap (texels 169..182 of back01,
    // landing at x + w - 21 once the right cap is placed at w - 28).
    constexpr int CLOSE_X_FROM_RIGHT = 21;
    constexpr int CLOSE_Y = 7;
    constexpr int CLOSE_W = 13;
    constexpr int CLOSE_H = 12;

    // Local wall-clock seconds since midnight — the ONE time source the
    // countdown and the opening-hour column share.
    DWORD LocalSecondsOfDay()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        return static_cast<DWORD>(st.wHour) * 3600ul
            + static_cast<DWORD>(st.wMinute) * 60ul
            + static_cast<DWORD>(st.wSecond);
    }

    struct StateColor
    {
        BYTE r;
        BYTE g;
        BYTE b;
    };

    // Open = gold, running = green, upcoming = steel.
    constexpr StateColor kStateColors[3] =
    {
        { 150, 180, 215 },
        { 255, 200, 80 },
        { 120, 230, 140 },
    };
}

CNewUIEventScheduleWindow::CNewUIEventScheduleWindow()
    : m_pNewUIMng(NULL)
    , m_iEntryCount(0)
    , m_scrollOffset(0)
    , m_bReceived(false)
    , m_dwLastRequestTick(0)
    , m_dwAnchorWallSec(0)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
    memset(m_Entries, 0, sizeof(m_Entries));
}

CNewUIEventScheduleWindow::~CNewUIEventScheduleWindow()
{
    Release();
}

bool CNewUIEventScheduleWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == NULL)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_EVENTSCHEDULE, this);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);
    return true;
}

void CNewUIEventScheduleWindow::Release()
{
    UnloadImages();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CNewUIEventScheduleWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
}

void CNewUIEventScheduleWindow::InitButtons()
{
    wchar_t closeText[256] = {};
    mu_swprintf(closeText, I18N::Game::CloseS, L"O");
    m_BtnExit.ChangeButtonImgState(true, IMAGE_EVENTS_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);
}

float CNewUIEventScheduleWindow::GetLayerDepth()
{
    return 4.6f;
}

float CNewUIEventScheduleWindow::GetKeyEventOrder()
{
    return 10.f;
}

void CNewUIEventScheduleWindow::OpenningProcess()
{
    m_scrollOffset = 0;
    RequestSchedule();
}

void CNewUIEventScheduleWindow::ClosingProcess()
{
}

void CNewUIEventScheduleWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_EVENTS_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_EVENTS_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_EVENTS_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_EVENTS_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_EVENTS_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_EVENTS_BTN_EXIT, GL_LINEAR);
}

void CNewUIEventScheduleWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_EVENTS_BACK);
    DeleteBitmap(IMAGE_EVENTS_TOP);
    DeleteBitmap(IMAGE_EVENTS_LEFT);
    DeleteBitmap(IMAGE_EVENTS_RIGHT);
    DeleteBitmap(IMAGE_EVENTS_BOTTOM);
    DeleteBitmap(IMAGE_EVENTS_BTN_EXIT);
}

void CNewUIEventScheduleWindow::RequestSchedule()
{
    if (SocketClient == nullptr)
    {
        return;
    }

    BYTE packet[4] = { 0xC1, 4, kGroup, kListSubCode };
    SocketClient->Send(packet, 4);
    m_dwLastRequestTick = GetTickCount();
}

void CNewUIEventScheduleWindow::ReceiveSchedule(const BYTE* buffer, int size)
{
    if (buffer == nullptr || size < kHeaderBytes)
    {
        return;
    }

    int count = buffer[5];
    if (count > kMaxEntries)
    {
        count = kMaxEntries;
    }

    const int needed = kHeaderBytes + (count * kEntryBytes);
    if (size < needed)
    {
        // Trust the actual payload, not the announced count.
        count = (size - kHeaderBytes) / kEntryBytes;
        if (count < 0)
        {
            count = 0;
        }
    }

    const DWORD wallNow = LocalSecondsOfDay();

    Entry merged[kMaxEntries] = {};
    for (int i = 0; i < count; ++i)
    {
        const BYTE* src = buffer + kHeaderBytes + (i * kEntryBytes);
        char name[kNameBytes + 1] = { 0 };
        memcpy(name, src, kNameBytes);
        name[kNameBytes] = '\0';
        MultiByteToWideChar(CP_UTF8, 0, name, -1, merged[i].Name, kNameBytes + 1);
        merged[i].Name[kNameBytes] = L'\0';

        merged[i].State = src[24];
        memcpy(&merged[i].SecondsToStart, src + 25, 4);
        memcpy(&merged[i].SecondsToEnd, src + 29, 4);
        memcpy(&merged[i].MinLevel, src + 33, 2);
        memcpy(&merged[i].MaxLevel, src + 35, 2);

        // Anti-jitter merge: the fresh server value replaces the displayed
        // decay unless it only bounces the number upward (server-side ceil and
        // plugin clock offsets are sub-minute; a minute-scale jump is a real
        // schedule change). Keeps the rendered countdown monotonic across the
        // 15 s refreshes so it never flickers ±1 minute.
        for (int old = 0; old < m_iEntryCount; ++old)
        {
            if (wcscmp(m_Entries[old].Name, merged[i].Name) != 0)
            {
                continue;
            }

            const std::uint32_t displayedStart =
                event_schedule_time::remaining_seconds(m_Entries[old].SecondsToStart, wallNow, m_dwAnchorWallSec);
            merged[i].SecondsToStart =
                event_schedule_time::merge_refresh_seconds(displayedStart, merged[i].SecondsToStart);

            const std::uint32_t displayedEnd =
                event_schedule_time::remaining_seconds(m_Entries[old].SecondsToEnd, wallNow, m_dwAnchorWallSec);
            merged[i].SecondsToEnd =
                event_schedule_time::merge_refresh_seconds(displayedEnd, merged[i].SecondsToEnd);
            break;
        }
    }

    memcpy(m_Entries, merged, sizeof(m_Entries));
    m_iEntryCount = count;
    m_bReceived = true;
    m_dwAnchorWallSec = wallNow;

    const int hidden = m_iEntryCount - VISIBLE_ROWS;
    if (m_scrollOffset > hidden)
    {
        m_scrollOffset = hidden > 0 ? hidden : 0;
    }
}

const wchar_t* CNewUIEventScheduleWindow::StateText(BYTE state)
{
    switch (state)
    {
    case EVENT_STATE_OPEN:
        return L"Aberto";
    case EVENT_STATE_RUNNING:
        return L"Em andamento";
    default:
        return L"Em breve";
    }
}

bool CNewUIEventScheduleWindow::UpdateMouseEvent()
{
    // Baked close "X" in the header's right cap. The shared
    // CNewUISystem::HandleFrameCornerClose assumes the 190 px inventory frame
    // width, so this 320 px window checks its own corner box, same behavior:
    // close and swallow the click so it does not fall through to world movement.
    if (IsPress(VK_LBUTTON)
        && CheckMouseIn(m_Pos.x + WINDOW_WIDTH - CLOSE_X_FROM_RIGHT, m_Pos.y + CLOSE_Y, CLOSE_W, CLOSE_H))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENTSCHEDULE);
        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENTSCHEDULE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    const int hidden = m_iEntryCount - VISIBLE_ROWS;
    if (MouseWheel != 0 && hidden > 0)
    {
        m_scrollOffset -= MouseWheel;
        if (m_scrollOffset < 0)
        {
            m_scrollOffset = 0;
        }
        else if (m_scrollOffset > hidden)
        {
            m_scrollOffset = hidden;
        }

        MouseWheel = 0;
        return false;
    }

    if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        return true;
    }

    return false;
}

bool CNewUIEventScheduleWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_EVENTSCHEDULE))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENTSCHEDULE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUIEventScheduleWindow::Update()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_EVENTSCHEDULE))
    {
        return true;
    }

    const DWORD now = GetTickCount();
    if (now - m_dwLastRequestTick >= kRefreshIntervalMs)
    {
        RequestSchedule();
    }

    return true;
}

// The shared inventory frame is drawn by every window as raw texels
// (RenderImage samples 1:1 — width/height are the sampled extent, not a scale),
// so a wide window must tile the background and stretch each frame piece
// explicitly. This mirrors CNewUIAutoBattler::TileWindowBack / RenderFrame:
// msgbox_back tiled on a 190x429 grid, the 190x64 header as cap/middle/cap
// (the right cap carries the baked close "X"), 21 px side strips stretched
// from the 21x320 texture, and the 190x45 bottom strip stretched to full width.
// That is what keeps the stone background flush with the ornamental frame at
// the window's real width — no hole, no loose frame line.
void CNewUIEventScheduleWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto w = static_cast<float>(WINDOW_WIDTH);
    const auto h = static_cast<float>(WINDOW_HEIGHT);

    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    for (float oy = 0.f; oy < h; oy += kBackSrcH)
    {
        const float th = (oy + kBackSrcH > h) ? (h - oy) : kBackSrcH;
        for (float ox = 0.f; ox < w; ox += kBackSrcW)
        {
            const float tw = (ox + kBackSrcW > w) ? (w - ox) : kBackSrcW;
            RenderImage(IMAGE_EVENTS_BACK, x + ox, y + oy, tw, th);
        }
    }

    RenderImageStretch(IMAGE_EVENTS_TOP, x, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
        0.f, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_EVENTS_TOP, x + kHeaderCapW, y, w - kHeaderCapW * 2.f, float(FRAME_TOP_HEIGHT),
        kHeaderMidSrcX, 0.f, kHeaderMidSrcW, float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_EVENTS_TOP, x + w - kHeaderCapW, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
        190.f - kHeaderCapW, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));

    const float middleHeight = h - float(FRAME_TOP_HEIGHT) - float(FRAME_BOTTOM_HEIGHT);
    RenderImageStretch(IMAGE_EVENTS_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_EVENTS_RIGHT, x + w - float(FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));

    RenderImageStretch(IMAGE_EVENTS_BOTTOM, x, y + h - float(FRAME_BOTTOM_HEIGHT), w, float(FRAME_BOTTOM_HEIGHT),
        0.f, 0.f, 190.f, float(FRAME_BOTTOM_HEIGHT));
}

bool CNewUIEventScheduleWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    RenderBaseWindow();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + TITLE_Y, L"Eventos", WINDOW_WIDTH, 0, RT3_SORT_CENTER);

    const int contentX = m_Pos.x + CONTENT_LEFT;
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(190, 190, 190, 255);
    g_pRenderText->RenderText(contentX + COL_NAME_X, m_Pos.y + HEADER_TOP, L"Evento", COL_NAME_W, ROW_HEIGHT, RT3_SORT_LEFT);
    g_pRenderText->RenderText(contentX + COL_STATE_X, m_Pos.y + HEADER_TOP, L"Estado", COL_STATE_W, ROW_HEIGHT, RT3_SORT_LEFT);
    // RT3_SORT_LEFT_CLIP, not RT3_SORT_RIGHT: the right-align box math was the only
    // difference from the two columns that render correctly and let the "Tempo"
    // texts escape the panel. LEFT_CLIP anchors the draw at the column's left edge
    // inside this window's rect and hard-clips at the window's inner right edge
    // (CONTENT_LEFT + COL_TIME_X + COL_TIME_W = WINDOW_WIDTH - CONTENT_LEFT), so the
    // column can never spill outside the frame at any resolution.
    g_pRenderText->RenderText(contentX + COL_TIME_X, m_Pos.y + HEADER_TOP, L"Tempo", COL_TIME_W, ROW_HEIGHT, RT3_SORT_LEFT_CLIP);

    if (m_iEntryCount == 0)
    {
        g_pRenderText->SetTextColor(200, 200, 200, 255);
        g_pRenderText->RenderText(
            contentX,
            m_Pos.y + CONTENT_TOP,
            m_bReceived ? L"Nenhum evento neste servidor." : L"Carregando...",
            CONTENT_WIDTH,
            ROW_HEIGHT,
            RT3_SORT_CENTER);

        m_BtnExit.Render();
        DisableAlphaBlend();
        return true;
    }

    // One wall-clock reading per frame: every row renders the same instant.
    const std::uint32_t wallFrameSec = LocalSecondsOfDay();
    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const int index = m_scrollOffset + row;
        if (index >= m_iEntryCount)
        {
            break;
        }

        const Entry& entry = m_Entries[index];
        const int y = m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT;
        const StateColor& color = kStateColors[entry.State < 3 ? entry.State : 0];

        // Both columns derive from the single frame clock reading and the
        // receive anchors (EventScheduleTime.h): the displayed second only
        // changes when the wall second changes, so the opening hour and the
        // countdown stay in agreement and cannot flicker ±1 minute.
        const std::uint32_t remainingStart =
            event_schedule_time::remaining_seconds(entry.SecondsToStart, wallFrameSec, m_dwAnchorWallSec);
        const std::uint32_t remainingEnd =
            event_schedule_time::remaining_seconds(entry.SecondsToEnd, wallFrameSec, m_dwAnchorWallSec);

        // Upcoming events show both the wall-clock time they open (e.g. 21:30) and
        // the countdown to it (e.g. 1h05m); open/running events show the time left.
        wchar_t timeText[48] = {};
        if (entry.State == EVENT_STATE_UPCOMING)
        {
            if (remainingStart > 0)
            {
                wchar_t clockText[16] = {};
                wchar_t countText[24] = {};
                event_schedule_time::format_clock(
                    event_schedule_time::open_wall_clock(remainingStart, wallFrameSec), clockText, 16);
                event_schedule_time::format_duration(remainingStart, countText, 24);
                mu_swprintf_s(timeText, 48, L"%ls (%ls)", clockText, countText);
            }
            else
            {
                mu_swprintf_s(timeText, 48, L"--");
            }
        }
        else
        {
            event_schedule_time::format_duration(remainingEnd, timeText, 48);
        }

        wchar_t nameText[64] = {};
        if (entry.MinLevel > 0)
        {
            mu_swprintf(nameText, L"%ls (%u+)", entry.Name, static_cast<unsigned int>(entry.MinLevel));
        }
        else
        {
            mu_swprintf(nameText, L"%ls", entry.Name);
        }

        g_pRenderText->SetTextColor(color.r, color.g, color.b, 255);
        g_pRenderText->RenderText(contentX + COL_NAME_X, y, nameText, COL_NAME_W, ROW_HEIGHT, RT3_SORT_LEFT);
        g_pRenderText->RenderText(contentX + COL_STATE_X, y, StateText(entry.State), COL_STATE_W, ROW_HEIGHT, RT3_SORT_LEFT);
        g_pRenderText->RenderText(contentX + COL_TIME_X, y, timeText, COL_TIME_W, ROW_HEIGHT, RT3_SORT_LEFT_CLIP);
    }

    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}

void ReceiveEventSchedule(const BYTE* buffer, int size)
{
    if (auto* window = g_pEventScheduleWindow)
    {
        window->ReceiveSchedule(buffer, size);
    }
}
