#include "doctest.h"

#include "UI/NewUI/Inventory/TradeAnswerLayout.h"

#include <cstring>
#include <vector>

// The OpenMU server answers a trade request with a C1 0x37 packet of 20 bytes.
// The partner's total level rides little-endian in the two reserved bytes at
// offset 14-15 -- the same trick as the party list level -- so the packet size
// stays unchanged and the native PTRADE reader needs no byte swap. These tests
// pin the wire contract the trade dialog relies on.

namespace
{
std::vector<std::uint8_t> build_answer(std::uint8_t accepted, const char* name, std::uint16_t level, std::uint32_t guildKey)
{
    std::vector<std::uint8_t> packet(trade_answer::kPacketBytes, 0);
    packet[0] = 0xC1;
    packet[1] = static_cast<std::uint8_t>(trade_answer::kPacketBytes);
    packet[2] = trade_answer::kCode;
    packet[trade_answer::kAcceptedOffset] = accepted;
    for (int i = 0; name != nullptr && name[i] != '\0'; ++i)
    {
        packet[trade_answer::kNameOffset + i] = static_cast<std::uint8_t>(name[i]);
    }

    packet[trade_answer::kLevelOffset] = static_cast<std::uint8_t>(level & 0xFF);
    packet[trade_answer::kLevelOffset + 1] = static_cast<std::uint8_t>((level >> 8) & 0xFF);
    packet[trade_answer::kGuildOffset] = static_cast<std::uint8_t>(guildKey & 0xFF);
    packet[trade_answer::kGuildOffset + 1] = static_cast<std::uint8_t>((guildKey >> 8) & 0xFF);
    packet[trade_answer::kGuildOffset + 2] = static_cast<std::uint8_t>((guildKey >> 16) & 0xFF);
    packet[trade_answer::kGuildOffset + 3] = static_cast<std::uint8_t>((guildKey >> 24) & 0xFF);
    return packet;
}
}

TEST_CASE("accepted answer carries the real partner level little-endian")
{
    const auto packet = build_answer(1, "Parceiro", 350, 42);
    trade_answer::ParsedAnswer parsed{};
    REQUIRE(trade_answer::parse(packet.data(), static_cast<int>(packet.size()), &parsed));
    CHECK(parsed.Accepted == 1);
    CHECK(std::strcmp(parsed.Name, "Parceiro") == 0);
    CHECK(parsed.Level == 350);
    CHECK(parsed.GuildKey == 42);
}

TEST_CASE("level 400 and master totals do not byte-swap")
{
    const auto packet = build_answer(1, "Master", 400, 0);
    trade_answer::ParsedAnswer parsed{};
    REQUIRE(trade_answer::parse(packet.data(), static_cast<int>(packet.size()), &parsed));
    CHECK(parsed.Level == 400);

    const auto packet2 = build_answer(1, "Master", 1070, 0);
    trade_answer::ParsedAnswer parsed2{};
    REQUIRE(trade_answer::parse(packet2.data(), static_cast<int>(packet2.size()), &parsed2));
    CHECK(parsed2.Level == 1070);
}

TEST_CASE("rejected answer has subcode zero and empty name")
{
    const auto packet = build_answer(0, "", 0, 0);
    trade_answer::ParsedAnswer parsed{};
    REQUIRE(trade_answer::parse(packet.data(), static_cast<int>(packet.size()), &parsed));
    CHECK(parsed.Accepted == 0);
    CHECK(parsed.Name[0] == '\0');
    CHECK(parsed.Level == 0);
}

TEST_CASE("broken packets are refused")
{
    trade_answer::ParsedAnswer parsed{};

    SUBCASE("truncated packet")
    {
        const auto packet = build_answer(1, "P", 100, 0);
        CHECK_FALSE(trade_answer::parse(packet.data(), trade_answer::kPacketBytes - 1, &parsed));
    }

    SUBCASE("wrong code")
    {
        auto packet = build_answer(1, "P", 100, 0);
        packet[2] = 0x36;
        CHECK_FALSE(trade_answer::parse(packet.data(), static_cast<int>(packet.size()), &parsed));
    }

    SUBCASE("wrong header type")
    {
        auto packet = build_answer(1, "P", 100, 0);
        packet[0] = 0xC2;
        CHECK_FALSE(trade_answer::parse(packet.data(), static_cast<int>(packet.size()), &parsed));
    }

    SUBCASE("null buffer")
    {
        CHECK_FALSE(trade_answer::parse(nullptr, trade_answer::kPacketBytes, &parsed));
    }
}
