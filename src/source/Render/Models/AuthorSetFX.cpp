#include "stdafx.h"
#include "Render/Models/AuthorSetFX.h"

#include <cmath>

#include "Core/Globals/_define.h"
#include "Core/Utilities/_GlobalFunctions.h"
#include "Engine/Object/PlayerActionState.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Models/PoseidonModels.h"
#include "Render/Models/ZeusModels.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/NewUI/NewUISystem.h"

namespace Render::Items::SetFX
{
namespace
{
    // Palettes taken straight from the two concept boards. Poseidon is
    // "Dourado Real" structure over "Azul Oceanico" runes with a pearl
    // highlight; Zeus is "Azul Celeste" over "Branco Platinado".
    struct Palette
    {
        float Primary[3];
        float Accent[3];
        float Highlight[3];
    };

    constexpr Palette PoseidonPalette = {
        { 1.00f, 0.80f, 0.32f },   // dourado real
        { 0.25f, 0.55f, 1.00f },   // azul oceanico
        { 0.95f, 0.97f, 1.00f },   // branco perolado
    };

    constexpr Palette ZeusPalette = {
        { 0.32f, 0.62f, 1.00f },   // azul celeste
        { 0.88f, 0.94f, 1.00f },   // branco platinado
        { 0.62f, 0.82f, 1.00f },   // realce da tempestade
    };

    const Palette& PaletteOf(Family family)
    {
        return family == Family::Zeus ? ZeusPalette : PoseidonPalette;
    }

    void ScaledLight(const float source[3], float intensity, vec3_t out)
    {
        Vector(source[0] * intensity, source[1] * intensity, source[2] * intensity, out);
    }

    // Breathing factor shared by every emitter so the whole set pulses as one.
    float Pulse()
    {
        return 0.65f + 0.35f * sinf(static_cast<float>(WorldTime) * 0.002f);
    }

    // `local` is not const: the native BMD transform takes a mutable vec_t[].
    bool ResolveBone(BMD* model, OBJECT* object, int bone, vec3_t local, vec3_t world)
    {
        if (model == nullptr || object->BoneTransform == nullptr || bone < 0 || model->NumBones <= bone)
        {
            return false;
        }

        model->TransformPosition(object->BoneTransform[bone], local, world, true);
        return true;
    }

    // The hand that actually carries an authored weapon; falls back to the
    // main hand so the strike still reads when only a classic weapon is held.
    int StrikeHand(const CHARACTER* character, Family family)
    {
        for (int hand = 0; hand < 2; ++hand)
        {
            const int type = character->Weapon[hand].Type;
            if (type < 0)
            {
                continue;
            }

            if ((family == Family::Poseidon && Poseidon::IsEquipment(type))
                || (family == Family::Zeus && Zeus::IsEquipment(type)))
            {
                return hand;
            }
        }

        return character->Weapon[0].Type >= 0 ? 0 : 1;
    }

    // "Aura do Imperio" / "Aura Celestial": a standing disc of light at the
    // feet with rising motes, brightening with the cumulative phase.
    void RenderRestAura(const CHARACTER* character, OBJECT* object, BMD* model, Family family, float phase)
    {
        if (!rand_fps_check(RestInterval))
        {
            return;
        }

        const Palette& palette = PaletteOf(family);
        const float intensity = (0.45f + 0.55f * phase) * object->Alpha;
        const float pulse = Pulse();

        vec3_t local, world, light;
        Vector(0.f, 0.f, AuraHeight, local);
        if (!ResolveBone(model, object, 0, local, world))
        {
            return;
        }

        ScaledLight(palette.Primary, intensity, light);
        CreateSprite(BITMAP_LIGHT, world, AuraScale + pulse * 0.25f, light, object);

        ScaledLight(palette.Accent, intensity * 0.8f, light);
        CreateSprite(BITMAP_SHINY + 1, world, 0.7f + pulse * 0.3f, light, object,
                     static_cast<float>(WorldTime) * 0.045f);

        if (rand_fps_check(RestInterval * 2))
        {
            ScaledLight(palette.Highlight, intensity, light);
            CreateParticle(BITMAP_SPARK + 1, world, object->Angle, light, 11, 0.35f + pulse * 0.2f, object);
        }

        // "Protecao das Aguas": the full phase opens a wider ward ring.
        if (phase >= WardPhaseThreshold)
        {
            ScaledLight(palette.Accent, intensity * 0.55f, light);
            CreateSprite(BITMAP_LIGHT, world, WardScale + pulse * 0.35f, light, object,
                         360.f - static_cast<float>(WorldTime) * 0.03f);
        }
    }

    // "Ondas Oceanicas" / "Trilha Celestial": a low wake dragged along the
    // ground while the character walks, runs or rides.
    void RenderTravelWake(const CHARACTER* character, OBJECT* object, BMD* model, Family family, float phase)
    {
        if (!rand_fps_check(TravelInterval))
        {
            return;
        }

        const Palette& palette = PaletteOf(family);
        const float intensity = (0.4f + 0.6f * phase) * object->Alpha;

        vec3_t local, world, light;
        Vector(0.f, 0.f, WakeHeight, local);
        if (!ResolveBone(model, object, 0, local, world))
        {
            return;
        }

        ScaledLight(palette.Accent, intensity, light);
        CreateSprite(BITMAP_LIGHT, world, WakeScale, light, object);
        CreateParticleFpsChecked(BITMAP_SPARK + 1, world, object->Angle, light, 5, 0.55f, object);
    }

    // "Efeito de Ataque": the trident's golden surge / the storm blade's
    // lightning, anchored on the weapon the character is actually swinging.
    void RenderStrikeFlash(const CHARACTER* character, OBJECT* object, BMD* model, Family family, float phase)
    {
        if (!rand_fps_check(StrikeInterval))
        {
            return;
        }

        const Palette& palette = PaletteOf(family);
        const float intensity = (0.55f + 0.45f * phase) * object->Alpha;
        const int hand = StrikeHand(character, family);

        vec3_t local, world, light;
        Vector(0.f, 0.f, 0.f, local);
        if (!ResolveBone(model, object, character->Weapon[hand].LinkBone, local, world))
        {
            return;
        }

        ScaledLight(palette.Primary, intensity, light);
        CreateSprite(BITMAP_SHINY + 1, world, 0.9f, light, object, static_cast<float>(WorldTime) * 0.09f);

        ScaledLight(palette.Accent, intensity, light);
        CreateParticle(BITMAP_SPARK + 1, world, object->Angle, light, 11, 0.5f, object);
    }
}

    float ActivePhase(const CHARACTER* character)
    {
        if (character == nullptr)
            return 0.f;
        return static_cast<float>(character->ServerEquipment.ActivePercent()) / 100.f;
    }

    Family FamilyOf(const CHARACTER* character)
    {
        if (character == nullptr)
            return Family::None;

        const int armor = character->BodyPart[BODYPART_ARMOR].Type;
        if (Poseidon::IsEquipment(armor))
            return Family::Poseidon;
        if (Zeus::IsEquipment(armor))
            return Family::Zeus;
        return Family::None;
    }

    bool EffectsAllowed(OBJECT* object)
    {
        return object != nullptr
            && g_pOption->GetRenderLevel() >= MinimumRenderLevel
            && object->Distance <= EffectMaximumDistance
            && object->Alpha > 0.f
            && !g_isCharacterBuff(object, eBuff_Cloaking);
    }

    void RenderSetEffects(const CHARACTER* character, OBJECT* object, BMD* model)
    {
        const float phase = ActivePhase(character);
        if (phase <= 0.f || !EffectsAllowed(object))
        {
            return;
        }

        const Family family = FamilyOf(character);
        if (family == Family::None)
        {
            return;
        }

        switch (ClassifyMotion(object->CurrentAction))
        {
        case Motion::Strike:
            RenderStrikeFlash(character, object, model, family, phase);
            break;
        case Motion::Travel:
            RenderTravelWake(character, object, model, family, phase);
            break;
        case Motion::Rest:
        default:
            RenderRestAura(character, object, model, family, phase);
            break;
        }
    }
}
