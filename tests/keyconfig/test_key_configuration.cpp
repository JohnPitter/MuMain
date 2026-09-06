// Unit tests for the pure Q/W/E/R potion-slot layout in
// Network/Server/KeyConfiguration.h. SaveOptions() (ZzzOpenData.cpp) writes the
// 30-byte blob and ReceiveOption() (WSclient.cpp) reads the very same blob back
// from the server on the next login, so writer and reader must be exact
// inverses. They were not: the writer emitted the four levels as bytes 26..29
// (Q, W, E, R) and the reader unpacked those four bytes as a big-endian int,
// which handed Q the byte the writer had used for R. A level that does not
// match the item in the inventory makes the slot resolve to nothing, so the
// hotkey renders empty and the player has to bind the potion again.

#include "doctest.h"

#include <cstdint>
#include <cstring>

#include "Network/Server/KeyConfiguration.h"

namespace
{
    // What ReceiveOption() used to do with bytes 26..29: read them as a
    // little-endian int and unpack it big-endian.
    struct LegacyLevels
    {
        int Q;
        int W;
        int E;
        int R;
    };

    LegacyLevels ReadLevelsTheOldWay(const std::uint8_t* configuration)
    {
        std::int32_t packed = 0;
        std::memcpy(&packed, configuration + 26, sizeof(packed));

        LegacyLevels levels{};
        levels.Q = (packed & 0xFF000000u) >> 24;
        levels.W = (packed & 0x00FF0000u) >> 16;
        levels.E = (packed & 0x0000FF00u) >> 8;
        levels.R = packed & 0x000000FF;
        return levels;
    }

    constexpr int kLargeHealingPotion = 3;
    constexpr int kLargeManaPotion = 6;
    constexpr int kAntidote = 8;
}

TEST_CASE("the four potion slots live at the Season 6 offsets")
{
    CHECK(KeyConfiguration::Size == 30);
    CHECK(KeyConfiguration::SlotCount == 4);

    // Q, W, E are consecutive; R sits behind the chat-log-box byte at 24.
    CHECK(KeyConfiguration::ItemOffset[0] == 21);
    CHECK(KeyConfiguration::ItemOffset[1] == 22);
    CHECK(KeyConfiguration::ItemOffset[2] == 23);
    CHECK(KeyConfiguration::ItemOffset[3] == 25);

    CHECK(KeyConfiguration::LevelOffset[0] == 26);
    CHECK(KeyConfiguration::LevelOffset[1] == 27);
    CHECK(KeyConfiguration::LevelOffset[2] == 28);
    CHECK(KeyConfiguration::LevelOffset[3] == 29);
}

TEST_CASE("a bound potion survives the write/read round trip")
{
    std::uint8_t configuration[KeyConfiguration::Size]{};

    KeyConfiguration::WritePotionSlot(configuration, 0, { kLargeHealingPotion, 0 });
    KeyConfiguration::WritePotionSlot(configuration, 1, { kLargeManaPotion, 0 });
    KeyConfiguration::WritePotionSlot(configuration, 2, { kAntidote, 0 });
    KeyConfiguration::WritePotionSlot(configuration, 3, { -1, 0 });

    CHECK(configuration[21] == kLargeHealingPotion);
    CHECK(configuration[22] == kLargeManaPotion);
    CHECK(configuration[23] == kAntidote);
    CHECK(configuration[25] == KeyConfiguration::Unbound);

    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 0).ItemIndex == kLargeHealingPotion);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 1).ItemIndex == kLargeManaPotion);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 2).ItemIndex == kAntidote);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 3).ItemIndex == -1);
}

TEST_CASE("an unbound slot reads back as unbound with a zero level")
{
    std::uint8_t configuration[KeyConfiguration::Size]{};
    std::memset(configuration, 0x7B, sizeof(configuration));

    KeyConfiguration::WritePotionSlot(configuration, 3, { -1, 9 });

    const KeyConfiguration::PotionSlot slot = KeyConfiguration::ReadPotionSlot(configuration, 3);
    CHECK(configuration[25] == KeyConfiguration::Unbound);
    CHECK(configuration[29] == 0);
    CHECK(slot.ItemIndex == -1);
    CHECK(slot.ItemLevel == 0);
}

TEST_CASE("each slot keeps its own level across the round trip")
{
    std::uint8_t configuration[KeyConfiguration::Size]{};

    KeyConfiguration::WritePotionSlot(configuration, 0, { kLargeHealingPotion, 1 });
    KeyConfiguration::WritePotionSlot(configuration, 1, { kLargeManaPotion, 2 });
    KeyConfiguration::WritePotionSlot(configuration, 2, { kAntidote, 3 });
    KeyConfiguration::WritePotionSlot(configuration, 3, { kLargeHealingPotion, 4 });

    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 0).ItemLevel == 1);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 1).ItemLevel == 2);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 2).ItemLevel == 3);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 3).ItemLevel == 4);
}

TEST_CASE("the regression: the old reader handed Q the level of R")
{
    std::uint8_t configuration[KeyConfiguration::Size]{};

    KeyConfiguration::WritePotionSlot(configuration, 0, { kLargeHealingPotion, 1 });
    KeyConfiguration::WritePotionSlot(configuration, 1, { kLargeManaPotion, 2 });
    KeyConfiguration::WritePotionSlot(configuration, 2, { kAntidote, 3 });
    KeyConfiguration::WritePotionSlot(configuration, 3, { kLargeHealingPotion, 4 });

    // Reading the same bytes the way the client did before the fix swaps
    // Q with R and W with E - the bug the player saw as "my potions are gone".
    const LegacyLevels legacy = ReadLevelsTheOldWay(configuration);
    CHECK(legacy.Q == 4);
    CHECK(legacy.W == 3);
    CHECK(legacy.E == 2);
    CHECK(legacy.R == 1);

    // The current reader is the exact inverse of the writer.
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 0).ItemLevel == 1);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 3).ItemLevel == 4);
}

TEST_CASE("levels no real item can have are normalized to zero")
{
    CHECK(KeyConfiguration::SanitizeItemLevel(0) == 0);
    CHECK(KeyConfiguration::SanitizeItemLevel(15) == 15);
    CHECK(KeyConfiguration::SanitizeItemLevel(16) == 0);
    CHECK(KeyConfiguration::SanitizeItemLevel(0x47) == 0);
    CHECK(KeyConfiguration::SanitizeItemLevel(255) == 0);
    CHECK(KeyConfiguration::SanitizeItemLevel(-1) == 0);

    // A blob a previous (broken) session left on the character heals on read,
    // so the next save no longer carries the impossible level forward.
    std::uint8_t configuration[KeyConfiguration::Size]{};
    configuration[21] = kLargeManaPotion;
    configuration[26] = 0x47;

    const KeyConfiguration::PotionSlot slot = KeyConfiguration::ReadPotionSlot(configuration, 0);
    CHECK(slot.ItemIndex == kLargeManaPotion);
    CHECK(slot.ItemLevel == 0);
}

TEST_CASE("the server default binds Q and W and leaves E and R unbound")
{
    // CreateCharacterAction.CreateDefaultKeyConfiguration on the server side.
    std::uint8_t configuration[KeyConfiguration::Size]{};
    configuration[21] = 1;    // small healing potion
    configuration[22] = 4;    // small mana potion
    configuration[23] = KeyConfiguration::Unbound;
    configuration[25] = KeyConfiguration::Unbound;

    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 0).ItemIndex == 1);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 1).ItemIndex == 4);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 2).ItemIndex == -1);
    CHECK(KeyConfiguration::ReadPotionSlot(configuration, 3).ItemIndex == -1);
}
