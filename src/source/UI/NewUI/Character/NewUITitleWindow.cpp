#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/Character/NewUITitleWindow.h"

#include "Audio/DSPlaySound.h"
#include "Character/CharacterTitle.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

using namespace SEASON3B;

namespace
{
    constexpr BYTE kTitleRed = 255;
    constexpr BYTE kTitleGreen = 220;
    constexpr BYTE kTitleBlue = 120;
}

CNewUITitleWindow::CNewUITitleWindow()
    : m_pNewUIMng(NULL)
    , m_scrollOffset(0)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
}

CNewUITitleWindow::~CNewUITitleWindow()
{
    Release();
}

bool CNewUITitleWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == NULL)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_TITLE, this);

    LoadImages();
    SetPos(x, y);
    InitButtons();
    Show(false);
    return true;
}

void CNewUITitleWindow::Release()
{
    UnloadImages();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CNewUITitleWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    m_BtnExit.ChangeButtonInfo(m_Pos.x + EXIT_BUTTON_X, m_Pos.y + EXIT_BUTTON_Y, EXIT_BUTTON_WIDTH, EXIT_BUTTON_HEIGHT);
    m_BtnToggle.ChangeButtonInfo(m_Pos.x + TOGGLE_X, m_Pos.y + TOGGLE_Y, TOGGLE_WIDTH, TOGGLE_HEIGHT);
}

void CNewUITitleWindow::InitButtons()
{
    wchar_t closeText[256] = {};
    mu_swprintf(closeText, I18N::Game::CloseS, L"Y");
    m_BtnExit.ChangeButtonImgState(true, IMAGE_TITLE_BTN_EXIT);
    m_BtnExit.ChangeToolTipText(closeText, true);

    m_BtnToggle.ChangeButtonImgState(true, IMAGE_TITLE_BTN, true);
    m_BtnToggle.ChangeTextColor(0xFFFFDC78);
}

float CNewUITitleWindow::GetLayerDepth()
{
    return 4.6f;
}

float CNewUITitleWindow::GetKeyEventOrder()
{
    return 10.f;
}

void CNewUITitleWindow::OpenningProcess()
{
    m_scrollOffset = 0;
}

void CNewUITitleWindow::ClosingProcess()
{
}

void CNewUITitleWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_TITLE_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_TITLE_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_TITLE_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_TITLE_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_TITLE_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_TITLE_BTN_EXIT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_btn_empty_small.tga", IMAGE_TITLE_BTN, GL_LINEAR);
}

void CNewUITitleWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_TITLE_BACK);
    DeleteBitmap(IMAGE_TITLE_TOP);
    DeleteBitmap(IMAGE_TITLE_LEFT);
    DeleteBitmap(IMAGE_TITLE_RIGHT);
    DeleteBitmap(IMAGE_TITLE_BOTTOM);
    DeleteBitmap(IMAGE_TITLE_BTN_EXIT);
    DeleteBitmap(IMAGE_TITLE_BTN);
}

int CNewUITitleWindow::HitRow() const
{
    if (!CheckMouseIn(m_Pos.x + CONTENT_LEFT, m_Pos.y + CONTENT_TOP, CONTENT_WIDTH, VISIBLE_ROWS * ROW_HEIGHT))
    {
        return -1;
    }

    const int row = (MouseY - (m_Pos.y + CONTENT_TOP)) / ROW_HEIGHT;
    if (row < 0 || row >= VISIBLE_ROWS)
    {
        return -1;
    }

    return m_scrollOffset + row;
}

bool CNewUITitleWindow::UpdateMouseEvent()
{
    if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, SEASON3B::INTERFACE_TITLE))
    {
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnExit.UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_TITLE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (m_BtnToggle.UpdateMouseEvent())
    {
        CharacterTitle::ToggleHidden();
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    const auto& ranks = CharacterTitle::Visible();
    const int hidden = static_cast<int>(ranks.size()) - VISIBLE_ROWS;
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

    if (IsRelease(VK_LBUTTON))
    {
        const int index = HitRow();
        if (index >= 0 && static_cast<size_t>(index) < ranks.size())
        {
            CharacterTitle::Select(ranks[index].Id);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
    }

    if (!CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        return true;
    }

    return false;
}

bool CNewUITitleWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_TITLE))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_TITLE);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUITitleWindow::Update()
{
    return true;
}

void CNewUITitleWindow::RenderBaseWindow()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    const auto middleHeight = static_cast<float>(WINDOW_HEIGHT - FRAME_TOP_HEIGHT - FRAME_BOTTOM_HEIGHT);

    RenderImage(IMAGE_TITLE_BACK, x, y, float(WINDOW_WIDTH), float(WINDOW_HEIGHT));
    RenderImage(IMAGE_TITLE_TOP, x, y, float(WINDOW_WIDTH), float(FRAME_TOP_HEIGHT));
    RenderImageStretch(IMAGE_TITLE_LEFT, x, y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImageStretch(IMAGE_TITLE_RIGHT, x + float(WINDOW_WIDTH - FRAME_SIDE_WIDTH), y + float(FRAME_TOP_HEIGHT), float(FRAME_SIDE_WIDTH), middleHeight,
        0.f, 0.f, float(FRAME_SIDE_WIDTH), float(FRAME_SIDE_TEXTURE_HEIGHT));
    RenderImage(IMAGE_TITLE_BOTTOM, x, y + float(WINDOW_HEIGHT - FRAME_BOTTOM_HEIGHT), float(WINDOW_WIDTH), float(FRAME_BOTTOM_HEIGHT));
}

bool CNewUITitleWindow::Render()
{
    EnableAlphaTest();
    glColor4f(1.f, 1.f, 1.f, 1.f);

    RenderBaseWindow();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + TITLE_Y, L"T\u00edtulo", WINDOW_WIDTH, 0, RT3_SORT_CENTER);

    g_pRenderText->SetFont(g_hFont);
    const auto& ranks = CharacterTitle::Visible();
    const int selected = CharacterTitle::SelectedId();

    for (int row = 0; row < VISIBLE_ROWS; ++row)
    {
        const size_t index = static_cast<size_t>(m_scrollOffset + row);
        if (index >= ranks.size())
        {
            break;
        }

        const int y = m_Pos.y + CONTENT_TOP + row * ROW_HEIGHT;
        const bool isSelected = ranks[index].Id == selected;
        if (isSelected)
        {
            g_pRenderText->SetTextColor(kTitleRed, kTitleGreen, kTitleBlue, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(220, 220, 220, 255);
        }

        const wchar_t* label = ranks[index].Name;
        wchar_t autoLabel[64] = {};
        if (ranks[index].Id == 0)
        {
            mu_swprintf(autoLabel, L"%ls (%ls)", L"Autom\u00e1tico", CharacterTitle::FromHeroState(Hero ? Hero->PK : 3));
            label = autoLabel;
        }

        g_pRenderText->RenderText(m_Pos.x + CONTENT_LEFT, y, label, CONTENT_WIDTH, ROW_HEIGHT, RT3_SORT_LEFT);
    }

    m_BtnToggle.ChangeText(CharacterTitle::IsHidden() ? L"Mostrar t\u00edtulo" : L"Ocultar t\u00edtulo");
    m_BtnToggle.SetFont(g_hFont);
    m_BtnToggle.Render();

    m_BtnExit.Render();
    DisableAlphaBlend();
    return true;
}
