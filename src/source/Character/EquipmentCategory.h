#pragma once

#include <cstdint>

namespace Character::Equipment
{
    // Intrinsic set categories sent by the server in the E7 v2 catalog.
    // Poseidon (2) shares the Ultimate rules (authored set with intrinsic
    // category, level/class requirement) but renders with its own identity.
    enum class Category : std::uint8_t { Normal, Ultimate, Poseidon };
}
