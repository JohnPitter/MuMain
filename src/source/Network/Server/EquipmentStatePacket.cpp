#include "Network/Server/EquipmentStatePacket.h"

#include <bit>
#include <cmath>

namespace Network::Equipment
{
    namespace
    {
        constexpr std::uint8_t Header = 0xC1;
        constexpr std::uint8_t Group = 0xF3;
        constexpr std::uint8_t Version = 1;
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
        if (packet.size() != StatePacketSize || packet[0] != Header || packet[1] != StatePacketSize
            || packet[2] != Group || packet[3] != StateSubCode || packet[4] != Version)
            return std::nullopt;

        const auto flags = packet[7];
        const float factor = ReadLittleFloat(packet.subspan(8, sizeof(float)));
        if ((flags & ~CelestialFlag) != 0 || !std::isfinite(factor) || factor <= 0)
            return std::nullopt;

        StateUpdate update;
        update.CharacterId = static_cast<std::uint16_t>((packet[5] << ByteBits) | packet[6]);
        update.Equipment = { true, (flags & CelestialFlag) != 0, factor };
        update.AttackSpeed = ReadLittleShort(packet.subspan(12, sizeof(std::uint16_t)));
        update.MagicSpeed = ReadLittleShort(packet.subspan(14, sizeof(std::uint16_t)));
        return update;
    }
}
