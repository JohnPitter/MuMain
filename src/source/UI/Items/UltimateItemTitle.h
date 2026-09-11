#pragma once

#include <span>

namespace UI::Items::UltimateTitle
{
    bool TryFormat(int itemType, int level, std::span<wchar_t> title);
}
