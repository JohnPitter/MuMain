#include "Render/Models/CelestialShimmer.h"

#include <algorithm>
#include <cmath>

namespace Render::Items::Celestial
{
    namespace
    {
        constexpr float MaximumUpgrade = 15;
        constexpr int FullMaterialDetail = 2;
        constexpr std::array<float, 3> GoldenMonsterTint = { 1.f, 0.5f, 0.f };
        constexpr std::array<float, 3> PlatinumReflectionTint = { 1.f, 1.f, 1.f };
        constexpr float GoldMetalBase = 0.90f;
        constexpr float GoldMetalUpgrade = 0.10f;
        constexpr float GoldChromeBase = 0.70f;
        constexpr float GoldChromeUpgrade = 0.30f;
        constexpr float PlatinumSweepBase = 0.65f;
        constexpr float PlatinumSweepUpgrade = 0.25f;
        constexpr float PlatinumChromeBase = 0.25f;
        constexpr float PlatinumChromeUpgrade = 0.10f;

        float UnitValue(float value)
        {
            return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f;
        }

        MaterialPasses GoldenReflections(float upgrade, int detail)
        {
            const float chrome = detail >= FullMaterialDetail
                ? GoldChromeBase + upgrade * GoldChromeUpgrade : 0.f;
            return {{ { GoldenMonsterTint, GoldMetalBase + upgrade * GoldMetalUpgrade, ShimmerSurface::Metal },
                { GoldenMonsterTint, chrome, ShimmerSurface::Chrome } }};
        }

        MaterialPasses PlatinumReflections(float upgrade)
        {
            return {{ { PlatinumReflectionTint, PlatinumSweepBase + upgrade * PlatinumSweepUpgrade, ShimmerSurface::Chrome4 },
                { PlatinumReflectionTint, PlatinumChromeBase + upgrade * PlatinumChromeUpgrade, ShimmerSurface::Chrome } }};
        }
    }

    MaterialPasses MaterialShimmer(std::string_view texture, int level, float pulse, int detail)
    {
        if (detail <= 0)
            return {};
        const float upgrade = std::clamp(static_cast<float>(level), 0.f, MaximumUpgrade) / MaximumUpgrade;
        const float wave = UnitValue(pulse);
        if (texture == "Celestial_Gold.jpg")
            return GoldenReflections(upgrade, detail);
        if (detail < FullMaterialDetail)
            return {};
        if (texture == "Celestial_Ivory.jpg")
            return PlatinumReflections(upgrade);
        if (texture == "Celestial_Sapphire.jpg")
            return {{ { { 0.20f, 0.52f, 1.f }, 0.08f + wave * 0.05f, ShimmerSurface::Emissive }, {} }};
        if (texture == "Celestial_Emissive.jpg")
            return {{ { { 1.f, 0.56f, 0.08f }, 0.12f + upgrade * 0.05f + wave * 0.03f, ShimmerSurface::Emissive }, {} }};
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
