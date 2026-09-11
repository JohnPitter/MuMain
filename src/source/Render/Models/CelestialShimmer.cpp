#include "Render/Models/CelestialShimmer.h"

#include <algorithm>

namespace Render::Items::Celestial
{
    Shimmer MaterialShimmer(std::string_view texture, int level, float pulse, int detail)
    {
        if (detail <= 0)
            return {};
        constexpr float MaximumUpgrade = 15;
        const float upgrade = std::clamp(static_cast<float>(level), 0.f, MaximumUpgrade) / MaximumUpgrade;
        const float wave = std::clamp(pulse, 0.f, 1.f);
        if (texture == "Celestial_Gold.jpg")
            return { { 1.f, 0.78f, 0.36f }, 0.42f + upgrade * 0.14f + wave * 0.04f, false };
        if (detail < 2)
            return {};
        if (texture == "Celestial_Ivory.jpg")
            return { { 1.f, 0.96f, 0.82f }, 0.06f + upgrade * 0.03f + wave * 0.02f, false };
        if (texture == "Celestial_Sapphire.jpg")
            return { { 0.45f, 0.78f, 1.f }, 0.25f + wave * 0.08f, true };
        if (texture == "Celestial_Emissive.jpg")
            return { { 1.f, 0.84f, 0.46f }, 0.26f + upgrade * 0.06f + wave * 0.06f, true };
        return {};
    }
}
