#pragma once

#include <array>
#include <string_view>

namespace Render::Items::Celestial
{
    enum class ShimmerSurface
    {
        Metal,
        Chrome,
        Emissive,
    };

    struct Shimmer
    {
        std::array<float, 3> Color{};
        float Strength = 0;
        ShimmerSurface Surface = ShimmerSurface::Metal;
    };

    using MaterialPasses = std::array<Shimmer, 2>;

    MaterialPasses MaterialShimmer(std::string_view texture, int level, float pulse, int detail);
    std::array<float, 3> AdditiveLight(const Shimmer& shimmer, float alpha);
    std::array<float, 3> HaloLight(float pulse, float alpha, int detail);
}
