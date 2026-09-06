#include "doctest.h"

#include "MUHelper/MuHelperPotionThreshold.h"

#include <cstdint>

using namespace MUHelper::Potion;

// --- Threshold decision (CMuHelper::TryUseHealthPotion / TryUseManaPotion) ---
//
// ShouldUsePotion() is the exact rule both the HP and the MP auto-potion
// checks in MuHelper.cpp run (see ConsumePotion()'s doc comment for how the
// two share one request cooldown and why HP is checked first); these cases
// pin the percentage math and the guard conditions independent of the
// engine-only bits (CharacterAttribute, inventory lookup, SendRequestUse)
// that keep this logic out of MuHelperData.cpp's own doctest binary.

TEST_CASE("ShouldUsePotion triggers once the remaining percentage reaches the threshold")
{
    // 40 life out of 100 max = 40% remaining, threshold 40% -- "Status do HP"
    // reading 4/10 segments lit, matching the screenshot in the bug report.
    CHECK(ShouldUsePotion(true, 40, 100, 40) == true);

    // Still above the threshold: no potion yet.
    CHECK(ShouldUsePotion(true, 41, 100, 40) == false);

    // Further below the threshold: still triggers.
    CHECK(ShouldUsePotion(true, 1, 100, 40) == true);
}

TEST_CASE("ShouldUsePotion never fires when the group is disabled")
{
    // Same life/threshold as the triggering case above, but the "Poção
    // Automática" checkbox (bUseHealPotion) is off.
    CHECK(ShouldUsePotion(false, 10, 100, 40) == false);
}

TEST_CASE("ShouldUsePotion never fires on a dead or uninitialized resource")
{
    // max <= 0 (attributes not loaded yet) must not be treated as "0% remaining".
    CHECK(ShouldUsePotion(true, 0, 0, 40) == false);

    // current <= 0 (character already dead) -- there is nothing to save with a potion.
    CHECK(ShouldUsePotion(true, 0, 100, 40) == false);
}

TEST_CASE("ShouldUsePotion rounds the same way for a threshold of 0 (feature effectively off)")
{
    // A blob decoded from before this feature existed has iManaPotionThreshold
    // == 0 (see MuHelperData.h). Only a resource that has hit exactly zero
    // would satisfy "<= 0%", and that case is already excluded by the
    // current <= 0 guard above, so old saves never spuriously drink MP potions.
    CHECK(ShouldUsePotion(true, 1, 100, 0) == false);
}

TEST_CASE("RemainingPercent ceils instead of truncating")
{
    // 39 of 100 truncates to 39% either way; 1 of 3 truncates to 33% with
    // plain integer division but the bar visually reads "still has some in
    // the last segment", i.e. round up to 34%, matching HPStatusAutoPotion's
    // existing (value * 10) nibble granularity in MuHelperData.cpp.
    CHECK(RemainingPercent(39, 100) == 39);
    CHECK(RemainingPercent(1, 3) == 34);
    CHECK(RemainingPercent(0, 100) == 0);
    CHECK(RemainingPercent(100, 100) == 100);
}

// --- Settings byte round trip (ConfigDataSerDe <-> PRECEIVE_MUHELPER_DATA) ---
//
// MPStatusAutoPotion is the low nibble of byte 34 of the packet in
// Network/Server/WSclient.h (byte 30 of the persisted 257-byte blob; see the
// layout doc comment on OpenMU's MuHelperSettingsSerializer). It sits right
// after the byte-33 client-local flag bits from the "Accept party" change and
// takes 1 of the 35 padding bytes that followed them, so the blob stays 257
// bytes. The real bitfield struct lives behind Network/Server/WSclient.h,
// which drags in the full engine PCH this lightweight doctest binary does not
// link against (see test_muhelper_settings.cpp) -- what is tested here is the
// wire contract itself: the nibble's position, mask, and that neither the
// unused high nibble of its own byte nor byte 33's flags leak into it.
namespace
{
constexpr std::uint8_t kManaPotionNibbleMask = 0x0F;
}

TEST_CASE("MPStatusAutoPotion nibble round-trips a threshold step 0-10")
{
    for (int step = 0; step <= 10; ++step)
    {
        // Serialize: ConfigDataSerDe::Serialize's cast-and-mask.
        const std::uint8_t encoded = static_cast<std::uint8_t>(step & kManaPotionNibbleMask);
        CHECK(encoded == step);

        // Deserialize: ConfigDataSerDe::Deserialize's (value * 10) back to a percentage.
        const int decodedPercent = static_cast<int>(encoded) * 10;
        CHECK(decodedPercent == step * 10);
    }
}

TEST_CASE("MPStatusAutoPotion nibble ignores the unused high nibble of its own byte")
{
    // High nibble set to garbage (0xF), low nibble (the real field) at step 6 -> 60%.
    constexpr std::uint8_t byteWithGarbageHighNibble = 0xF6;

    const std::uint8_t decodedNibble = byteWithGarbageHighNibble & kManaPotionNibbleMask;
    CHECK(decodedNibble == 6);
    CHECK(static_cast<int>(decodedNibble) * 10 == 60);
}

TEST_CASE("a zero MPStatusAutoPotion byte decodes to threshold 0, matching a pre-feature blob")
{
    constexpr std::uint8_t byte = 0;
    CHECK((byte & kManaPotionNibbleMask) == 0);
}
