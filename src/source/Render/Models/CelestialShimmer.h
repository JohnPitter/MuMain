#pragma once

#include <array>
#include <string_view>

namespace Render::Items::Celestial
{
    struct Shimmer
    {
        std::array<float, 3> Color{};
        float Strength = 0;
        bool Emissive = false;
    };

    Shimmer MaterialShimmer(std::string_view texture, int level, float pulse, int detail);
}
