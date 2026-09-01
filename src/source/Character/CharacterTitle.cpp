#include "stdafx.h"

#include "Character/CharacterTitle.h"

#include "Dotnet/Connection.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "I18N/All.h"

extern int CurrentProtocolState;
#ifndef RECEIVE_JOIN_MAP_SERVER
#define RECEIVE_JOIN_MAP_SERVER 61
#endif

#include <cwchar>
#include <set>

extern Connection* SocketClient;

namespace CharacterTitle
{
namespace
{
    constexpr int kHeroStateCount = 5;
    constexpr int kGensRankCount = 14;

    wchar_t g_gensNames[kGensRankCount][32]{};
    std::vector<Rank> g_catalog;
    std::vector<Rank> g_visible;
    std::set<int> g_owned;
    int g_equipped = AutoId;
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

        g_catalog.push_back({ AutoId, I18N::Game::Commoner });
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

    void RebuildVisible()
    {
        EnsureCatalog();
        g_visible.clear();
        g_visible.reserve(1 + g_owned.size());
        for (const auto& rank : g_catalog)
        {
            if (rank.Id == AutoId || g_owned.count(rank.Id) != 0)
            {
                g_visible.push_back(rank);
            }
        }
    }

    bool IsKnown(int id)
    {
        EnsureCatalog();
        for (const auto& rank : g_catalog)
        {
            if (rank.Id == id)
            {
                return true;
            }
        }

        return false;
    }

    bool CanEquip(int id)
    {
        return id == AutoId || g_owned.count(id) != 0;
    }
}

const std::vector<Rank>& Catalog()
{
    EnsureCatalog();
    return g_catalog;
}

const std::vector<Rank>& Visible()
{
    EnsureCatalog();
    if (g_visible.empty())
    {
        RebuildVisible();
    }

    return g_visible;
}

int SelectedId()
{
    return g_equipped;
}

void Select(int id)
{
    if (!IsKnown(id) || !CanEquip(id))
    {
        id = AutoId;
    }

    g_equipped = id;
    if (Hero != nullptr)
    {
        Hero->CosmeticTitleId = static_cast<BYTE>(id);
    }

    if (SocketClient == nullptr || CurrentProtocolState != RECEIVE_JOIN_MAP_SERVER)
    {
        return;
    }

    BYTE packet[5] = { 0xC1, 0x05, 0xF3, 0xEA, static_cast<BYTE>(id) };
    SocketClient->Send(packet, 5);
}

void Reset()
{
    g_owned.clear();
    g_equipped = AutoId;
    g_visible.clear();
}

void ReceiveOwned(const BYTE* buffer, int32_t size)
{
    if (buffer == nullptr || size < 6)
    {
        return;
    }

    const int header = (buffer[0] % 2 == 1) ? 0 : 1;
    if (size < 6 + header)
    {
        return;
    }

    g_equipped = buffer[4 + header];
    if (!IsKnown(g_equipped))
    {
        g_equipped = AutoId;
    }

    const int count = buffer[5 + header];
    g_owned.clear();
    for (int i = 0; i < count; ++i)
    {
        const int offset = 6 + header + i;
        if (offset >= size)
        {
            break;
        }

        const int id = buffer[offset];
        if (id >= 1 && id <= ScrollCount)
        {
            g_owned.insert(id);
        }
    }

    if (!CanEquip(g_equipped))
    {
        g_equipped = AutoId;
    }

    RebuildVisible();
    if (Hero != nullptr)
    {
        Hero->CosmeticTitleId = static_cast<BYTE>(g_equipped);
    }
}

void ReceiveAppearance(const BYTE* buffer, int32_t size)
{
    if (buffer == nullptr || size < 7)
    {
        return;
    }

    const int header = (buffer[0] % 2 == 1) ? 0 : 1;
    if (size < 7 + header)
    {
        return;
    }

    const int key = (static_cast<int>(buffer[4 + header]) << 8) + buffer[5 + header];
    const int titleId = buffer[6 + header];
    const int index = FindCharacterIndex(key);
    if (index >= MAX_CHARACTERS_CLIENT)
    {
        return;
    }

    CHARACTER* owner = &CharactersClient[index];
    owner->CosmeticTitleId = static_cast<BYTE>(IsKnown(titleId) ? titleId : AutoId);
    if (owner == Hero)
    {
        g_equipped = owner->CosmeticTitleId;
        if (!CanEquip(g_equipped))
        {
            g_equipped = AutoId;
            owner->CosmeticTitleId = AutoId;
        }
    }
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
    if (owner->CosmeticTitleId != AutoId)
    {
        title = NameForId(owner->CosmeticTitleId);
    }

    if (title == nullptr || title[0] == L'\0')
    {
        return;
    }

    wcsncpy_s(dest, destChars, title, _TRUNCATE);
}
}
