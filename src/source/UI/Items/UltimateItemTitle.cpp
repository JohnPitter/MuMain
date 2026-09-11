#include "stdafx.h"
#include "UI/Items/UltimateItemTitle.h"

#include <cwchar>
#include "Character/EquipmentCatalogCache.h"
#include "Engine/Object/ZzzInfomation.h"
#include "I18N/All.h"

namespace UI::Items::UltimateTitle
{
    bool TryFormat(int itemType, int level, std::span<wchar_t> title)
    {
        if (!Character::Equipment::GetCatalog().IsUltimate(itemType) || title.empty())
            return false;
        const auto name = ItemAttribute[itemType].Name;
        if (level == 0)
            std::swprintf(title.data(), title.size(), L"%ls %ls", I18N::Game::Ultimate, name);
        else
            std::swprintf(title.data(), title.size(), L"%ls %ls +%d", I18N::Game::Ultimate, name, level);
        return true;
    }
}
