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

    // Local wall clock in ONE reading: seconds since midnight (the ONE time
    // source the countdown and the opening columns share) and the epoch day
    // feeding the pure opening-date math for events more than a day away.
    void LocalWallClock(DWORD* secOfDay, std::int64_t* epochDays)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        *secOfDay = static_cast<DWORD>(st.wHour) * 3600ul
            + static_cast<DWORD>(st.wMinute) * 60ul
            + static_cast<DWORD>(st.wSecond);
        *epochDays = event_schedule_time::days_from_civil(st.wYear, st.wMonth, st.wDay);
    }

    DWORD LocalSecondsOfDay()
    {
        DWORD secOfDay;
        std::int64_t epochDays;
        LocalWallClock(&secOfDay, &epochDays);
        return secOfDay;
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

    // ---- Help popup -------------------------------------------------------
    //
    // One flat table of pre-wrapped lines: one NAME line per event (exactly as
    // rendered by the schedule list, which appends "(N+)" from the packet's
    // MinLevel), a few body lines, and "Onde:" lines pointing at the validated
    // entrance (OpenMU EventScheduleService.cs DisplayNames + the S6 event
    // ticket items / NPC spawns: Messenger of Archangel in Devias for Blood
    // Castle, Charon in Noria for Devil Square, Guardsman in Valley of Loren
    // for Chaos Castle, Gateway Machine for Kanturu). Unaccented on purpose —
    // every other pt-BR string in this window is unaccented too.
    enum eHelpLineKind
    {
        HL_NAME = 0, // gold, bold — the event name as the list renders it
        HL_BODY = 1, // light grey — what the event is
        HL_WHERE = 2, // steel blue — where / how to join
        HL_GAP = 3,  // one blank row between events
    };

    struct HelpLine
    {
        eHelpLineKind Kind;
        const wchar_t* Text;
    };

    constexpr HelpLine kHelpLines[] =
    {
        { HL_NAME, L"Mercadores" },
        { HL_BODY, L"Mercadores especiais aparecem nas" },
        { HL_BODY, L"cidades por tempo limitado. Compre" },
        { HL_BODY, L"itens raros com zen." },
        { HL_WHERE, L"Onde: aparecem nas cidades." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Minibosses" },
        { HL_BODY, L"Minibosses aparecem em varios mapas." },
        { HL_BODY, L"Derrote-os para receber joias e itens." },
        { HL_WHERE, L"Onde: em varios mapas do servidor." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Castle Siege" },
        { HL_BODY, L"Guerra pelo castelo no Valley of Loren." },
        { HL_BODY, L"A guild dona defende; atacantes tentam" },
        { HL_BODY, L"tomar o trono." },
        { HL_WHERE, L"Onde: Valley of Loren. Registre sua" },
        { HL_WHERE, L"guild com o NPC responsavel." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Blood Castle (15+)" },
        { HL_BODY, L"Atravesse a ponte, quebre o portao," },
        { HL_BODY, L"destrua o artefato e entregue ao Anjo." },
        { HL_WHERE, L"Onde: NPC Messenger of Archangel em" },
        { HL_WHERE, L"Devias, com o Invisibility Cloak." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Devil Square (15+)" },
        { HL_BODY, L"Sobreviva as ondas de monstros." },
        { HL_WHERE, L"Onde: NPC Charon em Noria, com o" },
        { HL_WHERE, L"Devil's Invitation." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Chaos Castle (15+)" },
        { HL_BODY, L"PvP no castelo em queda: sobreviva as" },
        { HL_BODY, L"ondas e empurre os rivais para fora." },
        { HL_WHERE, L"Onde: NPC Guardsman no Valley of" },
        { HL_WHERE, L"Loren, com o Armor of Guardsman." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Illusion Temple" },
        { HL_BODY, L"PvP em equipes no Templo da Ilusao." },
        { HL_WHERE, L"Onde: NPC Charon em Noria, com o" },
        { HL_WHERE, L"Scroll of Blood." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Dragoes Vermelhos" },
        { HL_BODY, L"Dragoes vermelhos invadem Lorencia." },
        { HL_BODY, L"Derrote-os para receber recompensas." },
        { HL_WHERE, L"Onde: invasao em Lorencia." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Invasao Dourada" },
        { HL_BODY, L"Monstros dourados invadem os mapas e" },
        { HL_BODY, L"dropam Box of Kundun (com item" },
        { HL_BODY, L"excelente dentro)." },
        { HL_WHERE, L"Onde: invasao em varios mapas." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Mago Branco" },
        { HL_BODY, L"O Mago Branco e seu exercito de orcs" },
        { HL_BODY, L"invadem Devias. Elimine os orcs e" },
        { HL_BODY, L"depois o mago." },
        { HL_WHERE, L"Onde: invasao em Devias." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Happy Hour" },
        { HL_BODY, L"EXP em dobro por 1 hora em todos os" },
        { HL_BODY, L"mapas." },
        { HL_WHERE, L"Onde: em todos os mapas." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Crywolf (350+)" },
        { HL_BODY, L"Defenda a Fortaleza de Crywolf da" },
        { HL_BODY, L"horda de Balgass. Recompensa conforme" },
        { HL_BODY, L"o desempenho da guarnicao." },
        { HL_WHERE, L"Onde: vale de Crywolf (warp), no" },
        { HL_WHERE, L"horario do evento." },
        { HL_GAP, nullptr },

        { HL_NAME, L"Kanturu" },
        { HL_BODY, L"Evento do Refinatorio: enfrente a" },
        { HL_BODY, L"Maya e a Nightmare com outros" },
        { HL_BODY, L"jogadores." },
        { HL_WHERE, L"Onde: NPC Gateway Machine em Kanturu." },
        { HL_GAP, nullptr },
    };

    constexpr int kHelpLineCount = sizeof(kHelpLines) / sizeof(kHelpLines[0]);
}

CNewUIEventScheduleWindow::CNewUIEventScheduleWindow()
    : m_pNewUIMng(NULL)
    , m_iEntryCount(0)
    , m_scrollOffset(0)
    , m_bReceived(false)
    , m_dwLastRequestTick(0)
    , m_dwAnchorWallSec(0)
    , m_HelpOpen(false)
    , m_HelpScroll(0)
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
    m_BtnHelp.ChangeButtonInfo(m_Pos.x + HELP_BUTTON_X, m_Pos.y + HELP_BUTTON_Y, HELP_BUTTON_WIDTH, HELP_BUTTON_HEIGHT);
}

void CNewUIEventScheduleWindow::InitButtons()
{
    wchar_t closeText[256] = {};
    mu_swprintf(closeText, I18N::Game::CloseS, L"O");
    m_BtnExit.ChangeButtonImgState(true, IMAGE_EVENTS_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);

    // "Ajuda" opens the help popup. Same shared empty-button texture the
    // other windows use for labeled buttons (owned by the message-box
    // manager — no extra LoadBitmap here).
    m_BtnHelp.ChangeButtonImgState(true, IMAGE_EVENTS_BTN_HELP, true);
    m_BtnHelp.SetFont(g_hFontBold);
    m_BtnHelp.ChangeText(L"Ajuda");
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
    CloseHelp();
    RequestSchedule();
}

void CNewUIEventScheduleWindow::ClosingProcess()
{
    CloseHelp();
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
    // While the help popup is open it owns the mouse: its baked "X" closes it,
    // the wheel scrolls it when the cursor is over it, any click outside
    // closes it, and every event is swallowed so nothing falls through to the
    // list or the world beneath.
    if (m_HelpOpen)
    {
        int hx;
        int hy;
        GetHelpRect(&hx, &hy);

        if (IsPress(VK_LBUTTON)
            && CheckMouseIn(hx + HELP_WIDTH - CLOSE_X_FROM_RIGHT, hy + CLOSE_Y, CLOSE_W, CLOSE_H))
        {
            CloseHelp();
            MouseLButton = false;
            MouseLButtonPop = false;
            MouseLButtonPush = false;
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        if (MouseWheel != 0)
        {
            if (CheckMouseIn(hx, hy, HELP_WIDTH, HELP_HEIGHT))
            {
                const int maxScroll = kHelpLineCount > HELP_VISIBLE_LINES
                    ? kHelpLineCount - HELP_VISIBLE_LINES
                    : 0;
                m_HelpScroll -= MouseWheel;
                if (m_HelpScroll < 0)
                {
                    m_HelpScroll = 0;
                }
                else if (m_HelpScroll > maxScroll)
                {
                    m_HelpScroll = maxScroll;
                }
            }

            MouseWheel = 0;
            return false;
        }

        if (IsPress(VK_LBUTTON))
        {
            if (CheckMouseIn(hx, hy, HELP_WIDTH, HELP_HEIGHT))
            {
                // A click inside the popup body (outside the "X") is absorbed,
                // it must not close the help nor fall through.
            }
            else
            {
                // Any click outside the popup closes it.
                CloseHelp();
                PlayBuffer(SOUND_CLICK01);
            }

            MouseLButton = false;
            MouseLButtonPop = false;
            MouseLButtonPush = false;
            return false;
        }

        return false;
    }

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

    if (m_BtnHelp.UpdateMouseEvent())
    {
        m_HelpOpen = true;
        m_HelpScroll = 0;
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
        // ESC peels one layer: close the help popup first, the window only
        // when it is not showing.
        if (m_HelpOpen)
        {
            CloseHelp();
        }
        else
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENTSCHEDULE);
        }

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
    RenderWindowFrame(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y),
        static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
}

// Shared by the schedule window and the help popup: same textures, same
// 3-slice geometry, so the popup keeps the window's exact visual language
// (and inherits the baked close "X" in the header's right cap).
void CNewUIEventScheduleWindow::RenderWindowFrame(float x, float y, float w, float h)
{
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
        m_BtnHelp.Render();
        if (m_HelpOpen)
        {
            RenderHelpWindow();
        }
        DisableAlphaBlend();
        return true;
    }

    // One wall-clock reading per frame: every row renders the same instant.
    DWORD wallFrameSec = 0;
    std::int64_t nowEpochDays = 0;
    LocalWallClock(&wallFrameSec, &nowEpochDays);
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

        // Upcoming events show the wall-clock time they open plus either the
        // countdown (under a day: "19:59 (3h48m)") or, above 24 hours where an
        // hour count stops meaning anything, the opening weekday and date
        // ("15:18 (seg 08/09)"). Open/running events show the time left.
        wchar_t timeText[48] = {};
        if (entry.State == EVENT_STATE_UPCOMING)
        {
            if (remainingStart > 0)
            {
                wchar_t clockText[16] = {};
                event_schedule_time::format_clock(
                    event_schedule_time::open_wall_clock(remainingStart, wallFrameSec), clockText, 16);
                if (event_schedule_time::should_show_date(remainingStart))
                {
                    const event_schedule_time::OpeningDate date =
                        event_schedule_time::opening_date(nowEpochDays, remainingStart, wallFrameSec);
                    wchar_t weekdayText[8] = {};
                    wchar_t dateText[8] = {};
                    event_schedule_time::format_weekday(date.weekdayIndex, weekdayText, 8);
                    event_schedule_time::format_date(date.day, date.month, dateText, 8);
                    mu_swprintf_s(timeText, 48, L"%ls (%ls %ls)", clockText, weekdayText, dateText);
                }
                else
                {
                    wchar_t countText[24] = {};
                    event_schedule_time::format_duration(remainingStart, countText, 24);
                    mu_swprintf_s(timeText, 48, L"%ls (%ls)", clockText, countText);
                }
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
    m_BtnHelp.Render();
    if (m_HelpOpen)
    {
        RenderHelpWindow();
    }
    DisableAlphaBlend();
    return true;
}

void CNewUIEventScheduleWindow::GetHelpRect(int* x, int* y) const
{
    // Centered over this window so it reads as the schedule's child dialog.
    *x = m_Pos.x + (WINDOW_WIDTH - HELP_WIDTH) / 2;
    *y = m_Pos.y + (WINDOW_HEIGHT - HELP_HEIGHT) / 2;
}

void CNewUIEventScheduleWindow::CloseHelp()
{
    m_HelpOpen = false;
    m_HelpScroll = 0;
}

// The "telinha": same frame pieces and fonts as the schedule window, with one
// block per event (gold name, grey explanation, steel-blue "Onde:" line),
// pre-wrapped into short lines and scrolled by whole lines with the mouse
// wheel. RT3_SORT_LEFT_CLIP keeps any line from spilling past the frame.
void CNewUIEventScheduleWindow::RenderHelpWindow()
{
    int hx;
    int hy;
    GetHelpRect(&hx, &hy);

    RenderWindowFrame(static_cast<float>(hx), static_cast<float>(hy),
        static_cast<float>(HELP_WIDTH), static_cast<float>(HELP_HEIGHT));

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
    g_pRenderText->RenderText(hx, hy + TITLE_Y, L"Ajuda - Eventos", HELP_WIDTH, 0, RT3_SORT_CENTER);

    const int maxScroll = kHelpLineCount > HELP_VISIBLE_LINES
        ? kHelpLineCount - HELP_VISIBLE_LINES
        : 0;
    if (m_HelpScroll > maxScroll)
    {
        m_HelpScroll = maxScroll;
    }
    else if (m_HelpScroll < 0)
    {
        m_HelpScroll = 0;
    }

    const int contentWidth = HELP_WIDTH - 2 * HELP_CONTENT_LEFT;
    for (int i = 0; i < kHelpLineCount; ++i)
    {
        const int row = i - m_HelpScroll;
        if (row < 0 || row >= HELP_VISIBLE_LINES)
        {
            continue;
        }

        const HelpLine& line = kHelpLines[i];
        if (line.Kind == HL_GAP)
        {
            continue;
        }

        const int y = hy + HELP_CONTENT_TOP + row * HELP_LINE_HEIGHT;
        switch (line.Kind)
        {
        case HL_NAME:
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
            break;
        case HL_WHERE:
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(kStateColors[0].r, kStateColors[0].g, kStateColors[0].b, 255);
            break;
        default:
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(200, 200, 200, 255);
            break;
        }

        g_pRenderText->RenderText(hx + HELP_CONTENT_LEFT, y, line.Text, contentWidth,
            HELP_LINE_HEIGHT, RT3_SORT_LEFT_CLIP);
    }

    // Scroll hint on the bottom strip, only when there is more to read.
    if (maxScroll > 0)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(160, 160, 160, 255);
        g_pRenderText->RenderText(hx, hy + HELP_HEIGHT - FRAME_BOTTOM_HEIGHT + 14,
            L"Role com o mouse para ver mais", HELP_WIDTH, 0, RT3_SORT_CENTER);
    }
}

void ReceiveEventSchedule(const BYTE* buffer, int size)
{
    if (auto* window = g_pEventScheduleWindow)
    {
        window->ReceiveSchedule(buffer, size);
    }
}
