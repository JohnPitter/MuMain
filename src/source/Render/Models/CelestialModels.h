#pragma once

#include <map>
#include "Core/Globals/_enum.h"

namespace Render::Items::Celestial
{
    constexpr int StaffHeartBone = 1;
    constexpr int StaffTipBone = 2;

    inline bool IsAuthoredProp(int model)
    {
        return model == MODEL_CELESTIAL_STAFF || model == MODEL_CELESTIAL_SHIELD
            || model == MODEL_CELESTIAL_RING || model == MODEL_CELESTIAL_PENDANT;
    }
}
