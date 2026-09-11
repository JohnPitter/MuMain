#include "doctest.h"
#include "Render/Models/CelestialShimmer.h"

using Render::Items::Celestial::MaterialShimmer;

TEST_CASE("Ultimate gold shines natively at zero upgrade without excellent flags")
{
    const auto base = MaterialShimmer("Celestial_Gold.jpg", 0, 0, 2);
    CHECK(base.Strength >= 0.4f);
    CHECK_FALSE(base.Emissive);
    CHECK(base.Color[0] > base.Color[1]);
    CHECK(MaterialShimmer("Celestial_Gold.jpg", 15, 1, 2).Strength > base.Strength);
    CHECK(MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 2).Strength < 0.12f);
}

TEST_CASE("Ultimate shimmer is bounded and honors reduced effects")
{
    for (const auto name : { "Celestial_Gold.jpg", "Celestial_Ivory.jpg", "Celestial_Sapphire.jpg", "Celestial_Emissive.jpg" })
    {
        CHECK(MaterialShimmer(name, 15, 1, 0).Strength == 0);
        CHECK(MaterialShimmer(name, 999, 100, 4).Strength <= 0.61f);
        CHECK(MaterialShimmer(name, -1, -1, 4).Strength >= 0);
    }
    CHECK(MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 1).Strength == 0);
    CHECK(MaterialShimmer("Kundun.jpg", 15, 1, 4).Strength == 0);
    CHECK(MaterialShimmer("Celestial_Emissive.jpg", 0, 1, 2).Emissive);
}
