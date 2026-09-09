#pragma once

// Wire length of an item data block inside the extended item packets
// (get-item C1 0x22, full inventory list C2/C3 F3 0x10, equipment F3 0x13,
// item-moved F3 0x14...). Pure and bounds-checked.
//
// Why the strictness: the full-inventory resync (C2/C3 F3 0x10) used to run
// only once, at login. Since the server gained an inventory-request handler
// it can fire mid-game (a pickup whose slot the client view refuses asks for
// a resync), and it parses this function for every item of the reply. The
// previous implementation dereferenced the base-struct cast unchecked and
// read the socket byte past the span end, so a truncated or malformed reply
// was undefined behavior on the main thread. Every read here is clamped to
// the span and the returned length never exceeds it, so the callers'
// `itemData.subspan(0, length)` stays in range by construction.
//
// Layout (PITEM_EXTENDED_BASE, packed -- Network/Server/WSclient.h):
//   +0 WORD  GroupAndNumber
//   +2 BYTE  Level
//   +3 BYTE  Durability
//   +4 BYTE  OptionFlags
// The base block is 5 bytes. Each present option group adds one byte, and
// HasSockets adds one further byte for the socket header plus one byte per
// socket (socket count is the low nibble of the socket header byte).

#include <cstdint>
#include <span>

namespace Network::Wire
{
    // Option-flag bits that grow the block. Same values as ItemOptionFlags
    // (Core/Globals/_enum.h) -- kept local so this header stays includable
    // from the test tree without the globals header.
    inline constexpr std::uint8_t kFlagHasOption = 0x01;
    inline constexpr std::uint8_t kFlagHasExcellent = 0x08;
    inline constexpr std::uint8_t kFlagHasAncient = 0x10;
    inline constexpr std::uint8_t kFlagHasHarmony = 0x20;
    inline constexpr std::uint8_t kFlagHasSockets = 0x80;

    inline constexpr int kItemExtendedBaseLength = 5;

    // Returns the item block length in bytes, clamped to [0, itemData.size()].
    // A span shorter than the 5-byte base yields itemData.size(): the caller
    // subspans the whole (undersized) remainder and the item parse fails
    // gracefully downstream, instead of reading past the buffer.
    inline int CalcItemLength(std::span<const unsigned char> itemData) noexcept
    {
        const int available = static_cast<int>(itemData.size());
        if (available < kItemExtendedBaseLength)
        {
            return available > 0 ? available : 0;
        }

        int size = kItemExtendedBaseLength;
        const std::uint8_t optionFlags = itemData[4];
        if (optionFlags & kFlagHasOption)
        {
            ++size;
        }
        if (optionFlags & kFlagHasExcellent)
        {
            ++size;
        }
        if (optionFlags & kFlagHasAncient)
        {
            ++size;
        }
        if (optionFlags & kFlagHasHarmony)
        {
            ++size;
        }
        if (optionFlags & kFlagHasSockets)
        {
            // The socket header byte is the first byte after the base: its
            // low nibble is the socket count, the whole byte is consumed.
            if (size < available)
            {
                size += itemData[size] & 0x0F;
            }
            ++size;
        }

        return size <= available ? size : available;
    }
}
