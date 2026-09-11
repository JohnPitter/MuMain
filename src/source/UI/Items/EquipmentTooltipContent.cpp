#include "UI/Items/EquipmentTooltipContent.h"

#include <algorithm>
#include <cwchar>

namespace UI::Items::EquipmentTooltip
{
    namespace
    {
        void SetText(Line& line, const wchar_t* text, LineRole role)
        {
            line = {};
            const auto length = std::min(std::wcslen(text), line.Text.size() - 1);
            std::copy_n(text, length, line.Text.data());
            line.Role = role;
        }

        void SetBonus(Line& line, const Character::Equipment::SetBonus& bonus, const Strings& strings, bool active)
        {
            const auto kind = static_cast<std::size_t>(bonus.Kind);
            const auto unit = static_cast<std::size_t>(bonus.Unit);
            line = {};
            line.Role = active ? LineRole::Detail : LineRole::Inactive;
            if (kind >= strings.Labels.size() || unit >= strings.Units.size())
                return;
            std::swprintf(line.Text.data(), line.Text.size(), L"%ls: %+.4g%ls",
                strings.Labels[kind], static_cast<double>(bonus.Value), strings.Units[unit]);
        }
    }

    std::size_t BuildLines(const Character::Equipment::BonusCatalog& catalog,
        const Character::Equipment::State& state, const Strings& strings, std::span<Line> lines)
    {
        constexpr std::size_t FixedLines = 6;
        const auto needed = FixedLines + catalog.BonusCount + (catalog.MinimumUpgrade > 0 ? 1 : 0);
        if (!catalog.Known || catalog.MemberCount == 0 || catalog.BonusCount > catalog.Bonuses.size() || lines.size() < needed)
            return 0;

        std::size_t index = 0;
        SetText(lines[index++], strings.Title, LineRole::Title);
        lines[index] = {};
        std::swprintf(lines[index].Text.data(), lines[index].Text.size(), strings.Required, static_cast<unsigned>(catalog.RequiredItems));
        ++index;
        if (catalog.MinimumUpgrade > 0)
        {
            lines[index] = {};
            std::swprintf(lines[index].Text.data(), lines[index].Text.size(), strings.Upgrade, static_cast<unsigned>(catalog.MinimumUpgrade));
            ++index;
        }
        const bool active = state.HasCelestialAura();
        SetText(lines[index++], state.Known ? (active ? strings.Active : strings.Inactive) : strings.Unknown,
            active ? LineRole::Active : LineRole::Inactive);
        for (std::size_t row = 0; row < catalog.BonusCount; ++row)
            SetBonus(lines[index++], catalog.Bonuses[row], strings, active);
        SetText(lines[index++], strings.Condition, LineRole::Note);
        SetText(lines[index++], strings.RecoveryNote, LineRole::Note);
        SetText(lines[index++], strings.LimitsNote, LineRole::Note);
        return index;
    }
}
