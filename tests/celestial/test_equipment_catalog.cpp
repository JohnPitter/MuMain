#include "doctest.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <string_view>
#include <vector>
#include "Character/EquipmentCatalogCache.h"
#include "Network/Server/EquipmentBonusCatalogPacket.h"
#include "UI/Items/EquipmentTooltipContent.h"

namespace
{
    using Character::Equipment::BonusCatalog;
    using Character::Equipment::BonusKind;
    using Character::Equipment::BonusUnit;
    using UI::Items::EquipmentTooltip::Line;
    using UI::Items::EquipmentTooltip::LineRole;
    using UI::Items::EquipmentTooltip::Strings;

    constexpr std::array<std::uint8_t, 18> MinimalPacket{
        0xC1, 18, 0xF3, 0xE7, 1, 2, 1, 1, 0,
        0xC8, 0x1A, 2, 1, 1, 0, 0, 0x70, 0x41
    };
    constexpr std::array<std::uint8_t, 9> EmptyPacket{ 0xC1, 9, 0xF3, 0xE7, 1, 0, 0, 0, 0 };

    constexpr Strings Text{
        L"Conjunto Celestial", L"Requer %u equipamentos", L"Upgrade: +%u",
        L"Seu conjunto: ativo", L"Seu conjunto: inativo", L"Aguardando servidor",
        L"Inclui acessórios", L"Não altera poções", L"Limites do servidor",
        { L"", L"Dano", L"Defesa", L"Vida máxima", L"Mana máxima", L"Ataque", L"Magia",
          L"Movimento", L"Ignorar defesa", L"Crítico", L"Recuperação de vida", L"Recuperação de mana", L"Redução elemental" },
        { L"", L"%", L" p.p." }
    };

    std::array<std::uint8_t, 18> WithValue(float value)
    {
        auto packet = MinimalPacket;
        const auto bits = std::bit_cast<std::uint32_t>(value);
        for (unsigned index = 0; index < sizeof(value); ++index)
            packet[14 + index] = static_cast<std::uint8_t>(bits >> (index * 8));
        return packet;
    }

    BonusCatalog FullCatalog()
    {
        auto catalog = *Network::Equipment::DecodeCatalog(MinimalPacket);
        catalog.BonusCount = 12;
        for (std::size_t index = 0; index < catalog.BonusCount; ++index)
            catalog.Bonuses[index] = { static_cast<BonusKind>(index + 1), BonusUnit::Percent, 15 };
        return catalog;
    }
}

TEST_CASE("Catalog decodes quantities and never classifies unrelated items")
{
    const auto catalog = Network::Equipment::DecodeCatalog(MinimalPacket);
    REQUIRE(catalog.has_value());
    CHECK(catalog->Known);
    CHECK(catalog->RequiredItems == 2);
    CHECK(catalog->Members[0].Quantity == 2);
    CHECK(catalog->Contains(13 * 512 + 200));
    CHECK_FALSE(catalog->Contains(13 * 512 + 90));
    CHECK_FALSE(catalog->Contains(5 * 512 + 10));
    CHECK_FALSE(catalog->Contains(-1));
    CHECK(catalog->Bonuses[0].Kind == BonusKind::Damage);
    CHECK(catalog->Bonuses[0].Unit == BonusUnit::Percent);
    CHECK(catalog->Bonuses[0].Value == 15);
}

TEST_CASE("Catalog requires exact length version and bounded counts")
{
    for (std::size_t size = 0; size < MinimalPacket.size(); ++size)
        CHECK_FALSE(Network::Equipment::DecodeCatalog(std::span(MinimalPacket).first(size)));
    std::array<std::uint8_t, 19> extra{};
    std::copy(MinimalPacket.begin(), MinimalPacket.end(), extra.begin());
    CHECK_FALSE(Network::Equipment::DecodeCatalog(extra));
    for (const auto offset : { 0, 1, 2, 3, 4, 5, 6, 7, 8 })
    {
        auto packet = MinimalPacket;
        packet[offset] = 0xFF;
        CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    }
    auto packet = MinimalPacket;
    packet[6] = 0;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    packet = MinimalPacket;
    packet[7] = 0;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
}

TEST_CASE("Catalog rejects malformed members and ambiguous repeated definitions")
{
    auto packet = MinimalPacket;
    packet[11] = 0;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    packet = MinimalPacket;
    packet[10] = 0x20;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    std::array<std::uint8_t, 21> repeated{
        0xC1, 21, 0xF3, 0xE7, 1, 2, 2, 1, 0,
        0xC8, 0x1A, 1, 0xC8, 0x1A, 1, 1, 1, 0, 0, 0x70, 0x41
    };
    CHECK_FALSE(Network::Equipment::DecodeCatalog(repeated));
    repeated[12] = 0xC9;
    CHECK(Network::Equipment::DecodeCatalog(repeated).has_value());
}

TEST_CASE("Catalog rejects unknown bonus kinds units and nonfinite or excessive values")
{
    for (auto kind : { 0, 13, 255 })
    {
        auto packet = MinimalPacket;
        packet[12] = static_cast<std::uint8_t>(kind);
        CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    }
    auto packet = MinimalPacket;
    packet[13] = 3;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    for (float value : { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(), 1'000'001.0f, -1'000'001.0f })
        CHECK_FALSE(Network::Equipment::DecodeCatalog(WithValue(value)));
    for (float value : { 0.0f, -15.0f, 1'000'000.0f, -1'000'000.0f })
        CHECK(Network::Equipment::DecodeCatalog(WithValue(value)).has_value());
}

TEST_CASE("Catalog reset and an empty channel discard the previous equipment description")
{
    using namespace Character::Equipment;
    ResetCatalog();
    CHECK_FALSE(GetCatalog().Known);
    SetCatalog(*Network::Equipment::DecodeCatalog(MinimalPacket));
    CHECK(GetCatalog().Contains(6856));
    const auto empty = Network::Equipment::DecodeCatalog(EmptyPacket);
    REQUIRE(empty.has_value());
    SetCatalog(*empty);
    CHECK(GetCatalog().Known);
    CHECK(GetCatalog().BonusCount == 0);
    CHECK_FALSE(GetCatalog().Contains(6856));
    SetCatalog(*Network::Equipment::DecodeCatalog(MinimalPacket));
    ResetCatalog();
    CHECK_FALSE(GetCatalog().Known);
    CHECK_FALSE(GetCatalog().Contains(6856));
}

TEST_CASE("Detailed tooltip uses server amounts and distinguishes percent from percentage points")
{
    auto catalog = FullCatalog();
    catalog.RequiredItems = 11;
    catalog.Bonuses[0].Value = 25;
    catalog.Bonuses[1] = { BonusKind::Defense, BonusUnit::Flat, 23 };
    catalog.Bonuses[8] = { BonusKind::CriticalChance, BonusUnit::PercentagePoints, 10 };
    std::array<Line, UI::Items::EquipmentTooltip::MaximumTooltipLines> lines{};
    const auto count = BuildLines(catalog, { true, true, 1.15f }, Text, lines);
    REQUIRE(count == 18);
    CHECK(std::wstring_view(lines[1].Text.data()) == L"Requer 11 equipamentos");
    CHECK(lines[2].Role == LineRole::Active);
    CHECK(std::wstring_view(lines[3].Text.data()) == L"Dano: +25%");
    CHECK(std::wstring_view(lines[4].Text.data()) == L"Defesa: +23");
    CHECK(std::wstring_view(lines[11].Text.data()) == L"Crítico: +10 p.p.");
    CHECK(lines[count - 1].Role == LineRole::Note);
    catalog.Bonuses[0].Value = -12.5f;
    BuildLines(catalog, { true, false, 1 }, Text, lines);
    CHECK(std::wstring_view(lines[3].Text.data()) == L"Dano: -12.5%");
}

TEST_CASE("Detailed tooltip never infers activation from catalog membership or movement")
{
    const auto catalog = FullCatalog();
    std::array<Line, UI::Items::EquipmentTooltip::MaximumTooltipLines> lines{};
    BuildLines(catalog, { true, false, 1.15f }, Text, lines);
    CHECK(std::wstring_view(lines[2].Text.data()) == Text.Inactive);
    CHECK(lines[3].Role == LineRole::Inactive);
    BuildLines(catalog, { false, true, 1.15f }, Text, lines);
    CHECK(std::wstring_view(lines[2].Text.data()) == Text.Unknown);
    CHECK(lines[2].Role == LineRole::Inactive);
    BuildLines(catalog, { true, true, 0.575f }, Text, lines);
    CHECK(std::wstring_view(lines[2].Text.data()) == Text.Active);
    CHECK(lines[3].Role == LineRole::Detail);
}

TEST_CASE("Detailed tooltip remains bounded with maximum rows and optional upgrade")
{
    auto catalog = FullCatalog();
    catalog.BonusCount = 16;
    catalog.MinimumUpgrade = 15;
    for (std::size_t index = 12; index < catalog.BonusCount; ++index)
        catalog.Bonuses[index] = catalog.Bonuses[0];
    std::array<Line, UI::Items::EquipmentTooltip::MaximumTooltipLines> lines{};
    lines.back().Text[0] = L'!';
    const auto count = BuildLines(catalog, { true, true, 1 }, Text, lines);
    CHECK(count == 23);
    CHECK(std::wstring_view(lines[2].Text.data()) == L"Upgrade: +15");
    CHECK(lines.back().Text[0] == L'!');
    lines.front().Text[0] = L'?';
    CHECK(BuildLines(catalog, {}, Text, std::span(lines).first(22)) == 0);
    CHECK(lines.front().Text[0] == L'?');
    catalog.BonusCount = 17;
    CHECK(BuildLines(catalog, {}, Text, lines) == 0);
    catalog = {};
    CHECK(BuildLines(catalog, {}, Text, lines) == 0);
    CHECK(BuildLines(*Network::Equipment::DecodeCatalog(EmptyPacket), {}, Text, lines) == 0);
}

TEST_CASE("Catalog emitted by dotnet carries all ten definitions and twelve real bonus rows")
{
    const char* fixture = std::getenv("CELESTIAL_CATALOG_FIXTURE");
    if (fixture == nullptr)
    {
        MESSAGE("Set CELESTIAL_CATALOG_FIXTURE to check the actual dotnet catalog sender.");
        return;
    }
    std::ifstream input(fixture, std::ios::binary);
    REQUIRE(input.good());
    const std::vector<std::uint8_t> bytes{ std::istreambuf_iterator<char>(input), {} };
    REQUIRE(bytes.size() == 125);
    const auto catalog = Network::Equipment::DecodeCatalog(bytes);
    REQUIRE(catalog.has_value());
    CHECK(catalog->RequiredItems == 11);
    CHECK(catalog->MemberCount == 10);
    CHECK(catalog->BonusCount == 12);
    CHECK(catalog->MinimumUpgrade == 0);
    CHECK(catalog->ItemCategory == Character::Equipment::Category::Ultimate);
    CHECK(catalog->RequiredLevel == 400);
    CHECK(catalog->RequiredClass == 3);
    CHECK(catalog->PhaseCount == 3);
    CHECK(catalog->Phases[0].MemberMask == 0x007C);
    CHECK(catalog->Phases[1].MemberMask == 0x007F);
    CHECK(catalog->Phases[2].MemberMask == 0x03FF);
    CHECK(catalog->Phases[0].Percent == 30);
    CHECK(catalog->Phases[1].Percent == 60);
    CHECK(catalog->Phases[2].Percent == 100);
    unsigned quantity = 0;
    for (std::size_t index = 0; index < catalog->MemberCount; ++index)
        quantity += catalog->Members[index].Quantity;
    CHECK(quantity == 11);
    CHECK(catalog->Members[8].ItemType == 6856);
    CHECK(catalog->Members[8].Quantity == 2);
    CHECK(catalog->Contains(5 * 512 + 37));
    CHECK(catalog->Contains(13 * 512 + 201));
    const std::array<float, 12> values{ 15, 15, 10, 10, 8, 8, 15, 5, 10, 20, 20, 10 };
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        CHECK(catalog->Bonuses[index].Kind == static_cast<BonusKind>(index + 1));
        CHECK(catalog->Bonuses[index].Value == values[index]);
        const auto unit = index == 7 || index == 8 || index == 11 ? BonusUnit::PercentagePoints : BonusUnit::Percent;
        CHECK(catalog->Bonuses[index].Unit == unit);
    }
    std::array<Line, UI::Items::EquipmentTooltip::MaximumTooltipLines> lines{};
    CHECK(BuildLines(*catalog, { true, true, 1.15f }, Text, lines) == 18);
    CHECK(std::wstring_view(lines[11].Text.data()) == L"Crítico: +10 p.p.");
}

namespace
{
    constexpr std::array<std::uint8_t, 26> UltimatePacket{
        0xC1, 26, 0xF3, 0xE7, 2, 2, 1, 1, 0, 1, 0x90, 1, 3, 1,
        0xC8, 0x1A, 2, 1, 1, 0, 0, 0x70, 0x41, 1, 0, 100
    };

    Strings PhaseText()
    {
        auto text = Text;
        text.PhaseStatus = L"Ativo: %u%%";
        text.PhaseTemplate = L"Fase %u: %u%% %ls";
        text.PhaseLabels = { L"armadura", L"+ armas", L"+ acessórios" };
        text.PhaseCondition = L"Ativo / total";
        return text;
    }
}

TEST_CASE("Ultimate metadata fixes presentation without changing unrelated or legacy items")
{
    const auto catalog = Network::Equipment::DecodeCatalog(UltimatePacket);
    REQUIRE(catalog.has_value());
    for (unsigned legacyLevel : { 0u, 400u, 420u, 460u, 480u })
    {
        CHECK(catalog->DisplayLevel(6856, legacyLevel) == 400);
        CHECK(catalog->DisplayLevel(5 * 512 + 10, legacyLevel) == legacyLevel);
    }
    CHECK(catalog->IsUltimate(6856));
    CHECK_FALSE(catalog->IsUltimate(5 * 512 + 10));
    CHECK_FALSE(Network::Equipment::DecodeCatalog(MinimalPacket)->IsUltimate(6856));
    for (std::size_t size = 0; size < UltimatePacket.size(); ++size)
        CHECK_FALSE(Network::Equipment::DecodeCatalog(std::span(UltimatePacket).first(size)));
}

TEST_CASE("Phased metadata rejects invalid category class masks and incomplete stages")
{
    for (auto index : { 9, 12, 13, 23, 24, 25 })
    {
        auto packet = UltimatePacket;
        packet[index] = 0xFF;
        CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    }
    auto packet = UltimatePacket;
    packet[25] = 30;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
    packet = UltimatePacket;
    packet[23] = 0;
    CHECK_FALSE(Network::Equipment::DecodeCatalog(packet));
}

TEST_CASE("Phased tooltip scales every server bonus and shows cumulative milestones")
{
    auto catalog = *Network::Equipment::DecodeCatalog(UltimatePacket);
    catalog.PhaseCount = 3;
    catalog.Phases = { Character::Equipment::BonusPhase{ 1, 30 }, { 3, 60 }, { 7, 100 } };
    std::array<Line, UI::Items::EquipmentTooltip::MaximumTooltipLines> lines{};
    for (auto percent : { 0, 30, 60, 100 })
    {
        const Character::Equipment::State state{ true, percent > 0, 1.f, static_cast<std::uint8_t>(percent) };
        CHECK(BuildLines(catalog, state, PhaseText(), lines) == 10);
        for (std::size_t index = 0; index < catalog.PhaseCount; ++index)
            CHECK((lines[3 + index].Role == LineRole::Active) == (percent >= catalog.Phases[index].Percent));
    }
    BuildLines(catalog, { true, true, 1.045f, 30 }, PhaseText(), lines);
    CHECK(std::wstring_view(lines[2].Text.data()) == L"Ativo: 30%");
    CHECK(std::wstring_view(lines[6].Text.data()) == L"Dano: +4.5% / +15%");
    BuildLines(catalog, { true, true, 1.09f, 60 }, PhaseText(), lines);
    CHECK(std::wstring_view(lines[6].Text.data()) == L"Dano: +9% / +15%");
    BuildLines(catalog, {}, PhaseText(), lines);
    CHECK(std::wstring_view(lines[2].Text.data()) == Text.Unknown);
    CHECK(std::wstring_view(lines[6].Text.data()) == L"Dano: +0% / +15%");
}
