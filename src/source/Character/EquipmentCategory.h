#pragma once

#include <cstdint>

namespace Character::Equipment
{
    // Intrinsic set categories sent by the server in the E7 v2 catalog.
    // Poseidon (2) and Zeus (3) share the Ultimate rules (authored set with
    // intrinsic category, level/class requirement) but render with their own
    // identity.
    enum class Category : std::uint8_t { Normal, Ultimate, Poseidon, Zeus };
}
