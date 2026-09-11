#include "doctest.h"
#include "Render/Models/CelestialShimmer.h"

#include <limits>

using Render::Items::Celestial::AdditiveLight;
using Render::Items::Celestial::HaloLight;
using Render::Items::Celestial::MaterialShimmer;

TEST_CASE("Ultimate gold shines natively at zero upgrade without excellent flags")
{
    const auto base = MaterialShimmer("Celestial_Gold.jpg", 0, 0, 2);
    CHECK(base.Strength >= 0.15f);
    CHECK_FALSE(base.Emissive);
    CHECK(base.Color[0] > base.Color[1]);
    CHECK(MaterialShimmer("Celestial_Gold.jpg", 15, 1, 2).Strength > base.Strength);
    CHECK(MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 2).Strength <= 0.05f);
}

TEST_CASE("Ultimate shimmer is bounded and honors reduced effects")
{
    for (const auto name : { "Celestial_Gold.jpg", "Celestial_Ivory.jpg", "Celestial_Sapphire.jpg", "Celestial_Emissive.jpg" })
    {
        CHECK(MaterialShimmer(name, 15, 1, 0).Strength == 0);
        CHECK(MaterialShimmer(name, 999, 100, 4).Strength <= 0.266f);
        CHECK(MaterialShimmer(name, -1, -1, 4).Strength >= 0);
    }
    CHECK(MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 1).Strength == 0);
    CHECK(MaterialShimmer("Kundun.jpg", 15, 1, 4).Strength == 0);
    CHECK(MaterialShimmer("Celestial_Emissive.jpg", 0, 1, 2).Emissive);
}

TEST_CASE("Additive chrome and emissive passes premultiply intensity into RGB")
{
    for (const auto name : { "Celestial_Gold.jpg", "Celestial_Ivory.jpg", "Celestial_Sapphire.jpg", "Celestial_Emissive.jpg" })
    {
        const auto material = MaterialShimmer(name, 15, 1, 2);
        const auto full = AdditiveLight(material, 1);
        const auto faded = AdditiveLight(material, 0.25f);
        for (std::size_t channel = 0; channel < full.size(); ++channel)
        {
            CHECK(full[channel] == doctest::Approx(material.Color[channel] * material.Strength));
            CHECK(faded[channel] == doctest::Approx(full[channel] * 0.25f));
            CHECK(AdditiveLight(material, 0)[channel] == 0);
        }
    }
}

TEST_CASE("Golden base and white accents retain separate highlight energy budgets at plus fifteen")
{
    const auto gold = AdditiveLight(MaterialShimmer("Celestial_Gold.jpg", 15, 1, 2), 1);
    const auto trim = AdditiveLight(MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 2), 1);
    CHECK(gold[0] <= 0.266f);
    CHECK(gold[1] < gold[0] * 0.65f);
    CHECK(gold[2] < gold[0] * 0.15f);
    for (float channel : trim)
        CHECK(channel <= 0.05f);
    CHECK(gold[0] > trim[0] * 5);
}

TEST_CASE("Halo preserves golden hue while respecting fade and disabled effects")
{
    const auto halo = HaloLight(1, 1, 2);
    CHECK(halo[0] <= 0.4f);
    CHECK(halo[1] < halo[0] * 0.6f);
    CHECK(halo[2] < halo[0] * 0.1f);
    for (std::size_t channel = 0; channel < halo.size(); ++channel)
    {
        CHECK(HaloLight(1, 0.25f, 2)[channel] == doctest::Approx(halo[channel] * 0.25f));
        CHECK(HaloLight(1, 0, 2)[channel] == 0);
        CHECK(HaloLight(1, 1, 0)[channel] == 0);
    }
}

TEST_CASE("Non finite opacity and wave values cannot leak into additive render colors")
{
    const auto invalid = std::numeric_limits<float>::quiet_NaN();
    const auto material = MaterialShimmer("Celestial_Gold.jpg", 15, invalid, 2);
    CHECK(material.Strength == doctest::Approx(0.24f));
    for (std::size_t channel = 0; channel < material.Color.size(); ++channel)
    {
        CHECK(AdditiveLight(material, invalid)[channel] == 0);
        CHECK(AdditiveLight(material, -1)[channel] == 0);
        CHECK(HaloLight(1, invalid, 2)[channel] == 0);
    }
}
