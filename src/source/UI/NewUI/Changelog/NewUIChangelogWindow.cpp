#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Changelog/NewUIChangelogWindow.h"

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

    // Steel ink for dates, grey for body text (same pair the events help popup uses).
    constexpr BYTE kDateRed = 150;
    constexpr BYTE kDateGreen = 180;
    constexpr BYTE kDateBlue = 215;
    constexpr BYTE kBodyRed = 200;
    constexpr BYTE kBodyGreen = 200;
    constexpr BYTE kBodyBlue = 200;

    // Background tiling grid and header 3-slice geometry, copied from the
    // event schedule window (the validated house pattern for windows built
    // from the shared 190 px inventory frame textures).
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

    // Footer button contrast socket, copied from the event schedule window:
    // the dark footer textures vanish on the near-black msgbox_back tile, so
    // each button sits in a translucent well ringed in the title gold.
    void RenderFooterButtonSocket(CNewUIButton& button)
    {
        const POINT pos = button.GetPos();
        const POINT size = button.GetSize();
        const bool hot = button.GetBTState() != BUTTON_STATE_UP;

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
}

CNewUIChangelogWindow::CNewUIChangelogWindow()
    : m_pNewUIMng(NULL)
    , m_bExpanded(false)
    , m_bAwaitingFullList(false)
    , m_bAutoShowPending(false)
    , m_iEntryCount(0)
    , m_iScrollBlock(0)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
    memset(m_Entries, 0, sizeof(m_Entries));
}

CNewUIChangelogWindow::~CNewUIChangelogWindow()
{
    Release();
}

bool CNewUIChangelogWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == NULL)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_CHANGELOG, this);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);
    return true;
}

void CNewUIChangelogWindow::Release()
{
    UnloadImages();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CNewUIChangelogWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
    m_BtnShowAll.ChangeButtonInfo(m_Pos.x + SHOW_ALL_BUTTON_X, m_Pos.y + SHOW_ALL_BUTTON_Y, SHOW_ALL_BUTTON_WIDTH, SHOW_ALL_BUTTON_HEIGHT);
}

void CNewUIChangelogWindow::InitButtons()
{
    wchar_t closeText[256] = {};
    mu_swprintf(closeText, I18N::Game::CloseS, L"ESC");
    m_BtnExit.ChangeButtonImgState(true, IMAGE_CHANGELOG_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);

    // "Mostrar tudo" opens the full list. Same shared 108x29 empty-button
    // plate the other labeled windows use (owned by the message-box manager —
    // no extra LoadBitmap here); overflg maps up/hover/press to the texture's
    // three stacked frames, exactly like the events window's "Ajuda" button.
    m_BtnShowAll.ChangeButtonImgState(true, IMAGE_CHANGELOG_BTN_SHOW_ALL, true);
    m_BtnShowAll.SetFont(g_hFontBold);
    m_BtnShowAll.ChangeText(I18N::Game::ChangelogShowAll);
}

void CNewUIChangelogWindow::RenderFooterButtons()
{
    RenderFooterButtonSocket(m_BtnExit);
    m_BtnExit.Render();
    if (!m_bExpanded)
    {
        RenderFooterButtonSocket(m_BtnShowAll);
        m_BtnShowAll.Render();
    }
}

float CNewUIChangelogWindow::GetLayerDepth()
{
    // Above the regular windows (the event schedule sits at 4.6): the news
    // popup must stay on top of whatever the player had open at login.
    return 5.2f;
}

float CNewUIChangelogWindow::GetKeyEventOrder()
{
    return 10.f;
}

void CNewUIChangelogWindow::OpenningProcess()
{
    m_iScrollBlock = 0;
}

void CNewUIChangelogWindow::ClosingProcess()
{
    // m_bAwaitingFullList is deliberately NOT reset here: if the player closes
    // the popup while the "show all" reply is still in flight, the reply must
    // not reopen it (ConsumeAutoShow swallows that case).
}

void CNewUIChangelogWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_CHANGELOG_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_CHANGELOG_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_CHANGELOG_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_CHANGELOG_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_CHANGELOG_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_CHANGELOG_BTN_EXIT, GL_LINEAR);
}

void CNewUIChangelogWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_CHANGELOG_BACK);
    DeleteBitmap(IMAGE_CHANGELOG_TOP);
    DeleteBitmap(IMAGE_CHANGELOG_LEFT);
    DeleteBitmap(IMAGE_CHANGELOG_RIGHT);
    DeleteBitmap(IMAGE_CHANGELOG_BOTTOM);
    DeleteBitmap(IMAGE_CHANGELOG_BTN_EXIT);
}

void CNewUIChangelogWindow::RequestFullList()
{
    if (SocketClient == nullptr)
    {
        return;
    }

    BYTE packet[4] = { 0xC1, 4, changelog_layout::kGroup, changelog_layout::kSubCode };
    SocketClient->Send(packet, 4);
}

void CNewUIChangelogWindow::ReceiveEntries(const BYTE* buffer, int size)
{
    changelog_layout::ParsedEntry parsed[changelog_layout::kMaxEntries] = {};
    const int count = changelog_layout::parse_entries(buffer, size, parsed, changelog_layout::kMaxEntries);
    if (count < 0)
    {
        return;
    }

    if (m_bAwaitingFullList)
    {
        // This is the reply to a "Mostrar tudo" request: refresh the expanded
        // view in place and never auto-show (the popup may already be closed).
        m_bAwaitingFullList = false;
        m_bAutoShowPending = false;
    }
    else
    {
        // The login push: reset to the preview so a relog starts fresh.
        m_bExpanded = false;
        m_bAutoShowPending = count > 0;
    }

    memcpy(m_Entries, parsed, sizeof(parsed));
    m_iEntryCount = count;

    const int visible = VisibleBlocks();
    m_iScrollBlock = changelog_layout::clamp_scroll(m_iScrollBlock, VisibleEntryCount(), visible);
}

bool CNewUIChangelogWindow::ConsumeAutoShow()
{
    const bool pending = m_bAutoShowPending;
    m_bAutoShowPending = false;
    return pending;
}

int CNewUIChangelogWindow::VisibleEntryCount() const
{
    return m_bExpanded ? m_iEntryCount : (m_iEntryCount > kPreviewEntries ? kPreviewEntries : m_iEntryCount);
}

int CNewUIChangelogWindow::VisibleBlocks() const
{
    const int contentHeight = WINDOW_HEIGHT - CONTENT_TOP - FRAME_BOTTOM_HEIGHT - CONTENT_BOTTOM_MARGIN;
    int blocks = contentHeight / BLOCK_HEIGHT;
    if (blocks < 1)
    {
        blocks = 1;
    }

    return blocks;
}

bool CNewUIChangelogWindow::UpdateMouseEvent()
{
    // Baked close "X" in the header's right cap. The shared
    // CNewUISystem::HandleFrameCornerClose assumes the 190 px inventory frame
    // width, so this 320 px window checks its own corner box, same behavior:
    // close and swallow the click so it does not fall through to world movement.
    if (IsPress(VK_LBUTTON)
        && CheckMouseIn(m_Pos.x + WINDOW_WIDTH - CLOSE_X_FROM_RIGHT, m_Pos.y + CLOSE_Y, CLOSE_W, CLOSE_H))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_CHANGELOG);
        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_CHANGELOG);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (!m_bExpanded && m_BtnShowAll.UpdateMouseEvent())
    {
        m_bExpanded = true;
        m_bAwaitingFullList = true;
        m_iScrollBlock = 0;
        RequestFullList();
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    const int visibleBlocks = VisibleBlocks();
    if (m_bExpanded && MouseWheel != 0)
    {
        if (CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
        {
            m_iScrollBlock = changelog_layout::clamp_scroll(
                m_iScrollBlock - MouseWheel, m_iEntryCount, visibleBlocks);
        }

        MouseWheel = 0;
        return false;
    }

    if (IsPress(VK_LBUTTON))
    {
        if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
        {
            // A click outside the popup closes it, like the events help popup.
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_CHANGELOG);
            PlayBuffer(SOUND_CLICK01);
        }

        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        return false;
    }

    // The popup is modal-ish: while it is open it owns the mouse so nothing
    // falls through to the world beneath.
    return false;
}

bool CNewUIChangelogWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHANGELOG))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_CHANGELOG);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUIChangelogWindow::Update()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHANGELOG))
    {
        return true;
    }

    return true;
}

// Same frame pieces and 3-slice geometry as the event schedule window (both
// copied from the AutoBattler house pattern): msgbox_back tiled on a 190x429
// grid, the 190x64 header as cap/middle/cap (the right cap carries the baked
// close "X"), 21 px side strips, and the 190x45 bottom strip stretched.
void CNewUIChangelogWindow::RenderWindowFrame(float x, float y, float w, float h)
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    for (float oy = 0.f; oy < h; oy += kBackSrcH)
    {
        const float th = (oy + kBackSrcH > h) ? (h - oy) : kBackSrcH;
        for (float ox = 0.f; ox < w; ox += kBackSrcW)
        {
            const float tw = (ox + kBackSrcW > w) ? (w - ox) : kBackSrcW;
            RenderImage(IMAGE_CHANGELOG_BACK, x + ox, y + oy, tw, th);
        }
    }

    RenderImageStretch(IMAGE_CHANGELOG_TOP, x, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
        0.f, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_CHANGELOG_TOP, x + kHeaderCapW, y, w - kHeaderCapW * 2.f, float(FRAME_TOP_HEIGHT),
        kHeaderMidSrcX, 0.f, kHeaderMidSrcW, float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_CHANGELOG_TOP, x + w - kHeaderCapW, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
        190.f - kHeaderCapW, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));

    const float middleHeight = h - float(FRAME_TOP_HEIGHT) - float(FRAME_BOTTOM_HEIGHT);
    RenderImageStretch(IMAGE_CHANGELOG_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_CHANGELOG_RIGHT, x + w - float(FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));

    RenderImageStretch(IMAGE_CHANGELOG_BOTTOM, x, y + h - float(FRAME_BOTTOM_HEIGHT), w, float(FRAME_BOTTOM_HEIGHT),
        0.f, 0.f, 190.f, float(FRAME_BOTTOM_HEIGHT));
}

bool CNewUIChangelogWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    RenderWindowFrame(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y),
        static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::ChangelogTitle, WINDOW_WIDTH, 0, RT3_SORT_CENTER);

    if (m_iEntryCount == 0)
    {
        wchar_t emptyText[128] = {};
        mu_swprintf(emptyText, L"%ls", I18N::Game::ChangelogNoNews);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(kBodyRed, kBodyGreen, kBodyBlue, 255);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, m_Pos.y + CONTENT_TOP + BLOCK_HEIGHT,
            emptyText, CONTENT_WIDTH, LINE_HEIGHT, RT3_SORT_CENTER);

        RenderFooterButtons();
        DisableAlphaBlend();
        return true;
    }

    const int entryCount = VisibleEntryCount();
    const int startBlock = m_bExpanded ? m_iScrollBlock : 0;
    const int visibleBlocks = VisibleBlocks();
    const int contentX = m_Pos.x + CONTENT_LEFT;
    const int contentWidth = CONTENT_WIDTH;

    for (int block = 0; block < visibleBlocks; ++block)
    {
        const int index = startBlock + block;
        if (index >= entryCount)
        {
            break;
        }

        const changelog_layout::ParsedEntry& entry = m_Entries[index];
        const int y = m_Pos.y + CONTENT_TOP + block * BLOCK_HEIGHT;

        wchar_t dateText[changelog_layout::kDateTextChars] = {};
        changelog_layout::format_date(entry.YearOffset, entry.Month, entry.Day, dateText, changelog_layout::kDateTextChars);

        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(kDateRed, kDateGreen, kDateBlue, 255);
        g_pRenderText->RenderText(contentX, y, dateText, DATE_TEXT_WIDTH, LINE_HEIGHT, RT3_SORT_LEFT);

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
        g_pRenderText->RenderText(contentX + DATE_TEXT_WIDTH, y, entry.Title,
            contentWidth - DATE_TEXT_WIDTH, LINE_HEIGHT, RT3_SORT_LEFT_CLIP);

        wchar_t lines[DESC_LINES_PER_ENTRY][changelog_layout::kWrapBufferChars] = {};
        const int lineCount = changelog_layout::wrap_text(
            entry.Description, changelog_layout::kDescLineChars, lines, DESC_LINES_PER_ENTRY);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(kBodyRed, kBodyGreen, kBodyBlue, 255);
        for (int line = 0; line < lineCount && line < DESC_LINES_PER_ENTRY; ++line)
        {
            g_pRenderText->RenderText(contentX, y + (line + 1) * LINE_HEIGHT, lines[line],
                contentWidth, LINE_HEIGHT, RT3_SORT_LEFT_CLIP);
        }
    }

    // Scroll hint on the bottom strip, only when the expanded list has more below.
    if (m_bExpanded)
    {
        const int hidden = m_iEntryCount - startBlock - visibleBlocks;
        if (hidden > 0)
        {
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(160, 160, 160, 255);
            g_pRenderText->RenderText(m_Pos.x, m_Pos.y + WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT + SCROLL_HINT_Y_OFFSET,
                I18N::Game::ChangelogScrollHint, WINDOW_WIDTH, 0, RT3_SORT_CENTER);
        }
    }
    else if (m_iEntryCount > kPreviewEntries)
    {
        // The preview shows only the newest entries and there are more on the server.
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(160, 160, 160, 255);
        g_pRenderText->RenderText(m_Pos.x, m_Pos.y + WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT + SCROLL_HINT_Y_OFFSET,
            I18N::Game::ChangelogScrollHint, WINDOW_WIDTH, 0, RT3_SORT_CENTER);
    }

    RenderFooterButtons();
    DisableAlphaBlend();
    return true;
}

void ReceiveChangelog(const BYTE* buffer, int size)
{
    auto* window = g_pChangelogWindow;
    if (window == nullptr)
    {
        return;
    }

    window->ReceiveEntries(buffer, size);
    if (window->ConsumeAutoShow() && !g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_CHANGELOG))
    {
        g_pNewUISystem->Show(SEASON3B::INTERFACE_CHANGELOG);
    }
}
