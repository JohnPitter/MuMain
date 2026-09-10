#pragma once

#include <map>
#include "Core/Globals/_enum.h"

namespace Render::Items::Celestial
{
    constexpr int StaffHeartBone = 1;
    constexpr int StaffTipBone = 2;
    constexpr int WingHaloBone = 47;

    inline bool IsAuthoredEquipment(int model)
    {
        return model == MODEL_CELESTIAL_STAFF || model == MODEL_CELESTIAL_SHIELD
            || model == MODEL_CELESTIAL_RING || model == MODEL_CELESTIAL_PENDANT
            || model == MODEL_CELESTIAL_HELM || model == MODEL_CELESTIAL_ARMOR
            || model == MODEL_CELESTIAL_PANTS || model == MODEL_CELESTIAL_GLOVES
            || model == MODEL_CELESTIAL_BOOTS || model == MODEL_CELESTIAL_WINGS;
    }
}
