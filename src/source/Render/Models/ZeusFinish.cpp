#include "stdafx.h"
#include "Render/Models/ZeusFinish.h"

#include <algorithm>
#include <cmath>
#include "Render/Models/ZeusModels.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Effects/ZzzEffect.h"
#include "UI/NewUI/NewUISystem.h"

namespace Render::Items::Zeus
{
    namespace
    {
        constexpr float EffectMaximumDistance = 1200.f;
        constexpr float PulseSpeed = 0.002f;
        constexpr float MaximumUpgrade = 15.f;
        constexpr int FullMaterialDetail = 2;
        constexpr int UnlitTextureMesh = -2;

        // The celeste specular tint of the study: brighter than the native
        // (0, .5, 1) so the Zeus set reads apart from the Legendary while
        // staying inside the loader vocabulary. It tints the blue masses and
        // doubles as the storm-channel emission color (the emissive atlas was
        // authored with exactly this linear RGB).
        constexpr std::array<float, 3> CelesteSpecularTint = { 0.10f, 0.45f, 1.00f };
        // The approved white platina keeps a neutral tint: no gold, no blue
        // cast, and never a uniform white emission.
        constexpr std::array<float, 3> PlatinumReflectionTint = { 1.f, 1.f, 1.f };

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

        float UnitValue(float value)
        {
            return std::isfinite(value) ? std::clamp(value, 0.f, 1.f) : 0.f;
        }

        int SurfaceRenderFlags(FinishSurface surface)
        {
            switch (surface)
            {
            case FinishSurface::Metal: return RENDER_METAL;
            case FinishSurface::Chrome: return RENDER_CHROME;
            case FinishSurface::Chrome4: return RENDER_CHROME4;
            case FinishSurface::Emissive: return RENDER_TEXTURE;
            }
            return RENDER_TEXTURE;
        }

        int SurfaceTextureIndex(FinishSurface surface)
        {
            switch (surface)
            {
            case FinishSurface::Metal: return BITMAP_SHINY;
            case FinishSurface::Chrome: return BITMAP_CHROME;
            case FinishSurface::Chrome4: return BITMAP_CHROME2;
            case FinishSurface::Emissive: return -1;
            }
            return -1;
        }

        void RenderMaterialPass(BMD* model, int mesh, const FinishPass& pass, float alpha, float textureU, float textureV)
        {
            if (pass.Strength <= 0.f)
                return;
            const auto light = AdditiveLight(pass, alpha);
            VectorCopy(light.data(), model->BodyLight);
            glColor3fv(model->BodyLight);
            const bool emissive = pass.Surface == FinishSurface::Emissive;
            const int blendMesh = emissive ? UnlitTextureMesh : -1;
            model->RenderMesh(mesh, SurfaceRenderFlags(pass.Surface) | RENDER_BRIGHT,
                1.f, blendMesh, 1.f, emissive ? textureU : 0.f,
                emissive ? textureV : 0.f, SurfaceTextureIndex(pass.Surface));
        }
    }

    MaterialPasses MaterialFinish(std::string_view texture, int level, float pulse, int detail)
    {
        if (detail <= 0)
            return {};
        const float upgrade = std::clamp(static_cast<float>(level), 0.f, MaximumUpgrade) / MaximumUpgrade;
        const float wave = UnitValue(pulse);
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

    std::array<float, 3> AdditiveLight(const FinishPass& pass, float alpha)
    {
        const float intensity = UnitValue(pass.Strength) * UnitValue(alpha);
        return { UnitValue(pass.Color[0]) * intensity,
            UnitValue(pass.Color[1]) * intensity, UnitValue(pass.Color[2]) * intensity };
    }

    void RenderMaterialAccents(BMD* model, OBJECT* object, int level, float alpha)
    {
        const int detail = g_pOption->GetRenderLevel();
        if (detail <= 0 || object->Distance > EffectMaximumDistance || alpha <= 0.f
            || g_isCharacterBuff(object, eBuff_Cloaking))
            return;
        vec3_t originalLight;
        VectorCopy(model->BodyLight, originalLight);
        const float pulse = 0.5f + 0.5f * sinf(WorldTime * PulseSpeed);
        for (int mesh = 0; mesh < model->NumMeshs; ++mesh)
        {
            const auto script = model->Meshs[mesh].m_csTScript;
            if (mesh == object->HiddenMesh || (script != nullptr && script->getHiddenMesh()))
                continue;
            const auto passes = MaterialFinish(model->Textures[mesh].FileName, level, pulse, detail);
            for (const auto& pass : passes)
                RenderMaterialPass(model, mesh, pass, alpha,
                    object->BlendMeshTexCoordU, object->BlendMeshTexCoordV);
        }
        VectorCopy(originalLight, model->BodyLight);
        glColor3fv(originalLight);
    }
}
