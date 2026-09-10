#pragma once

#include <map>
#include "Core/Globals/_enum.h"

namespace Render::Items
{
    inline bool UsesSeparateHead(int helmet)
    {
        return helmet == MODEL_HELM || helmet == MODEL_PAD_HELM
            || helmet == MODEL_HELM + 63 || helmet == MODEL_HELM + 68
            || helmet == MODEL_HELM + 65 || helmet == MODEL_HELM + 70
            || helmet == MODEL_CELESTIAL_HELM
            || (helmet >= MODEL_VINE_HELM && helmet <= MODEL_SPIRIT_HELM);
    }
}
