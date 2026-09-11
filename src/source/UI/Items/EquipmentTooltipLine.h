#pragma once

#include <array>

namespace UI::Items::EquipmentTooltip
{
    enum class LineRole { Title, Active, Inactive, Detail, Note };
    constexpr std::size_t TooltipLineLength = 100;
    constexpr std::size_t MaximumTooltipLines = 28;

    struct Line
    {
        std::array<wchar_t, TooltipLineLength> Text{};
        LineRole Role = LineRole::Detail;
    };
}
