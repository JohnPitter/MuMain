//*****************************************************************************
// File: CharInfoBalloon.cpp
//*****************************************************************************

#include "stdafx.h"
#include "CharInfoBalloon.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Engine/Object/ZzzInterface.h"
#include "UI/Legacy/UIControls.h"
#include "CharacterManager.h"
#include "I18N/All.h"

#include <algorithm>
#include <cwchar>

#include "Camera/CameraProjection.h"

namespace
{
    template <std::size_t N>
    void CopyWideString(wchar_t (&destination)[N], const wchar_t* source)
    {
        if (source == nullptr)
        {
            destination[0] = L'\0';
            return;
        }

        std::wcsncpy(destination, source, N - 1);
        destination[N - 1] = L'\0';
    }

    DWORD ResolveNameColor(std::uint8_t controlCode)
    {
        if (controlCode & CTLCODE_01BLOCKCHAR)
            return ARGB(255, 0, 255, 255);
        if (controlCode & (CTLCODE_02BLOCKITEM | CTLCODE_10ACCOUNT_BLOCKITEM))
            return CLRDW_BR_ORANGE;
        if (controlCode & CTLCODE_04FORTV)
            return CLRDW_WHITE;
        if (controlCode & (CTLCODE_08OPERATOR | CTLCODE_20OPERATOR))
            return ARGB(255, 255, 0, 0);

        return CLRDW_WHITE;
    }
}

CCharInfoBalloon::CCharInfoBalloon() : m_pCharInfo(nullptr)
{
    I18N::RegisterLocaleObserver(&CCharInfoBalloon::OnLocaleChanged, this);
}

CCharInfoBalloon::~CCharInfoBalloon()
{
    I18N::UnregisterLocaleObserver(&CCharInfoBalloon::OnLocaleChanged, this);
}

void CCharInfoBalloon::OnLocaleChanged(void* ctx) noexcept
{
    auto* self = static_cast<CCharInfoBalloon*>(ctx);
    if (self->m_pCharInfo != nullptr)
    {
        self->SetInfo();
    }
}

void CCharInfoBalloon::Create(CHARACTER* pCharInfo)
{
    CSprite::Create(118, 54, BITMAP_LOG_IN + 7, 0, nullptr, 59, 54);

    m_pCharInfo = pCharInfo;
    m_dwNameColor = 0;
    std::fill(std::begin(m_szName), std::end(m_szName), L'\0');
    std::fill(std::begin(m_szGuild), std::end(m_szGuild), L'\0');
    std::fill(std::begin(m_szClass), std::end(m_szClass), L'\0');
}

void CCharInfoBalloon::Render()
{
    if (m_pCharInfo == nullptr || !CSprite::m_bShow)
        return;

    CSprite::Render();

    vec3_t afPos;
    VectorCopy(m_pCharInfo->Object.Position, afPos);
    afPos[2] += 350.0f;

    int nPosX, nPosY;
    CameraProjection::WorldToScreen(g_Camera, afPos, &nPosX, &nPosY);

    CSprite::SetPosition(
        int(ConvertPosX(static_cast<float>(nPosX))),
        int(ConvertPosY(static_cast<float>(nPosY)))
    );

    EnableAlphaBlend();
    g_pRenderText->SetFont(g_hFixFont);
    g_pRenderText->SetBgColor(0);

    const int spriteX = CSprite::GetXPos();
    const int spriteY = CSprite::GetYPos();
    const int spriteW = CSprite::GetWidth();

    const int nTextPosX = int((spriteX - g_fScreenOff_x) / g_fScreenRate_x);
    const int boxW = spriteW / g_fScreenRate_x;

    g_pRenderText->SetTextColor(m_dwNameColor);
    g_pRenderText->RenderText(
        nTextPosX,
        int((spriteY + 6 - g_fScreenOff_y) / g_fScreenRate_y),
        m_szName,
        boxW,
        0,
        RT3_SORT_CENTER
    );

    // Never draw the guild/status line on character select. Empty text with
    // boxW > 0 still goes through GDI and reprints the previous glyph (the
    // first letter of the name: "e" under erererer, "S" under SeuAntonio).
    if (m_szClass[0] != L'\0')
    {
        g_pRenderText->SetTextColor(CLRDW_BR_ORANGE);
        g_pRenderText->RenderText(
            nTextPosX,
            int((spriteY + 28 - g_fScreenOff_y) / g_fScreenRate_y),
            m_szClass,
            boxW,
            0,
            RT3_SORT_CENTER
        );
    }
    EnableAlphaTest();
}

void CCharInfoBalloon::SetInfo()
{
    if (m_pCharInfo == nullptr)
        return;

    if (!m_pCharInfo->Object.Live)
    {
        CSprite::m_bShow = false;
        return;
    }

    CSprite::m_bShow = true;

    m_dwNameColor = ResolveNameColor(m_pCharInfo->CtlCode);

    CopyWideString(m_szName, m_pCharInfo->ID);

    // Select balloon is name + class/level only. Status 255, Commoner/Plebeu,
    // guild role, and any leftover initial must never appear under the name.
    m_szGuild[0] = L'\0';

    mu_swprintf_s(m_szClass, L"%ls %d",
        gCharacterManager.GetCharacterClassText(m_pCharInfo->Class),
        m_pCharInfo->Level);
}