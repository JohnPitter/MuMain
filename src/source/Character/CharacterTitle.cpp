#include "stdafx.h"

#include "Character/CharacterTitle.h"

#include "Data/GameConfig/GameConfig.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "I18N/All.h"

#include <cwchar>

namespace CharacterTitle
{
namespace
{
    constexpr int kAutoId = 0;
    constexpr int kHeroStateCount = 5;
    constexpr int kGensRankCount = 14;

    wchar_t g_gensNames[kGensRankCount][32]{};
    std::vector<Rank> g_catalog;
    bool g_catalogReady = false;

    void SplitGensNames()
    {
        const wchar_t* src = I18N::Game::GrandDukeDukeMarquisCountViscount;
        int index = 0;
        wchar_t* write = g_gensNames[0];
        const wchar_t* end = g_gensNames[0] + sizeof(g_gensNames) / sizeof(wchar_t);

        for (const wchar_t* p = src; *p && index < kGensRankCount; ++p)
        {
            if (*p == L'#')
            {
                *write = L'\0';
                ++index;
                write = g_gensNames[index];
                continue;
            }

            if (write + 1 < end)
            {
                *write++ = *p;
            }
        }

        if (index < kGensRankCount)
        {
            *write = L'\0';
        }
    }

    void EnsureCatalog()
    {
        if (g_catalogReady)
        {
            return;
        }

        SplitGensNames();
        g_catalog.clear();
        g_catalog.reserve(1 + kHeroStateCount + kGensRankCount);

        g_catalog.push_back({ kAutoId, I18N::Game::Commoner });
        g_catalog.push_back({ 1, I18N::Game::Hero });
        g_catalog.push_back({ 2, I18N::Game::Commoner });
        g_catalog.push_back({ 3, I18N::Game::OutlawWarning });
        g_catalog.push_back({ 4, I18N::Game::_1stStageOutlaw });
        g_catalog.push_back({ 5, I18N::Game::_2ndStageOutlaw });

        for (int i = 0; i < kGensRankCount; ++i)
        {
            g_catalog.push_back({ 6 + i, g_gensNames[i] });
        }

        g_catalogReady = true;
    }

    const wchar_t* NameForId(int id)
    {
        EnsureCatalog();
        for (const auto& rank : g_catalog)
        {
            if (rank.Id == id)
            {
                return rank.Name;
            }
        }

        return I18N::Game::Commoner;
    }
}

const std::vector<Rank>& Catalog()
{
    EnsureCatalog();
    return g_catalog;
}

int SelectedId()
{
    if (Hero == nullptr || Hero->ID[0] == L'\0')
    {
        return kAutoId;
    }

    return GameConfig::GetInstance().GetCharacterTitleId(Hero->ID);
}

void Select(int id)
{
    if (Hero == nullptr || Hero->ID[0] == L'\0')
    {
        return;
    }

    EnsureCatalog();
    bool known = false;
    for (const auto& rank : g_catalog)
    {
        if (rank.Id == id)
        {
            known = true;
            break;
        }
    }

    if (!known)
    {
        id = kAutoId;
    }

    GameConfig::GetInstance().SetCharacterTitleId(Hero->ID, id);
}

const wchar_t* FromHeroState(BYTE pk)
{
    switch (pk)
    {
    case 1:
    case 2:
        return I18N::Game::Hero;
    case 4:
        return I18N::Game::OutlawWarning;
    case 5:
        return I18N::Game::_1stStageOutlaw;
    case 6:
        return I18N::Game::_2ndStageOutlaw;
    default:
        return I18N::Game::Commoner;
    }
}

void Fill(CHARACTER* owner, wchar_t* dest, size_t destChars)
{
    if (dest == nullptr || destChars == 0)
    {
        return;
    }

    dest[0] = L'\0';
    if (owner == nullptr || owner->Object.Kind != KIND_PLAYER)
    {
        return;
    }

    const wchar_t* title = FromHeroState(owner->PK);
    if (owner == Hero)
    {
        const int id = SelectedId();
        if (id != kAutoId)
        {
            title = NameForId(id);
        }
    }

    if (title == nullptr || title[0] == L'\0')
    {
        return;
    }

    wcsncpy_s(dest, destChars, title, _TRUNCATE);
}
}
