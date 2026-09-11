#include "stdafx.h"
#include "Network/Server/EquipmentStateReceiver.h"

#include "Network/Server/EquipmentStatePacket.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"

namespace Network::Equipment
{
    void ReceiveState(std::span<const std::uint8_t> packet)
    {
        const auto update = DecodeState(packet);
        if (!update)
        {
            g_ConsoleDebug->Write(MCD_ERROR, L"[EquipmentState] Invalid F3 E8 packet (%zu bytes)", packet.size());
            return;
        }

        const int index = FindCharacterIndex(update->CharacterId);
        if (index < 0 || index >= MAX_CHARACTERS_CLIENT)
            return;

        auto& character = CharactersClient[index];
        character.ServerEquipment = update->Equipment;
        character.AttackSpeed = update->AttackSpeed;
        character.MagicSpeed = update->MagicSpeed;
        if (&character == Hero)
        {
            CharacterAttribute->AttackSpeed = update->AttackSpeed;
            CharacterAttribute->MagicSpeed = update->MagicSpeed;
        }
    }
}
