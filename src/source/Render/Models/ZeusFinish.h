#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

struct BMD;
struct OBJECT;

namespace Render::Items::Zeus
{
    enum class FinishSurface
    {
        Metal,
        Chrome,
        Chrome4,
        Emissive,
    };

    struct FinishPass
    {
        std::array<float, 3> Color{};
        float Strength = 0;
        FinishSurface Surface = FinishSurface::Metal;
    };

    using MaterialPasses = std::array<FinishPass, 2>;

    // Legendary-blue finish profile (research/legendary-excellent-blue.md):
    // the celeste specular tint lives only on the authored blue atlas, the
    // approved white platina keeps a neutral tint, and the storm-channel
    // atlas receives the faint blue emission. Every other texture name is
    // ignored, so no native or sibling-set material can pick the profile up.
    // Pure functions live in this header so tests exercise the exact profile
    // without the render runtime; RenderMaterialAccents stays in the .cpp.
    inline float UnitFinishValue(float value)
    {
        return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f;
    }

    // The celeste specular tint of the study: brighter than the native
    // (0, .5, 1) so the Zeus set reads apart from the Legendary while staying
    // inside the loader vocabulary. It tints the blue masses and doubles as
    // the storm-channel emission color (the emissive atlas was authored with
    // exactly this linear RGB).
    constexpr std::array<float, 3> CelesteSpecularTint = { 0.10f, 0.45f, 1.00f };
    // The approved white platina keeps a neutral tint: no gold, no blue cast,
    // and never a uniform white emission.
    constexpr std::array<float, 3> PlatinumReflectionTint = { 1.f, 1.f, 1.f };

    inline MaterialPasses MaterialFinish(std::string_view texture, int level, float pulse, int detail)
    {
        constexpr float MaximumUpgrade = 15.f;
        constexpr int FullMaterialDetail = 2;
        constexpr float BlueMetalBase = 0.90f;
        constexpr float BlueMetalUpgrade = 0.10f;
        constexpr float BlueChromeBase = 0.70f;
        constexpr float BlueChromeUpgrade = 0.30f;
        constexpr float PlatinumSweepBase = 0.65f;
        constexpr float PlatinumSweepUpgrade = 0.25f;
        constexpr float PlatinumChromeBase = 0.25f;
        constexpr float PlatinumChromeUpgrade = 0.10f;
        constexpr float StormEmissionBase = 0.08f;
        constexpr float StormEmissionWave = 0.05f;
        if (detail <= 0)
            return {};
        const float upgrade = std::clamp(static_cast<float>(level), 0.f, MaximumUpgrade) / MaximumUpgrade;
        const float wave = UnitFinishValue(pulse);
        if (texture == "Zeus_Blue.jpg")
        {
            const float chrome = detail >= FullMaterialDetail
                ? BlueChromeBase + upgrade * BlueChromeUpgrade : 0.f;
            return {{ { CelesteSpecularTint, BlueMetalBase + upgrade * BlueMetalUpgrade, FinishSurface::Metal },
                { CelesteSpecularTint, chrome, FinishSurface::Chrome } }};
        }
        if (detail < FullMaterialDetail)
            return {};
        if (texture == "Zeus_Platina.jpg")
        {
            return {{ { PlatinumReflectionTint, PlatinumSweepBase + upgrade * PlatinumSweepUpgrade, FinishSurface::Chrome4 },
                { PlatinumReflectionTint, PlatinumChromeBase + upgrade * PlatinumChromeUpgrade, FinishSurface::Chrome } }};
        }
        if (texture == "Zeus_Emissive.jpg")
            return {{ { CelesteSpecularTint, StormEmissionBase + wave * StormEmissionWave, FinishSurface::Emissive }, {} }};
        return {};
    }

    inline std::array<float, 3> AdditiveLight(const FinishPass& pass, float alpha)
    {
        const float intensity = UnitFinishValue(pass.Strength) * UnitFinishValue(alpha);
        return { UnitFinishValue(pass.Color[0]) * intensity,
            UnitFinishValue(pass.Color[1]) * intensity, UnitFinishValue(pass.Color[2]) * intensity };
    }

    // Per-mesh accent passes over the base render; no halo, no ground glow.
    void RenderMaterialAccents(BMD* model, OBJECT* object, int level, float alpha);
}
