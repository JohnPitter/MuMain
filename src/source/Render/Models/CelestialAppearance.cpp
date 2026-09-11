#include "stdafx.h"
#include "Render/Models/CelestialAppearance.h"

#include <cmath>
#include "Render/Models/CelestialModels.h"
#include "Render/Models/CelestialShimmer.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Effects/ZzzEffect.h"
#include "UI/NewUI/NewUISystem.h"

namespace Render::Items::Celestial
{
    namespace
    {
        constexpr float EffectMaximumDistance = 1200.f;
        constexpr float PulseSpeed = 0.002f;
        constexpr int UnlitTextureMesh = -2;

        struct MaterialRenderContext
        {
            float Alpha;
            float TextureU;
            float TextureV;
        };

        int SurfaceRenderFlags(ShimmerSurface surface)
        {
            switch (surface)
            {
            case ShimmerSurface::Metal: return RENDER_METAL;
            case ShimmerSurface::Chrome: return RENDER_CHROME;
            case ShimmerSurface::Chrome4: return RENDER_CHROME4;
            case ShimmerSurface::Emissive: return RENDER_TEXTURE;
            }
            return RENDER_TEXTURE;
        }

        int SurfaceTextureIndex(ShimmerSurface surface)
        {
            switch (surface)
            {
            case ShimmerSurface::Metal: return BITMAP_SHINY;
            case ShimmerSurface::Chrome: return BITMAP_CHROME;
            case ShimmerSurface::Chrome4: return BITMAP_CHROME2;
            case ShimmerSurface::Emissive: return -1;
            }
            return -1;
        }

        void RenderMaterialPass(BMD* model, int mesh, const Shimmer& shimmer, const MaterialRenderContext& context)
        {
            if (shimmer.Strength <= 0.f)
                return;
            const auto light = AdditiveLight(shimmer, context.Alpha);
            VectorCopy(light.data(), model->BodyLight);
            glColor3fv(model->BodyLight);
            const bool emissive = shimmer.Surface == ShimmerSurface::Emissive;
            const int blendMesh = emissive ? UnlitTextureMesh : -1;
            model->RenderMesh(mesh, SurfaceRenderFlags(shimmer.Surface) | RENDER_BRIGHT,
                1.f, blendMesh, 1.f, emissive ? context.TextureU : 0.f,
                emissive ? context.TextureV : 0.f, SurfaceTextureIndex(shimmer.Surface));
        }
    }

    void RenderWingSurface(BMD* model, OBJECT* object)
    {
        glColor3fv(model->BodyLight);
        model->RenderBody(RENDER_TEXTURE, object->Alpha, object->BlendMesh,
            object->BlendMeshLight, object->BlendMeshTexCoordU,
            object->BlendMeshTexCoordV, object->HiddenMesh);
    }

    void RenderHaloGlow(BMD* model, OBJECT* object)
    {
        if (model->NumBones <= WingHaloBone || g_pOption->GetRenderLevel() == 0
            || object->Distance > EffectMaximumDistance || object->Alpha <= 0.f
            || g_isCharacterBuff(object, eBuff_Cloaking))
            return;
        constexpr float RotationSpeed = 0.025f;
        constexpr float HaloScale = 0.7f;
        const float pulse = 0.5f + 0.5f * sinf(WorldTime * PulseSpeed);
        const auto tint = HaloLight(pulse, object->Alpha, g_pOption->GetRenderLevel());
        vec3_t origin = { 0.f, 0.f, 0.f };
        vec3_t position;
        vec3_t light;
        VectorCopy(tint.data(), light);
        model->TransformPosition(BoneTransform[WingHaloBone], origin, position, true);
        CreateSprite(BITMAP_SHINY + 1, position, HaloScale, light, object, WorldTime * RotationSpeed);
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
        const MaterialRenderContext context{ alpha, object->BlendMeshTexCoordU, object->BlendMeshTexCoordV };
        for (int mesh = 0; mesh < model->NumMeshs; ++mesh)
        {
            const auto script = model->Meshs[mesh].m_csTScript;
            if (mesh == object->HiddenMesh || (script != nullptr && script->getHiddenMesh()))
                continue;
            const auto passes = MaterialShimmer(model->Textures[mesh].FileName, level, pulse, detail);
            for (const auto& pass : passes)
                RenderMaterialPass(model, mesh, pass, context);
        }
        VectorCopy(originalLight, model->BodyLight);
        glColor3fv(originalLight);
    }
}
