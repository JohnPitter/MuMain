#include "stdafx.h"
#include "UI/Items/EquipmentTooltip.h"

#include <algorithm>
#include "Character/EquipmentCatalogCache.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInventory.h"
#include "Engine/Object/ZzzInterface.h"
#include "I18N/All.h"
#include "UI/Items/EquipmentTooltipContent.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUICommon.h"

extern wchar_t TextList[50][100];
extern int TextListColor[50];
extern int TextBold[50];

namespace UI::Items::EquipmentTooltip
{
    namespace
    {
        constexpr int LegacyLineCapacity = 50;
        constexpr float TooltipBottom = 420.0f;
        std::array<Line, MaximumTooltipLines> DetailLines;

        Strings CurrentStrings()
        {
            using namespace I18N::Game;
            using Character::Equipment::Category;
            Strings strings{ CelestialBonusTitle, CelestialBonusRequired, CelestialBonusUpgrade,
                CelestialBonusActive, CelestialBonusInactive, CelestialBonusUnknown,
                CelestialBonusCondition, CelestialBonusRecoveryNote, CelestialBonusLimitsNote,
                { L"", CelestialBonusDamage, CelestialBonusDefense, CelestialBonusHealth, CelestialBonusMana,
                  CelestialBonusAttackSpeed, CelestialBonusMagicSpeed, CelestialBonusMovement,
                  CelestialBonusIgnoreDefense, CelestialBonusCriticalChance, CelestialBonusHealthRecovery,
                  CelestialBonusManaRecovery, CelestialBonusElementalProtection },
                { L"", L"%", CelestialBonusPercentagePoints },
                CelestialPhaseStatus, CelestialPhaseTemplate,
                { CelestialPhaseArmor, CelestialPhaseWeapons, CelestialPhaseAccessories }, CelestialPhaseCondition };
            if (Character::Equipment::GetCatalog().ItemCategory == Category::Poseidon)
            {
                strings.Title = PoseidonSetTitle;
                strings.PhaseLabels = { PoseidonPhaseArmor, PoseidonPhaseJewels, PoseidonPhaseComplete };
            }
            else if (Character::Equipment::GetCatalog().ItemCategory == Category::Zeus)
            {
                strings.Title = ZeusSetTitle;
                strings.PhaseLabels = { ZeusPhaseArmor, ZeusPhaseJewels, ZeusPhaseComplete };
            }
            return strings;
        }

        int Color(LineRole role)
        {
            switch (role)
            {
            case LineRole::Title: return TEXT_COLOR_YELLOW;
            case LineRole::Active: return TEXT_COLOR_GREEN;
            case LineRole::Inactive: return TEXT_COLOR_GRAY;
            case LineRole::Note: return TEXT_COLOR_WHITE;
            default: return TEXT_COLOR_BLUE;
            }
        }

        void RenderLines(int x, int y, std::size_t count)
        {
            for (std::size_t index = 0; index < count; ++index)
            {
                std::copy(DetailLines[index].Text.begin(), DetailLines[index].Text.end(), TextList[index]);
                TextListColor[index] = Color(DetailLines[index].Role);
                TextBold[index] = DetailLines[index].Role == LineRole::Title;
            }
            g_pRenderText->SetFont(g_hFont);
            SIZE size{};
            GetTextExtentPoint32W(g_pRenderText->GetFontDC(), L"A", 1, &size);
            const float height = count * size.cy / g_fScreenRate_y * 1.1f;
            const int top = static_cast<int>(std::clamp(static_cast<float>(y), 0.0f, std::max(0.0f, TooltipBottom - height)));
            RenderTipTextList(x, top, static_cast<int>(count), 0);
        }
    }

    bool TryRenderDetails(int itemType, int x, int y)
    {
        const auto& catalog = Character::Equipment::GetCatalog();
        if (!SEASON3B::IsRepeat(VK_SHIFT) || !catalog.Contains(itemType) || Hero == nullptr)
            return false;
        const auto count = BuildLines(catalog, Hero->ServerEquipment, CurrentStrings(), DetailLines);
        if (count == 0)
            return false;
        RenderLines(x, y, count);
        return true;
    }

        int AppendHint(int itemType, int textIndex)
        {
            constexpr int HintLines = 2;
            if (!Character::Equipment::GetCatalog().Contains(itemType) || Hero == nullptr
                || textIndex < 0 || textIndex > LegacyLineCapacity - HintLines)
                return textIndex;
            const auto& state = Hero->ServerEquipment;
            const auto strings = CurrentStrings();
            const auto status = BuildStatus(state, strings);
            swprintf_s(TextList[textIndex], L"%ls", status.Text.data());
            TextListColor[textIndex] = state.HasCelestialAura() ? TEXT_COLOR_GREEN : TEXT_COLOR_GRAY;
            TextBold[textIndex++] = false;
            swprintf_s(TextList[textIndex], L"%ls", Character::Equipment::GetCatalog().ItemCategory
                == Character::Equipment::Category::Poseidon ? I18N::Game::PoseidonBonusShiftHint
                : Character::Equipment::GetCatalog().ItemCategory == Character::Equipment::Category::Zeus
                ? I18N::Game::ZeusBonusShiftHint : I18N::Game::CelestialBonusShiftHint);
            TextListColor[textIndex] = TEXT_COLOR_YELLOW;
            TextBold[textIndex++] = false;
            return textIndex;
        }
}
