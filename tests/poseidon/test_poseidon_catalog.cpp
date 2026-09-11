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

    // Same shape as the Celestial phased packet, but the category byte (index 9)
    // carries 2 = Poseidon and the member is the Poseidon pendant (13/202).
    constexpr std::array<std::uint8_t, 26> PoseidonPacket{
        0xC1, 26, 0xF3, 0xE7, 2, 2, 1, 1, 0, 2, 0x90, 1, 17, 1,
        0xCA, 0x1A, 2, 1, 1, 0, 0, 0x70, 0x41, 1, 0, 100
    };
}

TEST_CASE("E7 v2 accepts the Poseidon category and keeps it distinct from Ultimate")
{
    const auto catalog = Network::Equipment::DecodeCatalog(PoseidonPacket);
    REQUIRE(catalog.has_value());
    CHECK(catalog->ItemCategory == Category::Poseidon);
    CHECK(catalog->RequiredLevel == 400);
    CHECK(catalog->RequiredClass == 17);
    CHECK(catalog->Contains(13 * 512 + 202));

    // The Poseidon category is its own identity: not Ultimate, but the
    // level/class metadata still applies to members only.
    CHECK_FALSE(catalog->IsUltimate(13 * 512 + 202));
    CHECK(catalog->DisplayLevel(13 * 512 + 202, 0) == 400);
    CHECK(catalog->DisplayLevel(2 * 512 + 0, 80) == 80);
}

TEST_CASE("E7 v2 still rejects unknown categories")
{
    // 3 became a valid category with the Zeus set (update 249); anything
    // above it is still rejected.
    for (const auto category : {4, 0xFF})
    {
        auto packet = PoseidonPacket;
        packet[9] = static_cast<std::uint8_t>(category);
        CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    }
}

TEST_CASE("Catalog emitted by dotnet describes the full Poseidon set")
{
    const char* fixture = std::getenv("POSEIDON_CATALOG_FIXTURE");
    if (fixture == nullptr)
    {
        MESSAGE("Set POSEIDON_CATALOG_FIXTURE to check the actual dotnet Poseidon catalog sender.");
        return;
    }
    std::ifstream input(fixture, std::ios::binary);
    REQUIRE(input.good());
    const std::vector<std::uint8_t> bytes{ std::istreambuf_iterator<char>(input), {} };
    REQUIRE(bytes.size() == 125);
    const auto catalog = Network::Equipment::DecodeCatalog(bytes);
    REQUIRE(catalog.has_value());
    CHECK(catalog->RequiredItems == 12);
    CHECK(catalog->MemberCount == 12);
    CHECK(catalog->BonusCount == 11);
    CHECK(catalog->MinimumUpgrade == 0);
    CHECK(catalog->ItemCategory == Category::Poseidon);
    CHECK(catalog->RequiredLevel == 400);
    CHECK(catalog->RequiredClass == 17);
    CHECK(catalog->PhaseCount == 3);
    CHECK(catalog->Phases[0].MemberMask == 0x003E);
    CHECK(catalog->Phases[1].MemberMask == 0x03BE);
    CHECK(catalog->Phases[2].MemberMask == 0x0FFF);
    CHECK(catalog->Phases[0].Percent == 30);
    CHECK(catalog->Phases[1].Percent == 60);
    CHECK(catalog->Phases[2].Percent == 100);
    unsigned quantity = 0;
    for (std::size_t index = 0; index < catalog->MemberCount; ++index)
        quantity += catalog->Members[index].Quantity;
    CHECK(quantity == 12);
    // Trident, the five armor pieces at index 75, cape, jewels and pets.
    CHECK(catalog->Members[0].ItemType == 2 * 512 + 20);
    CHECK(catalog->Members[1].ItemType == 7 * 512 + 75);
    CHECK(catalog->Members[2].ItemType == 8 * 512 + 75);
    CHECK(catalog->Members[3].ItemType == 9 * 512 + 75);
    CHECK(catalog->Members[4].ItemType == 10 * 512 + 75);
    CHECK(catalog->Members[5].ItemType == 11 * 512 + 75);
    CHECK(catalog->Members[6].ItemType == 12 * 512 + 52);
    CHECK(catalog->Contains(13 * 512 + 202));
    CHECK(catalog->Contains(13 * 512 + 203));
    CHECK(catalog->Contains(13 * 512 + 204));
    CHECK(catalog->Contains(13 * 512 + 205));
    CHECK(catalog->Contains(13 * 512 + 206));
    // Bonus rows as emitted by update 248 (no magic speed row).
    const std::array<std::uint8_t, 11> kinds{ 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12 };
    const std::array<float, 11> values{ 15, 15, 10, 10, 8, 15, 10, 10, 20, 20, 15 };
    for (std::size_t index = 0; index < kinds.size(); ++index)
    {
        CHECK(catalog->Bonuses[index].Kind == static_cast<Character::Equipment::BonusKind>(kinds[index]));
        CHECK(catalog->Bonuses[index].Value == values[index]);
    }
}
