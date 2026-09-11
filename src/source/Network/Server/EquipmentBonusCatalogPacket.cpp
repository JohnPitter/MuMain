#include "Network/Server/EquipmentBonusCatalogPacket.h"

#include <bit>
#include <cmath>

namespace Network::Equipment
{
    namespace
    {
        constexpr std::size_t LegacyHeaderSize = 9;
        constexpr std::size_t PhasedHeaderSize = 14;
        constexpr std::size_t MemberSize = 3;
        constexpr std::size_t BonusSize = 6;
        constexpr std::size_t PhaseSize = 3;
        constexpr std::uint16_t MaximumItemType = 8191;
        // Server class numbers (e.g. 17 = Lord Emperor); 0xFF is unset noise.
        constexpr std::uint8_t MaximumRequiredClass = 0x7F;
        constexpr float MaximumDisplayValue = 1'000'000;

        std::size_t ReadHeader(std::span<const std::uint8_t> packet, Character::Equipment::BonusCatalog& catalog)
        {
            using namespace Character::Equipment;
            if (packet.size() < LegacyHeaderSize || packet[0] != 0xC1 || packet[1] != packet.size()
                || packet[2] != 0xF3 || packet[3] != CatalogSubCode
                || packet[6] > MaximumCatalogEntries || packet[7] > MaximumCatalogEntries || packet[8] > 15)
                return 0;
            catalog.RequiredItems = packet[5];
            catalog.MemberCount = packet[6];
            catalog.BonusCount = packet[7];
            catalog.MinimumUpgrade = packet[8];
            if (packet[4] == 1)
                return LegacyHeaderSize;
            if (packet[4] != 2 || packet.size() < PhasedHeaderSize
                || packet[9] > static_cast<unsigned>(Category::Zeus) || packet[12] > MaximumRequiredClass
                || packet[13] > MaximumBonusPhases)
                return 0;
            catalog.ItemCategory = static_cast<Category>(packet[9]);
            catalog.RequiredLevel = static_cast<std::uint16_t>(packet[10] | (packet[11] << 8));
            catalog.RequiredClass = packet[12];
            catalog.PhaseCount = packet[13];
            if (catalog.MemberCount > 0 && catalog.PhaseCount == 0)
                return 0;
            if (catalog.MemberCount == 0 && (catalog.RequiredLevel != 0 || catalog.RequiredClass != 0
                || catalog.ItemCategory != Category::Normal || catalog.PhaseCount != 0))
                return 0;
            return PhasedHeaderSize;
        }

        bool ReadPhases(std::span<const std::uint8_t> data, Character::Equipment::BonusCatalog& catalog)
        {
            const auto allMembers = static_cast<std::uint16_t>((1u << catalog.MemberCount) - 1);
            Character::Equipment::BonusPhase previous;
            for (std::size_t index = 0; index < catalog.PhaseCount; ++index)
            {
                const auto row = data.subspan(index * PhaseSize, PhaseSize);
                const auto mask = static_cast<std::uint16_t>(row[0] | (row[1] << 8));
                if (mask == 0 || (mask & ~allMembers) != 0 || (mask & previous.MemberMask) != previous.MemberMask
                    || mask == previous.MemberMask || row[2] <= previous.Percent || row[2] > 100)
                    return false;
                previous = catalog.Phases[index] = { mask, row[2] };
            }
            return catalog.PhaseCount == 0 || (previous.MemberMask == allMembers && previous.Percent == 100);
        }

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
        BonusCatalog catalog;
        const auto headerSize = ReadHeader(packet, catalog);
        if (headerSize == 0)
            return std::nullopt;
        catalog.Known = true;
        const auto memberBytes = MemberSize * catalog.MemberCount;
        const auto bonusBytes = BonusSize * catalog.BonusCount;
        if (packet.size() != headerSize + memberBytes + bonusBytes + PhaseSize * catalog.PhaseCount
            || (catalog.MemberCount == 0) != (catalog.BonusCount == 0)
            || !ReadMembers(packet.subspan(headerSize, memberBytes), catalog)
            || !ReadBonuses(packet.subspan(headerSize + memberBytes, bonusBytes), catalog)
            || !ReadPhases(packet.subspan(headerSize + memberBytes + bonusBytes), catalog))
            return std::nullopt;
        return catalog;
    }
}
