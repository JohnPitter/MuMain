//*****************************************************************************
// File: LoginMainWin.cpp
//*****************************************************************************

#include "stdafx.h"
#include "UI/Windows/LoginMainWin.h"

#include "Core/Input/Input.h"
#include "UI/Legacy/UIMng.h"
#include "Network/Server/WSclient.h"

namespace
{
    constexpr int kLayoutWidth = 800;
    constexpr int kLayoutHeight = 600;
    constexpr int kButtonWidth = 54;
    constexpr int kButtonHeight = 30;
    constexpr int kDecoWidth = 189;
    constexpr int kDecoHeight = 103;
    constexpr int kDecoDatumX = 105;
    constexpr int kDecoDatumY = 59;

    void LoginLayoutScale(float& scaleX, float& scaleY)
    {
        CInput& input = CInput::Instance();
        scaleX = static_cast<float>(input.GetScreenWidth()) / static_cast<float>(kLayoutWidth);
        scaleY = static_cast<float>(input.GetScreenHeight()) / static_cast<float>(kLayoutHeight);
        if (scaleX <= 0.f)
            scaleX = 1.f;
        if (scaleY <= 0.f)
            scaleY = 1.f;
    }
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

CLoginMainWin::CLoginMainWin()
{
}

CLoginMainWin::~CLoginMainWin()
{
}

//=============================================================================
// Public Methods
//=============================================================================

void CLoginMainWin::Create()
{
    float scaleX = 1.f;
    float scaleY = 1.f;
    LoginLayoutScale(scaleX, scaleY);

    for (int i = 0; i <= LMW_BTN_CREDIT; ++i)
    {
        m_aBtn[i].Create(kButtonWidth, kButtonHeight, BITMAP_LOG_IN + 4 + i, 3, 2, 1);
        m_aBtn[i].SetScale(scaleX, scaleY);
    }

    CWin::Create(
        static_cast<int>(kLayoutWidth * scaleX),
        static_cast<int>(kButtonHeight * scaleY),
        -2
    );

    for (int i = 0; i < LMW_BTN_MAX; ++i)
        CWin::RegisterButton(&m_aBtn[i]);

    m_sprDecoCredit.Create(kDecoWidth, kDecoHeight, BITMAP_LOG_IN + 6, 0, nullptr,
        kDecoDatumX, kDecoDatumY, false, SPR_SIZING_DATUMS_LT, scaleX, scaleY);

    const int menuDatumX = kDecoWidth - kDecoDatumX;
    m_sprDecoMenu.Create(kDecoWidth, kDecoHeight, BITMAP_LOG_IN + 6, 0, nullptr,
        menuDatumX, kDecoDatumY, false, SPR_SIZING_DATUMS_LT, scaleX, scaleY);
    m_sprDecoMenu.FlipHorizontal();
}

void CLoginMainWin::PreRelease()
{
    m_sprDecoMenu.Release();
    m_sprDecoCredit.Release();
}

void CLoginMainWin::SetPosition(int, int)
{
    float scaleX = 1.f;
    float scaleY = 1.f;
    LoginLayoutScale(scaleX, scaleY);

    const int decoRightOverhang = kDecoWidth - kDecoDatumX;
    const int decoBottomOverhang = kDecoHeight - kDecoDatumY;
    const int btnY = kLayoutHeight - decoBottomOverhang;
    const int menuX = decoRightOverhang;
    const int creditX = kLayoutWidth - decoRightOverhang;

    CWin::SetPosition(0, static_cast<int>(btnY * scaleY));
    (void)scaleX;

    m_aBtn[LMW_BTN_MENU].SetPosition(menuX, btnY);
    m_aBtn[LMW_BTN_CREDIT].SetPosition(creditX, btnY);

    m_sprDecoMenu.SetPosition(menuX, btnY);
    m_sprDecoCredit.SetPosition(creditX, btnY);
}

void CLoginMainWin::Show(bool bShow)
{
    CWin::Show(bShow);

    for (int i = 0; i < LMW_BTN_MAX; ++i)
        m_aBtn[i].Show(bShow);

    m_sprDecoMenu.Show(bShow);
    m_sprDecoCredit.Show(bShow);
}

bool CLoginMainWin::CursorInWin(int nArea)
{
    if (!CWin::m_bShow)
        return false;

    switch (nArea)
    {
    case WA_MOVE:
        return false;
    }

    return CWin::CursorInWin(nArea);
}

void CLoginMainWin::UpdateWhileActive(double dDeltaTick)
{
    CUIMng& rUIMng = CUIMng::Instance();

    if (m_aBtn[LMW_BTN_MENU].IsClick())
    {
        rUIMng.ShowWin(&rUIMng.m_SysMenuWin);
        rUIMng.SetSysMenuWinShow(true);
    }
    else if (m_aBtn[LMW_BTN_CREDIT].IsClick())
    {
        SocketClient->ToConnectServer()->SendServerListRequest();

        rUIMng.ShowWin(&rUIMng.m_CreditWin);

        ::StopMp3(MUSIC_MAIN_THEME);
        ::PlayMp3(MUSIC_MUTHEME);
    }
}

void CLoginMainWin::RenderControls()
{
    m_sprDecoMenu.Render();
    m_sprDecoCredit.Render();
    CWin::RenderButtons();
}
