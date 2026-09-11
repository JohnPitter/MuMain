#pragma once

#include <array>

namespace UI::Items::EquipmentTooltip
{
    struct Strings
    {
        const wchar_t* Title;
        const wchar_t* Required;
        const wchar_t* Upgrade;
        const wchar_t* Active;
        const wchar_t* Inactive;
        const wchar_t* Unknown;
        const wchar_t* Condition;
        const wchar_t* RecoveryNote;
        const wchar_t* LimitsNote;
        std::array<const wchar_t*, 13> Labels;
        std::array<const wchar_t*, 3> Units;
        const wchar_t* PhaseStatus = nullptr;
        const wchar_t* PhaseTemplate = nullptr;
        std::array<const wchar_t*, 3> PhaseLabels{};
        const wchar_t* PhaseCondition = nullptr;
    };
}
