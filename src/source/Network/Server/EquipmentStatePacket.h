#pragma once

#include <optional>
#include <span>
#include "Network/Server/EquipmentStateUpdate.h"

namespace Network::Equipment
{
    constexpr std::uint8_t StateSubCode = 0xE8;
    constexpr std::size_t StatePacketSize = 16;
    constexpr std::size_t PhasedStatePacketSize = 17;
    std::optional<StateUpdate> DecodeState(std::span<const std::uint8_t> packet);
}
