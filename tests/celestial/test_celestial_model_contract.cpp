#include "doctest.h"

#include <windows.h>
#include "Core/Globals/_define.h"
#include "Render/Models/CelestialModels.h"

using Render::Items::Celestial::IsAuthoredProp;

TEST_CASE("Celestial authored surface policy only covers its four model IDs")
{
    int count = 0;
    for (int model = 0; model < MAX_MODELS; ++model)
        count += IsAuthoredProp(model) ? 1 : 0;

    CHECK(count == 4);
    CHECK(IsAuthoredProp(MODEL_CELESTIAL_STAFF));
    CHECK(IsAuthoredProp(MODEL_CELESTIAL_SHIELD));
    CHECK(IsAuthoredProp(MODEL_CELESTIAL_RING));
    CHECK(IsAuthoredProp(MODEL_CELESTIAL_PENDANT));
}

TEST_CASE("Celestial policy does not change existing gear or placeholder armor")
{
    CHECK_FALSE(IsAuthoredProp(MODEL_STAFF_OF_KUNDUN));
    CHECK_FALSE(IsAuthoredProp(MODEL_CELESTIAL_BOW));
    CHECK_FALSE(IsAuthoredProp(MODEL_CELESTIAL_ARMOR));
    CHECK_FALSE(IsAuthoredProp(MODEL_CELESTIAL_WINGS));
    CHECK_FALSE(IsAuthoredProp(MODEL_HELPER + EWS_ELF_2_CHARM));
    CHECK_FALSE(IsAuthoredProp(-1));
}

TEST_CASE("Celestial staff anchors agree with the authored BMD skeleton contract")
{
    CHECK(Render::Items::Celestial::StaffHeartBone == 1);
    CHECK(Render::Items::Celestial::StaffTipBone == 2);
    CHECK(MODEL_CELESTIAL_RING == MODEL_HELPER + 200);
    CHECK(MODEL_CELESTIAL_PENDANT == MODEL_HELPER + 201);
}
