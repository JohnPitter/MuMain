#include "stdafx.h"
#include "Network/Server/EquipmentBonusCatalogReceiver.h"

#include "Character/EquipmentCatalogCache.h"
#include "Network/Server/EquipmentBonusCatalogPacket.h"

namespace Network::Equipment
{
    void ReceiveCatalog(std::span<const std::uint8_t> packet)
    {
        const auto catalog = DecodeCatalog(packet);
        if (!catalog)
        {
            g_ConsoleDebug->Write(MCD_ERROR, L"[EquipmentCatalog] Invalid F3 E7 packet (%zu bytes)", packet.size());
            return;
        }
        Character::Equipment::SetCatalog(*catalog);
    }
}
