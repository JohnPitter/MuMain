#include "Render/Models/CelestialShimmer.h"

#include <algorithm>
#include <cmath>

namespace Render::Items::Celestial
{
    namespace
    {
        constexpr float MaximumUpgrade = 15;

        float UnitValue(float value)
        {
            return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f;
        }
    }

    Shimmer MaterialShimmer(std::string_view texture, int level, float pulse, int detail)
    {
        if (detail <= 0)
            return {};
        const float upgrade = std::clamp(static_cast<float>(level), 0.f, MaximumUpgrade) / MaximumUpgrade;
        const float wave = UnitValue(pulse);
        if (texture == "Celestial_Gold.jpg")
            return { { 1.f, 0.62f, 0.12f }, 0.16f + upgrade * 0.08f + wave * 0.025f, false };
        if (detail < 2)
            return {};
        if (texture == "Celestial_Ivory.jpg")
            return { { 1.f, 0.98f, 0.93f }, 0.025f + upgrade * 0.015f + wave * 0.01f, false };
        if (texture == "Celestial_Sapphire.jpg")
            return { { 0.20f, 0.52f, 1.f }, 0.08f + wave * 0.05f, true };
        if (texture == "Celestial_Emissive.jpg")
            return { { 1.f, 0.56f, 0.08f }, 0.12f + upgrade * 0.05f + wave * 0.03f, true };
        return {};
    }

    std::array<float, 3> AdditiveLight(const Shimmer& shimmer, float alpha)
    {
        const float intensity = UnitValue(shimmer.Strength) * UnitValue(alpha);
        return { UnitValue(shimmer.Color[0]) * intensity,
            UnitValue(shimmer.Color[1]) * intensity, UnitValue(shimmer.Color[2]) * intensity };
    }

    std::array<float, 3> HaloLight(float pulse, float alpha, int detail)
    {
        if (detail <= 0)
            return {};
        const float intensity = (0.7f + UnitValue(pulse) * 0.3f) * UnitValue(alpha);
        return { 0.4f * intensity, 0.22f * intensity, 0.035f * intensity };
    }
}
