#include "stdafx.h"
#include "Render/Models/ZeusFinish.h"

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
        constexpr int UnlitTextureMesh = -2;

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
