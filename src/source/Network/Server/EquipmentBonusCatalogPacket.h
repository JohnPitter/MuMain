#pragma once

#include <optional>
#include <span>
#include "Character/EquipmentBonusCatalog.h"

namespace Network::Equipment
{
    constexpr std::uint8_t CatalogSubCode = 0xE7;
    std::optional<Character::Equipment::BonusCatalog> DecodeCatalog(std::span<const std::uint8_t> packet);
}
