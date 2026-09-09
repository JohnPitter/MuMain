#pragma once

// Pure parsing for the MU trade request answer (C1 0x37).
//
// OpenMU answers a trade request with this 20-byte packet: the Accepted byte
// (0 = rejected/canceled, 1 = accepted, 2 = cannot trade), the partner name,
// the partner's total level and the guild id. The level travels in the two
// formerly reserved bytes at offset 14-15, little-endian, so the packet size
// stays unchanged -- the same trick as the party list (0x42) level.
//
// The client's native reader is the PTRADE struct (WSclient.h), which reads
// the same fields little-endian. static_asserts in WSclient.h pin the struct
// to these constants, and tests/trade/test_trade_answer_layout.cpp exercises
// the parser without a running game.

#include <cstdint>

namespace trade_answer
{
// Wire layout -- keep in sync with TradeRequestAnswer (ServerToClientPackets.xml).
inline constexpr std::uint8_t kCode = 0x37;
inline constexpr int kAcceptedOffset = 3;   // 0 = rejected/canceled, 1 = accepted, 2 = cannot trade
inline constexpr int kNameOffset = 4;
inline constexpr int kNameBytes = 10;
inline constexpr int kLevelOffset = 14;     // two reserved bytes, little-endian
inline constexpr int kGuildOffset = 16;
inline constexpr int kPacketBytes = 20;

struct ParsedAnswer
{
    std::uint8_t Accepted;      // SubCode in the client's PTRADE
    char Name[kNameBytes + 1];  // NUL-terminated copy of the raw (UTF-8) name bytes
    std::uint16_t Level;        // little-endian total level, as PTRADE::Level reads it
    std::uint32_t GuildKey;
};

// Parses a C1 0x37 packet into out. Returns false when the packet is broken
// (short or wrong header) and the trade must not be opened. A trailing
// garbage-free name is NUL-terminated; padding bytes are stripped.
inline bool parse(const std::uint8_t* buffer, int size, ParsedAnswer* out)
{
    if (buffer == nullptr || out == nullptr || size < kPacketBytes)
    {
        return false;
    }

    if (buffer[0] != 0xC1 || buffer[2] != kCode)
    {
        return false;
    }

    out->Accepted = buffer[kAcceptedOffset];

    int nameLength = kNameBytes;
    while (nameLength > 0 && buffer[kNameOffset + nameLength - 1] == 0)
    {
        --nameLength;
    }

    for (int i = 0; i < nameLength; ++i)
    {
        out->Name[i] = static_cast<char>(buffer[kNameOffset + i]);
    }

    out->Name[nameLength] = '\0';

    out->Level = static_cast<std::uint16_t>(
        buffer[kLevelOffset] | (static_cast<std::uint16_t>(buffer[kLevelOffset + 1]) << 8));
    out->GuildKey = static_cast<std::uint32_t>(buffer[kGuildOffset])
        | (static_cast<std::uint32_t>(buffer[kGuildOffset + 1]) << 8)
        | (static_cast<std::uint32_t>(buffer[kGuildOffset + 2]) << 16)
        | (static_cast<std::uint32_t>(buffer[kGuildOffset + 3]) << 24);
    return true;
}
}
