#pragma once

#include <cstddef>
#include <vector>

class CHARACTER;

// Display titles above the nameplate. Automático (id 0) follows Character.State
// (OpenMU HeroState / CHARACTER::PK). Ids 1–19 are prize/shop cosmetics; the
// server owns the list and equipped id. The Y window shows Automático plus
// titles the character owns.
namespace CharacterTitle
{
    constexpr int AutoId = 0;
    // Sentinel equipped id: title display hidden (nothing above the name).
    constexpr int HiddenId = 255;
    constexpr int ScrollFirstNumber = 170;
    constexpr int ScrollCount = 19;

    struct Rank
    {
        int Id;
        const wchar_t* Name;
    };

    // Full catalog (names for id 0..19). The Y window uses Visible().
    const std::vector<Rank>& Catalog();

    // Automático + owned prize titles, in catalog order.
    const std::vector<Rank>& Visible();

    int SelectedId();
    void Select(int id);

    // Show/hide toggle used by the Y window button. Hidden draws no title;
    // toggling back restores the last visible selection.
    void ToggleHidden();
    bool IsHidden();

    void Reset();
    void ReceiveOwned(const BYTE* buffer, int32_t size);
    void ReceiveAppearance(const BYTE* buffer, int32_t size);

    // PK status → the rank this character actually holds.
    const wchar_t* FromHeroState(BYTE pk);

    // Gold line above `owner`'s name in-world. Id 0 draws nothing.
    // Never used on character-select balloons.
    void Fill(CHARACTER* owner, wchar_t* dest, size_t destChars);
}
