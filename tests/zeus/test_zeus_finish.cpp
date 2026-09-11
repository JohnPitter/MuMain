#include "doctest.h"
#include "Render/Models/ZeusFinish.h"

#include <limits>

using Render::Items::Zeus::AdditiveLight;
using Render::Items::Zeus::FinishSurface;
using Render::Items::Zeus::MaterialFinish;
using Render::Items::Zeus::MaterialPasses;

namespace
{
    // The study's celeste specular tint (research/legendary-excellent-blue.md).
    constexpr std::array<float, 3> CelesteTint{ 0.10f, 0.45f, 1.00f };

    bool SameTint(const std::array<float, 3>& color)
    {
        return color[0] == CelesteTint[0] && color[1] == CelesteTint[1] && color[2] == CelesteTint[2];
    }
}

TEST_CASE("The blue masses carry the celeste specular tint at every upgrade")
{
    for (int level = 0; level <= 15; ++level)
    {
        const float upgrade = static_cast<float>(level) / 15.f;
        const auto blue = MaterialFinish("Zeus_Blue.jpg", level, 0.5f, 2);
        CHECK(blue[0].Surface == FinishSurface::Metal);
        CHECK(blue[1].Surface == FinishSurface::Chrome);
        CHECK(blue[0].Strength == doctest::Approx(0.9f + upgrade * 0.1f));
        CHECK(blue[1].Strength == doctest::Approx(0.7f + upgrade * 0.3f));
        for (const auto& pass : blue)
            CHECK(SameTint(pass.Color));
    }
}

TEST_CASE("The approved platina keeps a neutral tint without any gold or blue cast")
{
    const auto platinum = MaterialFinish("Zeus_Platina.jpg", 15, 1, 2);
    CHECK(platinum[0].Surface == FinishSurface::Chrome4);
    CHECK(platinum[1].Surface == FinishSurface::Chrome);
    CHECK(platinum[0].Strength == doctest::Approx(0.9f));
    CHECK(platinum[1].Strength == doctest::Approx(0.35f));
    for (const auto& pass : platinum)
    {
        for (std::size_t channel = 0; channel < pass.Color.size(); ++channel)
            CHECK(pass.Color[channel] == 1.f);
        const auto light = AdditiveLight(pass, 1);
        // Neutral tint: every channel identical, scaled only by the strength.
        CHECK(light[0] == doctest::Approx(light[1]));
        CHECK(light[1] == doctest::Approx(light[2]));
    }
}

TEST_CASE("Storm channels emit faint celeste light and never a white veil")
{
    const auto storm = MaterialFinish("Zeus_Emissive.jpg", 15, 1, 2);
    CHECK(storm[0].Surface == FinishSurface::Emissive);
    CHECK(storm[0].Strength == doctest::Approx(0.13f));
    CHECK(storm[1].Strength == 0.f);
    CHECK(SameTint(storm[0].Color));
    const auto still = MaterialFinish("Zeus_Emissive.jpg", 15, 0, 2);
    CHECK(still[0].Strength == doctest::Approx(0.08f));
}

TEST_CASE("Reduced effect quality keeps only the blue specular and quality zero turns everything off")
{
    const auto lowDetail = MaterialFinish("Zeus_Blue.jpg", 15, 1, 1);
    CHECK(lowDetail[0].Strength > 0.f);
    CHECK(lowDetail[1].Strength == 0.f);
    for (const auto name : { "Zeus_Platina.jpg", "Zeus_Emissive.jpg" })
        for (const auto& pass : MaterialFinish(name, 15, 1, 1))
            CHECK(pass.Strength == 0.f);
    for (const auto name : { "Zeus_Blue.jpg", "Zeus_Platina.jpg", "Zeus_Emissive.jpg" })
        for (const auto& pass : MaterialFinish(name, 15, 1, 0))
            CHECK(pass.Strength == 0.f);
}

TEST_CASE("Only authored Zeus material names receive the finish")
{
    for (const auto name : { "Kundun.jpg", "Celestial_Gold.jpg", "Celestial_Ivory.jpg",
        "Poseidon_Gold.jpg", "Poseidon_Black.jpg", "Poseidon_Pearl.jpg", "Robe01.jpg",
        "Zeus_Blue.jpg.other", "" })
    {
        for (const auto& pass : MaterialFinish(name, 15, 1, 4))
            CHECK(pass.Strength == 0.f);
    }
}

TEST_CASE("Accent passes premultiply their fade into RGB (GL_ONE, GL_ONE budget)")
{
    for (const auto name : { "Zeus_Blue.jpg", "Zeus_Platina.jpg", "Zeus_Emissive.jpg" })
    {
        for (const auto& pass : MaterialFinish(name, 15, 1, 2))
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

TEST_CASE("Non finite opacity and pulse cannot leak into render colors")
{
    const auto invalid = std::numeric_limits<float>::quiet_NaN();
    const auto infinite = std::numeric_limits<float>::infinity();
    const auto blue = MaterialFinish("Zeus_Blue.jpg", 15, invalid, 2);
    const auto storm = MaterialFinish("Zeus_Emissive.jpg", invalid, invalid, 2);
    CHECK(blue[0].Strength == doctest::Approx(1.f));
    CHECK(storm[0].Strength == doctest::Approx(0.08f));
    for (std::size_t channel = 0; channel < blue[0].Color.size(); ++channel)
    {
        CHECK(AdditiveLight(blue[0], invalid)[channel] == 0);
        CHECK(AdditiveLight(blue[0], infinite)[channel] == 0);
        CHECK(AdditiveLight(blue[0], -1)[channel] == 0);
    }
    const MaterialPasses clamped = MaterialFinish("Zeus_Blue.jpg", 999, 2, 4);
    for (const auto& pass : clamped)
        CHECK(pass.Strength <= 1.f);
}
