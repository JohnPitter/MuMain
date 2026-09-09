#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Maintenance/NewUIMaintenanceWindow.h"

#include "Audio/DSPlaySound.h"
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

    // Steel ink for the schedule line, grey for the message body (the same
    // pair the changelog popup uses for dates and body text).
    constexpr BYTE kScheduleRed = 150;
    constexpr BYTE kScheduleGreen = 180;
    constexpr BYTE kScheduleBlue = 215;
    constexpr BYTE kBodyRed = 210;
    constexpr BYTE kBodyGreen = 210;
    constexpr BYTE kBodyBlue = 210;

    // Background tiling grid and header 3-slice geometry, copied from the
    // changelog window (the validated house pattern for windows built from
    // the shared 190 px inventory frame textures).
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

    // Footer button contrast socket, copied from the changelog window: the
    // dark footer textures vanish on the near-black msgbox_back tile, so the
    // button sits in a translucent well ringed in the title gold.
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

CNewUIMaintenanceWindow::CNewUIMaintenanceWindow()
    : m_pNewUIMng(NULL)
    , m_bAutoShowPending(false)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
    memset(&m_Notice, 0, sizeof(m_Notice));
}

CNewUIMaintenanceWindow::~CNewUIMaintenanceWindow()
{
    Release();
}

bool CNewUIMaintenanceWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == NULL)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MAINTENANCE, this);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);
    return true;
}

void CNewUIMaintenanceWindow::Release()
{
    UnloadImages();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CNewUIMaintenanceWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    m_BtnOk.ChangeButtonInfo(m_Pos.x + OK_BUTTON_X, m_Pos.y + OK_BUTTON_Y, OK_BUTTON_WIDTH, OK_BUTTON_HEIGHT);
}

void CNewUIMaintenanceWindow::InitButtons()
{
    // The single OK button: the shared 108x29 empty-button plate (owned by
    // the message-box manager — no extra LoadBitmap here); overflg maps
    // up/hover/press to the texture's three stacked frames, exactly like the
    // changelog's "Mostrar tudo" button.
    m_BtnOk.ChangeButtonImgState(true, IMAGE_MAINTENANCE_BTN_OK, true);
    m_BtnOk.SetFont(g_hFontBold);
    m_BtnOk.ChangeText(I18N::Game::MaintenanceOk);
}

float CNewUIMaintenanceWindow::GetLayerDepth()
{
    // Above the regular windows, same tier as the news popup: the maintenance
    // notice must stay on top of whatever the player had open at login.
    return 5.2f;
}

float CNewUIMaintenanceWindow::GetKeyEventOrder()
{
    return 10.f;
}

void CNewUIMaintenanceWindow::OpenningProcess()
{
}

void CNewUIMaintenanceWindow::ClosingProcess()
{
}

void CNewUIMaintenanceWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_MAINTENANCE_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_MAINTENANCE_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_MAINTENANCE_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_MAINTENANCE_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_MAINTENANCE_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_MAINTENANCE_BTN_EXIT, GL_LINEAR);
}

void CNewUIMaintenanceWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_MAINTENANCE_BACK);
    DeleteBitmap(IMAGE_MAINTENANCE_TOP);
    DeleteBitmap(IMAGE_MAINTENANCE_LEFT);
    DeleteBitmap(IMAGE_MAINTENANCE_RIGHT);
    DeleteBitmap(IMAGE_MAINTENANCE_BOTTOM);
    DeleteBitmap(IMAGE_MAINTENANCE_BTN_EXIT);
}

void CNewUIMaintenanceWindow::ReceiveNotice(const BYTE* buffer, int size)
{
    maintenance_layout::ParsedNotice notice = {};
    if (maintenance_layout::parse_notice(buffer, size, &notice) != 0)
    {
        return;
    }

    // Only a live notice opens the window; a flag-0 packet (which the server
    // should never send) merely refreshes the text without arming the popup.
    m_Notice = notice;
    m_bAutoShowPending = notice.Active && notice.Message[0] != L'\0';
}

bool CNewUIMaintenanceWindow::ConsumeAutoShow()
{
    const bool pending = m_bAutoShowPending;
    m_bAutoShowPending = false;
    return pending;
}

bool CNewUIMaintenanceWindow::UpdateMouseEvent()
{
    // Baked close "X" in the header's right cap — same behavior as the
    // changelog window: close and swallow the click so it does not fall
    // through to world movement.
    if (IsPress(VK_LBUTTON)
        && CheckMouseIn(m_Pos.x + WINDOW_WIDTH - CLOSE_X_FROM_RIGHT, m_Pos.y + CLOSE_Y, CLOSE_W, CLOSE_H))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_MAINTENANCE);
        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnOk.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_MAINTENANCE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (IsPress(VK_LBUTTON))
    {
        if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
        {
            // A click outside the window closes it, like the news popup.
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_MAINTENANCE);
            PlayBuffer(SOUND_CLICK01);
        }

        MouseLButton = false;
        MouseLButtonPop = false;
        MouseLButtonPush = false;
        return false;
    }

    // Modal-ish: while it is open it owns the mouse so nothing falls through
    // to the world beneath.
    return false;
}

bool CNewUIMaintenanceWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MAINTENANCE))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_MAINTENANCE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUIMaintenanceWindow::Update()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MAINTENANCE))
    {
        return true;
    }

    return true;
}

// Same frame pieces and 3-slice geometry as the changelog window: msgbox_back
// tiled on a 190x429 grid, the 190x64 header as cap/middle/cap (the right cap
// carries the baked close "X"), 21 px side strips, and the 190x45 bottom
// strip stretched.
void CNewUIMaintenanceWindow::RenderWindowFrame(float x, float y, float w, float h)
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    for (float oy = 0.f; oy < h; oy += kBackSrcH)
    {
        const float th = (oy + kBackSrcH > h) ? (h - oy) : kBackSrcH;
        for (float ox = 0.f; ox < w; ox += kBackSrcW)
        {
            const float tw = (ox + kBackSrcW > w) ? (w - ox) : kBackSrcW;
            RenderImage(IMAGE_MAINTENANCE_BACK, x + ox, y + oy, tw, th);
        }
    }

    RenderImageStretch(IMAGE_MAINTENANCE_TOP, x, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
        0.f, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_MAINTENANCE_TOP, x + kHeaderCapW, y, w - kHeaderCapW * 2.f, float(FRAME_TOP_HEIGHT),
        kHeaderMidSrcX, 0.f, kHeaderMidSrcW, float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_MAINTENANCE_TOP, x + w - kHeaderCapW, y, kHeaderCapW, float(FRAME_TOP_HEIGHT),
        190.f - kHeaderCapW, 0.f, kHeaderCapW, float(FRAME_TOP_HEIGHT));

    const float middleHeight = h - float(FRAME_TOP_HEIGHT) - float(FRAME_BOTTOM_HEIGHT);
    RenderImageStretch(IMAGE_MAINTENANCE_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_MAINTENANCE_RIGHT, x + w - float(FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));

    RenderImageStretch(IMAGE_MAINTENANCE_BOTTOM, x, y + h - float(FRAME_BOTTOM_HEIGHT), w, float(FRAME_BOTTOM_HEIGHT),
        0.f, 0.f, 190.f, float(FRAME_BOTTOM_HEIGHT));
}

bool CNewUIMaintenanceWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    RenderWindowFrame(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y),
        static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + TITLE_Y, I18N::Game::MaintenanceTitle, WINDOW_WIDTH, 0, RT3_SORT_CENTER);

    int y = m_Pos.y + CONTENT_TOP;
    if (m_Notice.Schedule[0] != L'\0')
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(kScheduleRed, kScheduleGreen, kScheduleBlue, 255);
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, y, m_Notice.Schedule,
            CONTENT_WIDTH, LINE_HEIGHT, RT3_SORT_LEFT_CLIP);
        y += LINE_HEIGHT + SCHEDULE_GAP;
    }

    wchar_t lines[MESSAGE_LINES][changelog_layout::kWrapBufferChars] = {};
    const int lineCount = changelog_layout::wrap_text(
        m_Notice.Message, MESSAGE_LINE_CHARS, lines, MESSAGE_LINES);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(kBodyRed, kBodyGreen, kBodyBlue, 255);
    for (int line = 0; line < lineCount && line < MESSAGE_LINES; ++line)
    {
        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, y, lines[line],
            CONTENT_WIDTH, LINE_HEIGHT, RT3_SORT_LEFT_CLIP);
        y += LINE_HEIGHT;
    }

    RenderFooterButtonSocket(m_BtnOk);
    m_BtnOk.Render();
    DisableAlphaBlend();
    return true;
}

void ReceiveMaintenance(const BYTE* buffer, int size)
{
    auto* window = g_pMaintenanceWindow;
    if (window == nullptr)
    {
        return;
    }

    window->ReceiveNotice(buffer, size);
    if (window->ConsumeAutoShow() && !g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MAINTENANCE))
    {
        g_pNewUISystem->Show(SEASON3B::INTERFACE_MAINTENANCE);
    }
}
