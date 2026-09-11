#include "Network/Server/EquipmentBonusCatalogPacket.h"

#include <bit>
#include <cmath>

namespace Network::Equipment
{
    namespace
    {
        constexpr std::size_t HeaderSize = 9;
        constexpr std::size_t MemberSize = 3;
        constexpr std::size_t BonusSize = 6;
        constexpr std::uint16_t MaximumItemType = 8191;
        constexpr float MaximumDisplayValue = 1'000'000;

        bool ReadMembers(std::span<const std::uint8_t> data, Character::Equipment::BonusCatalog& catalog)
        {
            unsigned quantity = 0;
            for (std::size_t index = 0; index < catalog.MemberCount; ++index)
            {
                const auto member = data.subspan(index * MemberSize, MemberSize);
                const auto type = static_cast<std::uint16_t>(member[0] | (member[1] << 8));
                if (type > MaximumItemType || member[2] == 0)
                    return false;
                for (std::size_t previous = 0; previous < index; ++previous)
                    if (catalog.Members[previous].ItemType == type)
                        return false;
                catalog.Members[index] = { type, member[2] };
                quantity += member[2];
            }
            return quantity == catalog.RequiredItems;
        }

        bool ReadBonuses(std::span<const std::uint8_t> data, Character::Equipment::BonusCatalog& catalog)
        {
            using namespace Character::Equipment;
            for (std::size_t index = 0; index < catalog.BonusCount; ++index)
            {
                const auto row = data.subspan(index * BonusSize, BonusSize);
                if (row[0] < static_cast<unsigned>(BonusKind::Damage)
                    || row[0] > static_cast<unsigned>(BonusKind::ElementalProtection)
                    || row[1] > static_cast<unsigned>(BonusUnit::PercentagePoints))
                    return false;
                const auto bits = static_cast<std::uint32_t>(row[2]) | (static_cast<std::uint32_t>(row[3]) << 8)
                    | (static_cast<std::uint32_t>(row[4]) << 16) | (static_cast<std::uint32_t>(row[5]) << 24);
                const float value = std::bit_cast<float>(bits);
                if (!std::isfinite(value) || std::abs(value) > MaximumDisplayValue)
                    return false;
                catalog.Bonuses[index] = { static_cast<BonusKind>(row[0]), static_cast<BonusUnit>(row[1]), value };
            }
            return true;
        }
    }

    std::optional<Character::Equipment::BonusCatalog> DecodeCatalog(std::span<const std::uint8_t> packet)
    {
        using namespace Character::Equipment;
        if (packet.size() < HeaderSize || packet[0] != 0xC1 || packet[1] != packet.size()
            || packet[2] != 0xF3 || packet[3] != CatalogSubCode || packet[4] != 1
            || packet[6] > MaximumCatalogEntries || packet[7] > MaximumCatalogEntries || packet[8] > 15)
            return std::nullopt;

        BonusCatalog catalog;
        catalog.Known = true;
        catalog.RequiredItems = packet[5];
        catalog.MemberCount = packet[6];
        catalog.BonusCount = packet[7];
        catalog.MinimumUpgrade = packet[8];
        const auto memberBytes = MemberSize * catalog.MemberCount;
        if (packet.size() != HeaderSize + memberBytes + BonusSize * catalog.BonusCount
            || (catalog.MemberCount == 0) != (catalog.BonusCount == 0)
            || !ReadMembers(packet.subspan(HeaderSize, memberBytes), catalog)
            || !ReadBonuses(packet.subspan(HeaderSize + memberBytes), catalog))
            return std::nullopt;
        return catalog;
    }
}
