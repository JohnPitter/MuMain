#include "doctest.h"

#include <windows.h>
#include "Core/Globals/_define.h"
#include "Render/Models/CelestialModels.h"
#include "Render/Models/HelmetAppearance.h"

using Render::Items::Celestial::IsAuthoredEquipment;

TEST_CASE("Celestial authored surface policy only covers its ten equipment models")
{
    int count = 0;
    for (int model = 0; model < MAX_MODELS; ++model)
        count += IsAuthoredEquipment(model) ? 1 : 0;

    CHECK(count == 10);
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_STAFF));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_SHIELD));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_RING));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_PENDANT));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_HELM));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_ARMOR));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_PANTS));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_GLOVES));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_BOOTS));
    CHECK(IsAuthoredEquipment(MODEL_CELESTIAL_WINGS));
}

TEST_CASE("Celestial policy does not change existing equipment")
{
    CHECK_FALSE(IsAuthoredEquipment(MODEL_STAFF_OF_KUNDUN));
    CHECK_FALSE(IsAuthoredEquipment(MODEL_CELESTIAL_BOW));
    CHECK_FALSE(IsAuthoredEquipment(MODEL_PHOENIX_SOUL_ARMOR));
    CHECK_FALSE(IsAuthoredEquipment(MODEL_WING_OF_RUIN));
    CHECK_FALSE(IsAuthoredEquipment(MODEL_HELPER + EWS_ELF_2_CHARM));
    CHECK_FALSE(IsAuthoredEquipment(-1));
}

TEST_CASE("Celestial staff anchors agree with the authored BMD skeleton contract")
{
    CHECK(Render::Items::Celestial::StaffHeartBone == 1);
    CHECK(Render::Items::Celestial::StaffTipBone == 2);
    CHECK(Render::Items::Celestial::WingHaloBone == 47);
    CHECK(MODEL_CELESTIAL_RING == MODEL_HELPER + 200);
    CHECK(MODEL_CELESTIAL_PENDANT == MODEL_HELPER + 201);
}

TEST_CASE("Open Celestial crown shows the native head without changing other helmets")
{
    for (int model = -1; model < MAX_MODELS; ++model)
    {
        const bool nativeOpenHelmet = model == MODEL_HELM || model == MODEL_PAD_HELM
            || model == MODEL_HELM + 63 || model == MODEL_HELM + 68
            || model == MODEL_HELM + 65 || model == MODEL_HELM + 70
            || (model >= MODEL_VINE_HELM && model <= MODEL_SPIRIT_HELM);
        CHECK(Render::Items::UsesSeparateHead(model)
            == (nativeOpenHelmet || model == MODEL_CELESTIAL_HELM));
    }
}
