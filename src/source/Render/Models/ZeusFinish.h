#pragma once

#include <array>
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
    MaterialPasses MaterialFinish(std::string_view texture, int level, float pulse, int detail);
    std::array<float, 3> AdditiveLight(const FinishPass& pass, float alpha);

    // Per-mesh accent passes over the base render; no halo, no ground glow.
    void RenderMaterialAccents(BMD* model, OBJECT* object, int level, float alpha);
}
