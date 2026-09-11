#include "doctest.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include "Character/EquipmentCatalogCache.h"
#include "Network/Server/EquipmentBonusCatalogPacket.h"

namespace
{
    using Character::Equipment::BonusCatalog;
    using Character::Equipment::Category;

    constexpr std::uint16_t Sword = 0 * 512 + 36;
    constexpr std::uint16_t Armor = 8 * 512 + 76;
    constexpr std::uint16_t Pants = 9 * 512 + 76;
    constexpr std::uint16_t Gloves = 10 * 512 + 76;
    constexpr std::uint16_t Boots = 11 * 512 + 76;
    constexpr std::uint16_t Wings = 12 * 512 + 53;
    constexpr std::uint16_t Cape = 12 * 512 + 54;
    constexpr std::uint16_t Staff = 5 * 512 + 38;
    constexpr std::uint16_t Pendant = 13 * 512 + 207;
    constexpr std::uint16_t StormRing = 13 * 512 + 208;
    constexpr std::uint16_t WisdomRing = 13 * 512 + 209;

    // Same shape as the Poseidon phased packet, but the category byte
    // (index 9) carries 3 = Zeus, the required class is the Duel Master (13)
    // and the single member is the Zeus sword (0/36).
    constexpr std::array<std::uint8_t, 26> ZeusPacket{
        0xC1, 26, 0xF3, 0xE7, 2, 1, 1, 1, 0, 3, 0x90, 1, 13, 1,
        0x24, 0x00, 1, 1, 1, 0, 0, 0x70, 0x41, 1, 0, 100
    };
}

TEST_CASE("E7 v2 accepts the Zeus category and keeps it distinct from Ultimate and Poseidon")
{
    const auto catalog = Network::Equipment::DecodeCatalog(ZeusPacket);
    REQUIRE(catalog.has_value());
    CHECK(catalog->ItemCategory == Category::Zeus);
    CHECK(catalog->RequiredLevel == 400);
    CHECK(catalog->RequiredClass == 13);
    CHECK(catalog->Contains(Sword));

    // The Zeus category is its own identity like the Poseidon one: not
    // Ultimate, but the level/class metadata still applies to members only.
    CHECK_FALSE(catalog->IsUltimate(Sword));
    CHECK(catalog->DisplayLevel(Sword, 0) == 400);
    CHECK(catalog->DisplayLevel(2 * 512 + 0, 80) == 80);
}

TEST_CASE("E7 v2 still rejects unknown categories and keeps Poseidon working unchanged")
{
    for (const auto category : {4, 5, 0xFF})
    {
        auto packet = ZeusPacket;
        packet[9] = static_cast<std::uint8_t>(category);
        CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    }
    // Regression: the Poseidon category (2) decodes exactly as before the
    // Zeus extension.
    auto poseidon = ZeusPacket;
    poseidon[9] = static_cast<std::uint8_t>(Category::Poseidon);
    poseidon[12] = 17; // Lord Emperor
    const auto catalog = Network::Equipment::DecodeCatalog(poseidon);
    REQUIRE(catalog.has_value());
    CHECK(catalog->ItemCategory == Category::Poseidon);
    CHECK(catalog->RequiredClass == 17);
}

TEST_CASE("Catalog emitted by dotnet describes the full Zeus set")
{
    const char* fixture = std::getenv("ZEUS_CATALOG_FIXTURE");
    if (fixture == nullptr)
    {
        MESSAGE("Set ZEUS_CATALOG_FIXTURE to check the actual dotnet Zeus catalog sender.");
        return;
    }
    std::ifstream input(fixture, std::ios::binary);
    REQUIRE(input.good());
    const std::vector<std::uint8_t> bytes{ std::istreambuf_iterator<char>(input), {} };
    REQUIRE(bytes.size() == 116);
    const auto catalog = Network::Equipment::DecodeCatalog(bytes);
    REQUIRE(catalog.has_value());
    CHECK(catalog->RequiredItems == 9);
    CHECK(catalog->MemberCount == 9);
    CHECK(catalog->BonusCount == 11);
    CHECK(catalog->MinimumUpgrade == 0);
    CHECK(catalog->ItemCategory == Category::Zeus);
    CHECK(catalog->RequiredLevel == 400);
    // 13 = Duel Master, the magic Gladiator evolution the set belongs to.
    CHECK(catalog->RequiredClass == 13);
    CHECK(catalog->PhaseCount == 3);
    CHECK(catalog->Phases[0].MemberMask == 0x001E);
    CHECK(catalog->Phases[1].MemberMask == 0x01DE);
    CHECK(catalog->Phases[2].MemberMask == 0x01FF);
    CHECK(catalog->Phases[0].Percent == 30);
    CHECK(catalog->Phases[1].Percent == 60);
    CHECK(catalog->Phases[2].Percent == 100);
    unsigned quantity = 0;
    for (std::size_t index = 0; index < catalog->MemberCount; ++index)
        quantity += catalog->Members[index].Quantity;
    CHECK(quantity == 9);
    // Nine counted members in emitter order: sword, the five body pieces at
    // index 76, wings, pendant and the two rings.
    CHECK(catalog->Members[0].ItemType == Sword);
    CHECK(catalog->Members[1].ItemType == Armor);
    CHECK(catalog->Members[2].ItemType == Pants);
    CHECK(catalog->Members[3].ItemType == Gloves);
    CHECK(catalog->Members[4].ItemType == Boots);
    CHECK(catalog->Members[5].ItemType == Wings);
    CHECK(catalog->Members[6].ItemType == Pendant);
    CHECK(catalog->Members[7].ItemType == StormRing);
    CHECK(catalog->Members[8].ItemType == WisdomRing);
    // The staff and the storm cape exist as items but stay outside the
    // counted set (one weapon slot is filled by the sword; the cape is extra).
    CHECK(catalog->Contains(Wings));
    CHECK(catalog->Contains(Pendant));
    CHECK(catalog->Contains(StormRing));
    CHECK(catalog->Contains(WisdomRing));
    CHECK_FALSE(catalog->Contains(Staff));
    CHECK_FALSE(catalog->Contains(Cape));
    // Bonus rows as emitted by update 249 (no magic speed row).
    const std::array<std::uint8_t, 11> kinds{ 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12 };
    const std::array<std::uint8_t, 11> units{ 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2 };
    const std::array<float, 11> values{ 15, 12, 10, 10, 10, 12, 10, 10, 20, 20, 10 };
    for (std::size_t index = 0; index < kinds.size(); ++index)
    {
        CHECK(catalog->Bonuses[index].Kind == static_cast<Character::Equipment::BonusKind>(kinds[index]));
        CHECK(catalog->Bonuses[index].Unit == static_cast<Character::Equipment::BonusUnit>(units[index]));
        CHECK(catalog->Bonuses[index].Value == values[index]);
    }
}
