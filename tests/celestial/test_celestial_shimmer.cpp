#include "doctest.h"
#include "Render/Models/CelestialShimmer.h"

#include <limits>

using Render::Items::Celestial::AdditiveLight;
using Render::Items::Celestial::HaloLight;
using Render::Items::Celestial::MaterialShimmer;
using Render::Items::Celestial::Shimmer;
using Render::Items::Celestial::ShimmerSurface;

TEST_CASE("Ultimate gold has native metal and moving chrome without excellent flags")
{
    const auto base = MaterialShimmer("Celestial_Gold.jpg", 0, 0, 2);
    CHECK(base[0].Surface == ShimmerSurface::Metal);
    CHECK(base[1].Surface == ShimmerSurface::Chrome);
    CHECK(base[0].Strength == doctest::Approx(0.9f));
    CHECK(base[1].Strength == doctest::Approx(0.7f));
    for (const auto& pass : base)
    {
        CHECK(pass.Color[0] == 1.f);
        CHECK(pass.Color[1] == 0.5f);
        CHECK(pass.Color[2] == 0.f);
    }
}

TEST_CASE("Plus fifteen gold matches both native Golden monster reflection passes")
{
    for (const auto& pass : MaterialShimmer("Celestial_Gold.jpg", 15, 1, 2))
    {
        CHECK(pass.Strength == doctest::Approx(1.f));
        const auto light = AdditiveLight(pass, 1);
        CHECK(light[0] == doctest::Approx(1.f));
        CHECK(light[1] == doctest::Approx(0.5f));
        CHECK(light[2] == 0.f);
    }
}

TEST_CASE("Golden reflections move by normal projection rather than pulsing a white veil")
{
    const auto first = MaterialShimmer("Celestial_Gold.jpg", 15, 0, 2);
    const auto second = MaterialShimmer("Celestial_Gold.jpg", 15, 1, 2);
    for (std::size_t pass = 0; pass < first.size(); ++pass)
    {
        CHECK(first[pass].Strength == second[pass].Strength);
        CHECK(first[pass].Color == second[pass].Color);
        CHECK(first[pass].Surface != ShimmerSurface::Emissive);
    }
}

TEST_CASE("Ultimate reflection levels are clamped and honor reduced effects")
{
    for (const auto name : { "Celestial_Gold.jpg", "Celestial_Ivory.jpg", "Celestial_Sapphire.jpg", "Celestial_Emissive.jpg" })
    {
        for (const auto& pass : MaterialShimmer(name, 15, 1, 0))
            CHECK(pass.Strength == 0);
        for (const auto& pass : MaterialShimmer(name, 999, 100, 4))
            CHECK(pass.Strength <= 1.f);
        for (const auto& pass : MaterialShimmer(name, -1, -1, 4))
            CHECK(pass.Strength >= 0);
    }
    CHECK(MaterialShimmer("Celestial_Gold.jpg", 15, 1, 1)[0].Strength > 0.f);
    CHECK(MaterialShimmer("Celestial_Gold.jpg", 15, 1, 1)[1].Strength == 0.f);
    CHECK(MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 1)[0].Strength == 0.f);
}

TEST_CASE("Only authored Celestial material names receive the finish")
{
    for (const auto name : { "Kundun.jpg", "icenight.jpg", "Celestial_Feathers.jpg", "", "Celestial_Gold.jpg.other" })
        for (const auto& pass : MaterialShimmer(name, 15, 1, 4))
            CHECK(pass.Strength == 0.f);
}

TEST_CASE("Additive reflection and emissive passes premultiply fade once into RGB")
{
    for (const auto name : { "Celestial_Gold.jpg", "Celestial_Ivory.jpg", "Celestial_Sapphire.jpg", "Celestial_Emissive.jpg" })
    {
        for (const auto& pass : MaterialShimmer(name, 15, 1, 2))
        {
            const auto full = AdditiveLight(pass, 1);
            const auto faded = AdditiveLight(pass, 0.25f);
            for (std::size_t channel = 0; channel < full.size(); ++channel)
            {
                CHECK(full[channel] == doctest::Approx(pass.Color[channel] * pass.Strength));
                CHECK(faded[channel] == doctest::Approx(full[channel] * 0.25f));
                CHECK(AdditiveLight(pass, 0)[channel] == 0);
            }
        }
    }
}

TEST_CASE("Steel accents keep lower reflection energy than gold and are not emissive")
{
    const auto steel = MaterialShimmer("Celestial_Ivory.jpg", 15, 1, 2);
    CHECK(steel[0].Surface == ShimmerSurface::Metal);
    CHECK(steel[1].Surface == ShimmerSurface::Chrome);
    const auto metal = AdditiveLight(steel[0], 1);
    const auto chrome = AdditiveLight(steel[1], 1);
    for (std::size_t channel = 0; channel < metal.size(); ++channel)
        CHECK(metal[channel] + chrome[channel] <= 0.333f);
    CHECK(metal[2] - metal[0] < 0.02f);
}

TEST_CASE("Gem and arcane emission remain confined to their original UV materials")
{
    const auto gem = MaterialShimmer("Celestial_Sapphire.jpg", 15, 1, 2);
    const auto arcane = MaterialShimmer("Celestial_Emissive.jpg", 15, 1, 2);
    CHECK(gem[0].Surface == ShimmerSurface::Emissive);
    CHECK(arcane[0].Surface == ShimmerSurface::Emissive);
    CHECK(gem[0].Strength <= 0.131f);
    CHECK(arcane[0].Strength <= 0.201f);
    CHECK(gem[1].Strength == 0);
    CHECK(arcane[1].Strength == 0);
}

TEST_CASE("Halo preserves golden hue without increasing its glow budget")
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

TEST_CASE("Non finite opacity and pulse cannot leak into render colors")
{
    const auto invalid = std::numeric_limits<float>::quiet_NaN();
    const auto infinite = std::numeric_limits<float>::infinity();
    const auto gold = MaterialShimmer("Celestial_Gold.jpg", 15, invalid, 2);
    const auto gem = MaterialShimmer("Celestial_Sapphire.jpg", 15, invalid, 2);
    CHECK(gold[0].Strength == doctest::Approx(1.f));
    CHECK(gem[0].Strength == doctest::Approx(0.08f));
    for (std::size_t channel = 0; channel < gold[0].Color.size(); ++channel)
    {
        CHECK(AdditiveLight(gold[0], invalid)[channel] == 0);
        CHECK(AdditiveLight(gold[0], infinite)[channel] == 0);
        CHECK(AdditiveLight(gold[0], -1)[channel] == 0);
        CHECK(HaloLight(1, invalid, 2)[channel] == 0);
    }
}

TEST_CASE("Invalid authored pass values cannot exceed the additive channel budget")
{
    const auto invalid = std::numeric_limits<float>::quiet_NaN();
    const Shimmer invalidColor = { { invalid, -1.f, 100.f }, 100.f, ShimmerSurface::Metal };
    const auto light = AdditiveLight(invalidColor, 100);
    CHECK(light[0] == 0.f);
    CHECK(light[1] == 0.f);
    CHECK(light[2] == 1.f);
    const Shimmer invalidStrength = { { 1.f, 1.f, 1.f }, invalid, ShimmerSurface::Chrome };
    for (float channel : AdditiveLight(invalidStrength, 1))
        CHECK(channel == 0.f);
}
