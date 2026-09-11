#include "Network/Server/EquipmentStatePacket.h"

#include <bit>
#include <cmath>

namespace Network::Equipment
{
    namespace
    {
        constexpr std::uint8_t Header = 0xC1;
        constexpr std::uint8_t Group = 0xF3;
        constexpr std::uint8_t LegacyVersion = 1;
        constexpr std::uint8_t PhasedVersion = 2;
        constexpr std::uint8_t CelestialFlag = 1;
        constexpr unsigned ByteBits = 8;

        std::uint16_t ReadLittleShort(std::span<const std::uint8_t> bytes)
        {
            return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << ByteBits));
        }

        float ReadLittleFloat(std::span<const std::uint8_t> bytes)
        {
            std::uint32_t bits = 0;
            for (unsigned index = 0; index < sizeof(bits); ++index)
                bits |= static_cast<std::uint32_t>(bytes[index]) << (index * ByteBits);
            return std::bit_cast<float>(bits);
        }
    }

    std::optional<StateUpdate> DecodeState(std::span<const std::uint8_t> packet)
    {
        if (packet.size() < StatePacketSize || packet[0] != Header || packet[1] != packet.size()
            || packet[2] != Group || packet[3] != StateSubCode)
            return std::nullopt;

        const bool legacy = packet[4] == LegacyVersion && packet.size() == StatePacketSize;
        const bool phased = packet[4] == PhasedVersion && packet.size() == PhasedStatePacketSize;
        if (!legacy && !phased)
            return std::nullopt;

        const auto flags = packet[7];
        const float factor = ReadLittleFloat(packet.subspan(8, sizeof(float)));
        if ((flags & ~CelestialFlag) != 0 || !std::isfinite(factor) || factor <= 0)
            return std::nullopt;

        const bool active = (flags & CelestialFlag) != 0;
        const std::uint8_t percent = phased ? packet[16] : (active ? 100 : 0);
        if (percent > 100 || (percent > 0) != active)
            return std::nullopt;

        StateUpdate update;
        update.CharacterId = static_cast<std::uint16_t>((packet[5] << ByteBits) | packet[6]);
        update.Equipment = { true, active, factor, percent };
        update.AttackSpeed = ReadLittleShort(packet.subspan(12, sizeof(std::uint16_t)));
        update.MagicSpeed = ReadLittleShort(packet.subspan(14, sizeof(std::uint16_t)));
        return update;
    }
}
