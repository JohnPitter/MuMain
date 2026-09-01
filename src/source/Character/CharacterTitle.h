#pragma once

#include <cstddef>
#include <vector>

class CHARACTER;

// Display titles above the nameplate. The real rank lives on Character.State
// (OpenMU HeroState) and arrives as CHARACTER::PK: 3 = Commoner / "Plebeu".
// The player may pick another rank from this server's catalog (hero states +
// Gens) and that choice is stored in config.ini [Titles].
namespace CharacterTitle
{
    struct Rank
    {
        int Id;
        const wchar_t* Name;
    };

    const std::vector<Rank>& Catalog();

    int SelectedId();
    void Select(int id);

    // PK status → the rank this character actually holds.
    const wchar_t* FromHeroState(BYTE pk);

    // What to draw above `owner`'s name. Local hero uses the picked title
    // (0 = follow PK); everyone else uses their real PK rank.
    void Fill(CHARACTER* owner, wchar_t* dest, size_t destChars);
}
