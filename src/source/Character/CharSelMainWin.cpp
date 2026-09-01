//*****************************************************************************
// File: CharSelMainWin.cpp
//*****************************************************************************

#include "stdafx.h"
#include "CharSelMainWin.h"
#include "Core/Input/Input.h"
#include "UI/Legacy/UIMng.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include "Guild/UIGuildInfo.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Network/Server/ServerListManager.h"
#include "I18N/All.h"

#include <algorithm>
#include <utility>

#include "Scenes/SceneCommon.h"

namespace
{
    constexpr int kCharacterSlotCount = 5;
    constexpr int kButtonSpacing = 1;
    constexpr int kInfoSpacing = 2;
    constexpr int kInfoOffsetY = 5;
    constexpr int kLayoutWidth = 800;
    constexpr int kLayoutHeight = 600;
    constexpr int kButtonWidth = 54;
    constexpr int kButtonHeight = 30;
    constexpr int kDecoWidth = 189;
    constexpr int kDecoHeight = 103;
    constexpr int kDecoDatumX = 105;
    constexpr int kDecoDatumY = 59;
    constexpr int kMarginX = 30;
    constexpr int kAccountBlockMsgX = 320;
    constexpr int kAccountBlockPrimaryY = 330;
    constexpr int kAccountBlockSecondaryY = 348;
    constexpr int kWindowAlpha = 143;
    constexpr int kInfoSpriteHeight = 21;

    CSprite g_sprDecoLeft;

    void LayoutScale(float& scaleX, float& scaleY)
    {
        CInput& input = CInput::Instance();
        scaleX = static_cast<float>(input.GetScreenWidth()) / static_cast<float>(kLayoutWidth);
        scaleY = static_cast<float>(input.GetScreenHeight()) / static_cast<float>(kLayoutHeight);
        if (scaleX <= 0.f)
            scaleX = 1.f;
        if (scaleY <= 0.f)
            scaleY = 1.f;
    }

    template <typename Predicate>
    bool AnyCharacter(Predicate&& predicate)
    {
        return std::any_of(
            CharactersClient,
            CharactersClient + kCharacterSlotCount,
            std::forward<Predicate>(predicate));
    }

    bool HasAccountBlockedCharacter()
    {
        return AnyCharacter([](const CHARACTER& character)
        {
            return character.Object.Live != 0
                && (character.CtlCode & CTLCODE_10ACCOUNT_BLOCKITEM);
        });
    }

    bool HasEmptyCharacterSlot()
    {
        return AnyCharacter([](const CHARACTER& character)
        {
            return character.Object.Live == 0;
        });
    }

    bool HasLiveCharacter()
    {
        return AnyCharacter([](const CHARACTER& character)
        {
            return character.Object.Live != 0;
        });
    }

    CHARACTER* GetSelectedCharacter()
    {
        if (SelectedHero < 0 || SelectedHero >= kCharacterSlotCount)
            return nullptr;
        return &CharactersClient[SelectedHero];
    }

    void RenderAccountBlockMessage()
    {
        g_pRenderText->SetTextColor(0, 0, 0, 255);
        g_pRenderText->SetBgColor(255, 255, 0, 128);
        g_pRenderText->RenderText(kAccountBlockMsgX, kAccountBlockPrimaryY, I18N::Game::ThisAccountIsItemBlocked, 0, 0, RT3_WRITE_CENTER);
        g_pRenderText->RenderText(kAccountBlockMsgX, kAccountBlockSecondaryY, I18N::Game::PleaseCheckOnHttpMuonlineWebzenComSite, 0, 0, RT3_WRITE_CENTER);
    }
}

CCharSelMainWin::CCharSelMainWin()
{
}

CCharSelMainWin::~CCharSelMainWin()
{
}

void CCharSelMainWin::Create()
{
    float scaleX = 1.f;
    float scaleY = 1.f;
    LayoutScale(scaleX, scaleY);

    m_aBtn[CSMW_BTN_CREATE].Create(kButtonWidth, kButtonHeight, BITMAP_LOG_IN + 3, 4, 2, 1, 3);
    m_aBtn[CSMW_BTN_MENU].Create(kButtonWidth, kButtonHeight, BITMAP_LOG_IN + 4, 3, 2, 1);
    m_aBtn[CSMW_BTN_CONNECT].Create(kButtonWidth, kButtonHeight, BITMAP_LOG_IN + 5, 4, 2, 1, 3);
    m_aBtn[CSMW_BTN_DELETE].Create(kButtonWidth, kButtonHeight, BITMAP_LOG_IN + 6, 4, 2, 1, 3);
    for (int i = 0; i < CSMW_BTN_MAX; ++i)
        m_aBtn[i].SetScale(scaleX, scaleY);

    CWin::Create(
        static_cast<int>(kLayoutWidth * scaleX),
        static_cast<int>(kButtonHeight * scaleY),
        -2);

    for (int i = 0; i < CSMW_BTN_MAX; ++i)
        CWin::RegisterButton(&m_aBtn[i]);

    m_asprBack[CSMW_SPR_DECO].Create(kDecoWidth, kDecoHeight, BITMAP_LOG_IN + 2, 0, nullptr,
        kDecoDatumX, kDecoDatumY, false, SPR_SIZING_DATUMS_LT, scaleX, scaleY);

    g_sprDecoLeft.Create(kDecoWidth, kDecoHeight, BITMAP_LOG_IN + 2, 0, nullptr,
        kDecoWidth - kDecoDatumX, kDecoDatumY, false, SPR_SIZING_DATUMS_LT, scaleX, scaleY);
    g_sprDecoLeft.FlipHorizontal();

    m_asprBack[CSMW_SPR_INFO].Create(8, kInfoSpriteHeight, -1, 0, nullptr, 0, 0, false,
        SPR_SIZING_DATUMS_LT, scaleX, scaleY);
    m_asprBack[CSMW_SPR_INFO].SetColor(0, 0, 0);
    m_asprBack[CSMW_SPR_INFO].SetAlpha(kWindowAlpha);

    m_bAccountBlockItem = HasAccountBlockedCharacter();
}

void CCharSelMainWin::PreRelease()
{
    for (int i = 0; i < CSMW_SPR_MAX; ++i)
        m_asprBack[i].Release();
    g_sprDecoLeft.Release();
}

void CCharSelMainWin::SetPosition(int, int)
{
    float scaleX = 1.f;
    float scaleY = 1.f;
    LayoutScale(scaleX, scaleY);

    const int decoRightOverhang = kDecoWidth - kDecoDatumX;
    const int decoBottomOverhang = kDecoHeight - kDecoDatumY;
    const int btnY = kLayoutHeight - decoBottomOverhang;
    const int createX = kMarginX;
    const int menuX = createX + kButtonWidth + kButtonSpacing;
    const int deleteX = kLayoutWidth - decoRightOverhang;
    const int connectX = deleteX - kButtonWidth - kButtonSpacing;

    CWin::SetPosition(0, static_cast<int>(btnY * scaleY));
    (void)scaleX;

    m_aBtn[CSMW_BTN_CREATE].SetPosition(createX, btnY);
    m_aBtn[CSMW_BTN_MENU].SetPosition(menuX, btnY);
    m_aBtn[CSMW_BTN_CONNECT].SetPosition(connectX, btnY);
    m_aBtn[CSMW_BTN_DELETE].SetPosition(deleteX, btnY);

    g_sprDecoLeft.SetPosition(createX, btnY);
    m_asprBack[CSMW_SPR_DECO].SetPosition(deleteX, btnY);

    const int infoX = menuX + kButtonWidth + kInfoSpacing;
    const int infoWidth = connectX - infoX - kInfoSpacing;
    m_asprBack[CSMW_SPR_INFO].SetSize(infoWidth, kInfoSpriteHeight);
    m_asprBack[CSMW_SPR_INFO].SetPosition(infoX, btnY + kInfoOffsetY);
}

void CCharSelMainWin::Show(bool bShow)
{
    CWin::Show(bShow);

    for (auto& sprite : m_asprBack)
        sprite.Show(bShow);
    g_sprDecoLeft.Show(bShow);
    for (auto& button : m_aBtn)
        button.Show(bShow);
}

bool CCharSelMainWin::CursorInWin(int nArea)
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

void CCharSelMainWin::UpdateDisplay()
{
    m_aBtn[CSMW_BTN_CREATE].SetEnable(HasEmptyCharacterSlot());

    if (SelectedHero < 0 || SelectedHero >= kCharacterSlotCount
        || CharactersClient[SelectedHero].Object.Live == 0
        || CharactersClient[SelectedHero].ID[0] == L'\0')
    {
        SelectedHero = -1;
        for (int i = 0; i < kCharacterSlotCount; ++i)
        {
            if (CharactersClient[i].Object.Live != 0 && CharactersClient[i].ID[0] != L'\0')
            {
                SelectedHero = i;
                break;
            }
        }
    }

    const bool hasSelection = (SelectedHero > -1);
    m_aBtn[CSMW_BTN_CONNECT].SetEnable(hasSelection);
    m_aBtn[CSMW_BTN_DELETE].SetEnable(hasSelection);

    if (!HasLiveCharacter())
    {
        CUIMng& rUIMng = CUIMng::Instance();
        rUIMng.ShowWin(&rUIMng.m_CharMakeWin);
    }
}

void CCharSelMainWin::UpdateWhileActive(double dDeltaTick)
{
    CUIMng& uiManager = CUIMng::Instance();

    if (m_aBtn[CSMW_BTN_CONNECT].IsClick())
    {
        ::StartGame();
    }
    else if (m_aBtn[CSMW_BTN_MENU].IsClick())
    {
        uiManager.ShowWin(&uiManager.m_SysMenuWin);
        uiManager.SetSysMenuWinShow(true);
    }
    else if (m_aBtn[CSMW_BTN_CREATE].IsClick())
    {
        uiManager.ShowWin(&uiManager.m_CharMakeWin);
    }
    else if (m_aBtn[CSMW_BTN_DELETE].IsClick())
    {
        DeleteCharacter();
    }
}

void CCharSelMainWin::RenderControls()
{
    g_sprDecoLeft.Render();
    for (auto& sprite : m_asprBack)
        sprite.Render();

    ::EnableAlphaTest();
    ::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    g_pRenderText->SetFont(g_hFixFont);
    g_pRenderText->SetTextColor(CLRDW_WHITE);
    g_pRenderText->SetBgColor(0);

    if (m_bAccountBlockItem)
        RenderAccountBlockMessage();

    CWin::RenderButtons();
}

void CCharSelMainWin::DeleteCharacter()
{
    CHARACTER* selected = GetSelectedCharacter();
    if (selected == nullptr)
        return;

    CUIMng& uiManager = CUIMng::Instance();

    if (selected->GuildStatus != G_NONE)
    {
        uiManager.PopUpMsgWin(MESSAGE_DELETE_CHARACTER_GUILDWARNING);
    }
    else if (selected->CtlCode & (CTLCODE_02BLOCKITEM | CTLCODE_10ACCOUNT_BLOCKITEM))
    {
        uiManager.PopUpMsgWin(MESSAGE_DELETE_CHARACTER_ID_BLOCK);
    }
    else
    {
        uiManager.PopUpMsgWin(MESSAGE_DELETE_CHARACTER_CONFIRM);
    }
}