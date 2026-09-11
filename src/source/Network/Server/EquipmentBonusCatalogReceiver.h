#pragma once

#include <cstdint>
#include <span>

namespace Network::Equipment
{
    void ReceiveCatalog(std::span<const std::uint8_t> packet);
}
