#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Events/NewUIEventScheduleWindow.h"

#include "Audio/DSPlaySound.h"
#include "Dotnet/Connection.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

using namespace SEASON3B;

namespace
{
    // Title / header ink, matching the other LuxView pt-BR windows.
    constexpr BYTE kTitleRed = 255;
    constexpr BYTE kTitleGreen = 220;
    constexpr BYTE kTitleBlue = 120;

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
    , m_dwLastCountdownTick(0)
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
    m_dwLastCountdownTick = GetTickCount();
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

    memset(m_Entries, 0, sizeof(m_Entries));
    for (int i = 0; i < count; ++i)
    {
        const BYTE* src = buffer + kHeaderBytes + (i * kEntryBytes);
        char name[kNameBytes + 1] = { 0 };
        memcpy(name, src, kNameBytes);
        name[kNameBytes] = '\0';
        MultiByteToWideChar(CP_UTF8, 0, name, -1, m_Entries[i].Name, kNameBytes + 1);
        m_Entries[i].Name[kNameBytes] = L'\0';

        m_Entries[i].State = src[24];
        memcpy(&m_Entries[i].SecondsToStart, src + 25, 4);
        memcpy(&m_Entries[i].SecondsToEnd, src + 29, 4);
        memcpy(&m_Entries[i].MinLevel, src + 33, 2);
        memcpy(&m_Entries[i].MaxLevel, src + 35, 2);
    }

    m_iEntryCount = count;
    m_bReceived = true;
    m_dwLastCountdownTick = GetTickCount();

    const int hidden = m_iEntryCount - VISIBLE_ROWS;
    if (m_scrollOffset > hidden)
    {
        m_scrollOffset = hidden > 0 ? hidden : 0;
    }
}

// Decrements the received counters once per elapsed second, so the display keeps
// moving between the 15 s server refreshes.
void CNewUIEventScheduleWindow::TickCountdown()
{
    const DWORD now = GetTickCount();
    if (now < m_dwLastCountdownTick)
    {
        m_dwLastCountdownTick = now;
        return;
    }

    const DWORD elapsed = (now - m_dwLastCountdownTick) / 1000;
    if (elapsed == 0)
    {
        return;
    }

    m_dwLastCountdownTick += elapsed * 1000;
    for (int i = 0; i < m_iEntryCount; ++i)
    {
        m_Entries[i].SecondsToStart = (m_Entries[i].SecondsToStart > elapsed) ? (m_Entries[i].SecondsToStart - elapsed) : 0;
        m_Entries[i].SecondsToEnd = (m_Entries[i].SecondsToEnd > elapsed) ? (m_Entries[i].SecondsToEnd - elapsed) : 0;
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

void CNewUIEventScheduleWindow::FormatDuration(DWORD seconds, wchar_t* target, size_t targetCount)
{
    if (target == nullptr || targetCount == 0)
    {
        return;
    }

    if (seconds == 0)
    {
        mu_swprintf_s(target, targetCount, L"--");
        return;
    }

    if (seconds >= 3600)
    {
        mu_swprintf_s(target, targetCount, L"%luh%02lum", seconds / 3600, (seconds % 3600) / 60);
        return;
    }

    if (seconds >= 60)
    {
        mu_swprintf_s(target, targetCount, L"%lu:%02lu", seconds / 60, seconds % 60);
        return;
    }

    mu_swprintf_s(target, targetCount, L"%lus", seconds);
}

void CNewUIEventScheduleWindow::FormatOpenClock(DWORD secondsFromNow, wchar_t* target, size_t targetCount)
{
    if (target == nullptr || targetCount == 0)
    {
        return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);

    // Current local time-of-day plus the countdown, wrapped to a 24h clock.
    unsigned long total = static_cast<unsigned long>(st.wHour) * 3600ul
        + static_cast<unsigned long>(st.wMinute) * 60ul
        + static_cast<unsigned long>(st.wSecond)
        + secondsFromNow;
    total %= 86400ul;

    mu_swprintf_s(target, targetCount, L"%02lu:%02lu", total / 3600ul, (total % 3600ul) / 60ul);
}

bool CNewUIEventScheduleWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, SEASON3B::INTERFACE_EVENTSCHEDULE))
    {
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

    TickCountdown();

    const DWORD now = GetTickCount();
    if (now - m_dwLastRequestTick >= kRefreshIntervalMs)
    {
        RequestSchedule();
    }

    return true;
}

void CNewUIEventScheduleWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WINDOW_HEIGHT - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);

    RenderImage(IMAGE_EVENTS_BACK, x, y, float(WINDOW_WIDTH), float(WINDOW_HEIGHT));
    RenderImage(IMAGE_EVENTS_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_EVENTS_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_EVENTS_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImage(IMAGE_EVENTS_BOTTOM, x, y + float(WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH), float(FRAME_BOTTOM_HEIGHT));
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
    g_pRenderText->RenderText(contentX + COL_TIME_X, m_Pos.y + HEADER_TOP, L"Tempo", COL_TIME_W, ROW_HEIGHT, RT3_SORT_RIGHT);

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

        // Upcoming events show both the wall-clock time they open (e.g. 21:30) and
        // the countdown to it (e.g. 1h05m); open/running events show the time left.
        wchar_t timeText[48] = {};
        if (entry.State == EVENT_STATE_UPCOMING)
        {
            if (entry.SecondsToStart > 0)
            {
                wchar_t clockText[16] = {};
                wchar_t countText[24] = {};
                FormatOpenClock(entry.SecondsToStart, clockText, 16);
                FormatDuration(entry.SecondsToStart, countText, 24);
                mu_swprintf_s(timeText, 48, L"%ls (%ls)", clockText, countText);
            }
            else
            {
                mu_swprintf_s(timeText, 48, L"--");
            }
        }
        else
        {
            FormatDuration(entry.SecondsToEnd, timeText, 48);
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
        g_pRenderText->RenderText(contentX + COL_TIME_X, y, timeText, COL_TIME_W, ROW_HEIGHT, RT3_SORT_RIGHT);
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
