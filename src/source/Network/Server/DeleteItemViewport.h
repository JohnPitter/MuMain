#pragma once

// Wire parser for the ground-item removal packet (C2 0x21 ItemDropRemoved,
// Network/Packets ServerToClient: [C2][SizeH][SizeL][0x21][ItemCount]
// followed by ItemCount big-endian WORD drop ids). Pure and bounds-checked.
//
// Why the strictness: this packet is the end of every pickup-refusal cycle
// (ground-item owner protection answers C3 0x22 0xFF, and when the owner
// finally picks the drop -- or the drop expires -- every viewer receives this
// removal). The previous client handler iterated ItemCount entries off a raw
// pointer with no length validation (a short packet read past the heap
// buffer, undefined behavior on the main thread) and clamped out-of-range
// ids to slot 0, deleting an innocent ground item and poisoning the MU
// Helper's tracked drops. Every read here is clamped to the span and
// out-of-range ids are skipped, never remapped.
//
// Layout (packed -- matches ItemDropRemovedRef on the server):
//   +0 BYTE  Code (0xC2, 16-bit length family)
//   +1 BYTE  SizeH
//   +2 BYTE  SizeL
//   +3 BYTE  HeadCode (0x21)
//   +4 BYTE  ItemCount
//   +5 WORD  drop id, big-endian (repeated ItemCount times)

#include <cstdint>
#include <span>

namespace Network::Wire
{
/// <summary>
/// Header size of the C2 0x21 packet: 4 header bytes plus the item count byte.
/// </summary>
constexpr int kDeleteItemViewportHeaderLength = 5;

/// <summary>
/// Parses the ground-item removal packet and invokes <paramref name="onItemId"/>
/// for every entry whose id is a valid ground-item slot (0 &lt;= id &lt; maxItemId).
/// Out-of-range ids are skipped, never remapped to another slot. Never reads
/// past <paramref name="data"/>: entries that would extend beyond the span are
/// ignored and reported through <paramref name="outTruncated"/>.
/// </summary>
/// <param name="data">The full packet bytes.</param>
/// <param name="maxItemId">Exclusive upper bound of valid drop ids (the client's ground-item table size).</param>
/// <param name="onItemId">Called with each in-range id, in wire order.</param>
/// <param name="outSkipped">Optional; receives the number of out-of-range ids.</param>
/// <returns><c>false</c> only when the packet is shorter than the header, in which case nothing is delivered.</returns>
template <typename TCallback>
bool ParseDeleteItemViewport(std::span<const std::uint8_t> data, int maxItemId, TCallback&& onItemId, int* outSkipped = nullptr)
{
    if (outSkipped != nullptr)
    {
        *outSkipped = 0;
    }

    if (data.size() < static_cast<std::size_t>(kDeleteItemViewportHeaderLength))
    {
        return false;
    }

    const auto declaredCount = static_cast<std::size_t>(data[4]);
    const auto payloadBytes = data.size() - kDeleteItemViewportHeaderLength;
    const auto consumableCount = payloadBytes / 2 < declaredCount ? payloadBytes / 2 : declaredCount;

    int skipped = 0;
    std::size_t offset = kDeleteItemViewportHeaderLength;
    for (std::size_t i = 0; i < consumableCount; ++i, offset += 2)
    {
        const auto id = static_cast<int>((static_cast<int>(data[offset]) << 8) | data[offset + 1]);
        if (id < 0 || id >= maxItemId)
        {
            ++skipped;
            continue;
        }

        onItemId(id);
    }

    if (outSkipped != nullptr)
    {
        *outSkipped = skipped;
    }

    return true;
}
}
