#pragma once

#include <cstdint>
#include <span>

namespace Network::Equipment
{
    void ReceiveState(std::span<const std::uint8_t> packet);
}
