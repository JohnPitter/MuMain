#include "stdafx.h"

#include "Character/VipStatus.h"

#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"

namespace VipStatus
{
    void ReceiveStatus(const BYTE* buffer, int32_t size)
    {
        if (buffer == nullptr || size < 7)
        {
            return;
        }

        // Matches CharacterTitle::ReceiveAppearance: packet type C1/C3 (odd)
        // has a 1-byte length field, C2/C4 (even) has 2 bytes, shifting every
        // field after the header by one.
        const int header = (buffer[0] % 2 == 1) ? 0 : 1;
        if (size < 7 + header)
        {
            return;
        }

        const int key = (static_cast<int>(buffer[4 + header]) << 8) + buffer[5 + header];
        const bool isVip = buffer[6 + header] != 0;

        const int index = FindCharacterIndex(key);
        if (index >= MAX_CHARACTERS_CLIENT)
        {
            return;
        }

        CharactersClient[index].IsVip = isVip;
    }
}
