#pragma once

// Pure parsing for the LuxView wedding proposal request (C1 F3 EE).
//
// The server (MUnique.OpenMU.GameServer.RemoteView.Wedding.ShowWeddingRequestPlugIn)
// pushes this LuxView-only packet when another player proposes marriage; it
// replaces the previously reused party invite request so the Yes/No dialog can
// show wedding wording instead of the party invite text. The answer still
// travels as the native party invite response packet -- the server routes it by
// the WeddingRequest player state. Everything here is a pure function of its
// inputs so the parser can be unit tested
// (tests/wedding/test_wedding_request_layout.cpp) without a running game.

#include <cstddef>
#include <cstdint>

namespace wedding_request
{
// Wire limits — keep in sync with ShowWeddingRequestPlugIn.cs on the server.
inline constexpr std::uint8_t kGroup = 0xF3;
inline constexpr std::uint8_t kSubCode = 0xEE;
inline constexpr int kHeaderBytes = 7;      // C1 + length + group + sub-code + 2-byte id + name length
inline constexpr int kNameBytes = 10;       // the server truncates to the character name limit

struct ParsedRequest
{
    std::uint16_t ProponentId;              // echoed back in the party invite response (server ignores it)
    wchar_t Name[kNameBytes + 1];           // NUL-terminated, never overruns
};

// Minimal UTF-8 -> UTF-16 for the name. Handles 1-3 byte sequences (BMP);
// malformed leads and 4-byte sequences become '?' instead of derailing the
// walk. Always NUL-terminates, never overruns dstSize.
inline void utf8_to_wide(const std::uint8_t* src, int length, wchar_t* dst, int dstSize)
{
    if (dstSize <= 0)
    {
        return;
    }

    int out = 0;
    int i = 0;
    while (i < length && out < dstSize - 1)
    {
        const std::uint8_t lead = src[i];
        wchar_t code = L'?';
        int sequence = 1;
        if (lead < 0x80)
        {
            code = static_cast<wchar_t>(lead);
        }
        else if ((lead & 0xE0) == 0xC0 && i + 1 < length && (src[i + 1] & 0xC0) == 0x80)
        {
            code = static_cast<wchar_t>(((lead & 0x1Fu) << 6) | (src[i + 1] & 0x3Fu));
            sequence = 2;
        }
        else if ((lead & 0xF0) == 0xE0 && i + 2 < length
            && (src[i + 1] & 0xC0) == 0x80 && (src[i + 2] & 0xC0) == 0x80)
        {
            code = static_cast<wchar_t>(((lead & 0x0Fu) << 12)
                | ((src[i + 1] & 0x3Fu) << 6)
                | (src[i + 2] & 0x3Fu));
            sequence = 3;
        }

        dst[out++] = code;
        i += sequence;
    }

    dst[out] = L'\0';
}

// Parses a C1 F3 EE packet into out. Returns true when the header is intact
// (the name is then filled, possibly empty); false when the packet is broken
// and the dialog must not be shown. A truncated name is tolerated: the parse
// trusts the actual payload, not the announced length.
inline bool parse_request(const std::uint8_t* buffer, int size, ParsedRequest* out)
{
    if (buffer == nullptr || out == nullptr || size < kHeaderBytes)
    {
        return false;
    }

    if (buffer[0] != 0xC1 || buffer[2] != kGroup || buffer[3] != kSubCode)
    {
        return false;
    }

    out->ProponentId = static_cast<std::uint16_t>((buffer[4] << 8) | buffer[5]);
    int nameLength = buffer[6];
    if (nameLength > kNameBytes)
    {
        nameLength = kNameBytes;
    }

    if (kHeaderBytes + nameLength > size)
    {
        nameLength = size - kHeaderBytes;
    }

    utf8_to_wide(buffer + kHeaderBytes, nameLength, out->Name, kNameBytes + 1);
    return true;
}
}
