#include "stdafx.h"
#include "doctest.h"
#include "Engine/Object/ZzzCharacter.h"
#include "GameLogic/Pets/GIPetManager.h"
#include <memory>

TEST_CASE("Fresh character slots can be cleared before the login scene without pets")
{
    constexpr int AllocationPadding = 129;
    auto slots = std::make_unique<CHARACTER[]>(MAX_CHARACTERS_CLIENT + AllocationPadding);
    auto* previousSlots = CharactersClient;
    CharactersClient = slots.get() + AllocationPadding - 1;
    for (int pass = 0; pass < 3; ++pass)
    {
        ClearCharacters(-1);
        for (int index = 0; index < MAX_CHARACTERS_CLIENT; ++index)
        {
            CHECK_FALSE(CharactersClient[index].Object.Live);
            CHECK(CharactersClient[index].m_pPet == nullptr);
            CHECK(CharactersClient[index].m_pParts == nullptr);
            CHECK_FALSE(CharactersClient[index].ServerEquipment.Known);
        }
    }
    CharactersClient = previousSlots;
}
