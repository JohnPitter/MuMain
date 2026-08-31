// PartyManager.cpp: implementation of the CPartyManager class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GameLogic/Social/PartyManager.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInventory.h"
#include "Network/Server/WSclient.h"
#include "World/MapInfra/MapManager.h"

using namespace SEASON3B;

CPartyManager::CPartyManager()
{
}

CPartyManager::~CPartyManager()
{
    Release();
}

bool CPartyManager::Create()
{
    return true;
}

void CPartyManager::Release()
{
}

bool CPartyManager::Update()
{
    return true;
}

bool CPartyManager::Render()
{
    return true;
}

CPartyManager* CPartyManager::GetInstance()
{
    static CPartyManager sPartyManager;
    return &sPartyManager;
}

void CPartyManager::SearchPartyMember()
{
    for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
    {
        CHARACTER* c = &CharactersClient[i];
        OBJECT* o = &c->Object;
        if (o->Type == MODEL_PLAYER && o->Kind == KIND_PLAYER && o->Live && o->Visible && o->Alpha > 0.f && c->Dead == 0)
        {
            for (int j = 0; j < PartyNumber; ++j)
            {
                PARTY_t* p = &Party[j];

                if (p->index != -2) continue;
                if (p->index > -1) continue;

                if (NamesEqual(p->Name, c->ID))
                {
                    p->index = i;
                    break;
                }
            }
        }
    }

    for (int j = 0; j < PartyNumber; ++j)
    {
        PARTY_t* p = &Party[j];

        if (p->index >= 0) continue;

        if (NamesEqual(p->Name, Hero->ID))
        {
            p->index = -3;
        }
        else
        {
            p->index = -1;
        }
    }
}

bool CPartyManager::IsPartyActive()
{
    return PartyNumber > 1;
}

bool CPartyManager::IsPartyMember(int index)
{
    CHARACTER* c = &CharactersClient[index];
    return IsPartyMemberChar(c);
}

bool CPartyManager::IsPartyMemberChar(CHARACTER* c)
{
    for (int i = 0; i < PartyNumber; ++i)
    {
        if (NamesEqual(Party[i].Name, c->ID))
            return true;
    }

    return false;
}

CHARACTER* CPartyManager::GetPartyMemberChar(PARTY_t* pMember)
{
    if (pMember == nullptr || pMember->Name[0] == L'\0')
        return NULL;

    for (int i = 0; i < MAX_CHARACTERS_CLIENT; i++)
    {
        CHARACTER* c = &CharactersClient[i];
        if (c->Object.Live && NamesEqual(pMember->Name, c->ID))
            return c;
    }

    return NULL;
}

bool CPartyManager::NamesEqual(const wchar_t* left, const wchar_t* right)
{
    if (!left || !right)
        return false;

    while (*left == L' ')
        ++left;
    while (*right == L' ')
        ++right;

    size_t leftLen = wcslen(left);
    size_t rightLen = wcslen(right);
    while (leftLen > 0 && left[leftLen - 1] == L' ')
        --leftLen;
    while (rightLen > 0 && right[rightLen - 1] == L' ')
        --rightLen;

    if (leftLen != rightLen)
        return false;
    return wcsncmp(left, right, leftLen) == 0;
}

bool CPartyManager::IsLocalHero(const PARTY_t* member)
{
    return member && Hero && NamesEqual(member->Name, Hero->ID);
}

void CPartyManager::SyncLivePartyPositions()
{
    if (PartyNumber <= 0)
        return;

    SearchPartyMember();

    const BYTE currentMap = static_cast<BYTE>(gMapManager.WorldActive);
    for (int i = 0; i < PartyNumber; ++i)
    {
        PARTY_t* member = &Party[i];
        CHARACTER* character = nullptr;
        if (member->index >= 0 && member->index < MAX_CHARACTERS_CLIENT)
        {
            CHARACTER* candidate = &CharactersClient[member->index];
            if (candidate->Object.Live)
                character = candidate;
        }
        if (!character)
            character = GetPartyMemberChar(member);
        if (!character)
            continue;

        member->Map = currentMap;
        member->x = static_cast<BYTE>(character->PositionX);
        member->y = static_cast<BYTE>(character->PositionY);
    }
}

void CPartyManager::RequestPartyListIfDue()
{
    if (PartyNumber <= 0 || !SocketClient)
        return;

    static DWORD s_lastRequestTick = 0;
    const DWORD now = GetTickCount();
    if (s_lastRequestTick != 0 && (now - s_lastRequestTick) < 1000)
        return;

    s_lastRequestTick = now;
    SocketClient->ToGameServer()->SendPartyListRequest();
}