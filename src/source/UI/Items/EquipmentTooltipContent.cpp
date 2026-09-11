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

        void SetBonus(Line& line, const Character::Equipment::SetBonus& bonus, const Strings& strings, unsigned percent)
        {
            const auto kind = static_cast<std::size_t>(bonus.Kind);
            const auto unit = static_cast<std::size_t>(bonus.Unit);
            line = {};
            line.Role = percent > 0 ? LineRole::Detail : LineRole::Inactive;
            if (kind >= strings.Labels.size() || unit >= strings.Units.size())
                return;
            const double maximum = bonus.Value;
            if (strings.PhaseTemplate != nullptr)
                std::swprintf(line.Text.data(), line.Text.size(), L"%ls: %+.4g%ls / %+.4g%ls",
                    strings.Labels[kind], maximum * percent / 100, strings.Units[unit], maximum, strings.Units[unit]);
            else
                std::swprintf(line.Text.data(), line.Text.size(), L"%ls: %+.4g%ls",
                    strings.Labels[kind], maximum, strings.Units[unit]);
        }

        std::size_t AddPhases(const Character::Equipment::BonusCatalog& catalog, unsigned percent,
            const Strings& strings, std::span<Line> lines)
        {
            if (strings.PhaseTemplate == nullptr)
                return 0;
            for (std::size_t index = 0; index < catalog.PhaseCount; ++index)
            {
                auto& line = lines[index];
                line = {};
                const auto phase = catalog.Phases[index];
                line.Role = percent >= phase.Percent ? LineRole::Active : LineRole::Inactive;
                const auto label = strings.PhaseLabels[index];
                std::swprintf(line.Text.data(), line.Text.size(), strings.PhaseTemplate,
                    static_cast<unsigned>(index + 1), static_cast<unsigned>(phase.Percent), label == nullptr ? L"" : label);
            }
            return catalog.PhaseCount;
        }
    }

    Line BuildStatus(const Character::Equipment::State& state, const Strings& strings)
    {
        Line line;
        const bool active = state.HasCelestialAura();
        SetText(line, state.Known ? (active ? strings.Active : strings.Inactive) : strings.Unknown,
            active ? LineRole::Active : LineRole::Inactive);
        if (state.Known && strings.PhaseStatus != nullptr)
            std::swprintf(line.Text.data(), line.Text.size(), strings.PhaseStatus, state.ActivePercent());
        return line;
    }

    std::size_t BuildLines(const Character::Equipment::BonusCatalog& catalog,
        const Character::Equipment::State& state, const Strings& strings, std::span<Line> lines)
    {
        constexpr std::size_t FixedLines = 6;
        const auto phaseLines = strings.PhaseTemplate != nullptr ? catalog.PhaseCount : 0;
        const auto needed = FixedLines + phaseLines + catalog.BonusCount + (catalog.MinimumUpgrade > 0 ? 1 : 0);
        if (!catalog.Known || catalog.MemberCount == 0 || catalog.BonusCount > catalog.Bonuses.size()
            || catalog.PhaseCount > catalog.Phases.size() || lines.size() < needed)
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
        lines[index++] = BuildStatus(state, strings);
        index += AddPhases(catalog, state.ActivePercent(), strings, lines.subspan(index));
        for (std::size_t row = 0; row < catalog.BonusCount; ++row)
            SetBonus(lines[index++], catalog.Bonuses[row], strings, state.ActivePercent());
        SetText(lines[index++], catalog.PhaseCount > 0 && strings.PhaseCondition != nullptr
            ? strings.PhaseCondition : strings.Condition, LineRole::Note);
        SetText(lines[index++], strings.RecoveryNote, LineRole::Note);
        SetText(lines[index++], strings.LimitsNote, LineRole::Note);
        return index;
    }
}
