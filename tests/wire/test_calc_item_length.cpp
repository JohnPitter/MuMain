#include "doctest.h"

#include "Network/Server/CalcItemLength.h"

#include <cstdint>
#include <vector>

using Network::Wire::CalcItemLength;
using Network::Wire::kFlagHasAncient;
using Network::Wire::kFlagHasExcellent;
using Network::Wire::kFlagHasHarmony;
using Network::Wire::kFlagHasOption;
using Network::Wire::kFlagHasSockets;
using Network::Wire::kItemExtendedBaseLength;

namespace
{
// A complete item block: WORD GroupAndNumber + Level + Durability + OptionFlags
// plus enough trailing payload for every option group (the option bytes and a
// socket header with nibble 0x0F would need at most 1 + 1 + 1 + 1 + 1 + 15).
std::vector<unsigned char> Block(std::uint8_t optionFlags)
{
    std::vector<unsigned char> item{0x0A, 0x00, 0x05, 0x0C, optionFlags};
    for (int i = 0; i < 20; ++i)
        item.push_back(0x00);
    return item;
}

// A complete item block whose option payload starts right after the base
// (index 5), with room to spare so a full-block parse never clamps.
std::vector<unsigned char> WithTail(std::uint8_t optionFlags, std::vector<unsigned char> tail)
{
    std::vector<unsigned char> item{0x0A, 0x00, 0x05, 0x0C, optionFlags};
    item.insert(item.end(), tail.begin(), tail.end());
    for (int i = 0; i < 8; ++i)
        item.push_back(0x00);
    return item;
}
}

TEST_CASE("plain item block is exactly the 5-byte base")
{
    CHECK(CalcItemLength(Block(0x00)) == 5);
}

TEST_CASE("each present option group adds one byte")
{
    CHECK(CalcItemLength(Block(kFlagHasOption)) == 6);
    CHECK(CalcItemLength(Block(kFlagHasExcellent)) == 6);
    CHECK(CalcItemLength(Block(kFlagHasAncient)) == 6);
    CHECK(CalcItemLength(Block(kFlagHasHarmony)) == 6);
    CHECK(CalcItemLength(Block(kFlagHasOption | kFlagHasExcellent | kFlagHasAncient | kFlagHasHarmony)) == 9);
}

TEST_CASE("sockets add the header byte plus one byte per socket")
{
    // Socket header byte 0x03: low nibble says 3 sockets.
    auto item = WithTail(kFlagHasSockets, {0x03, 0x01, 0x02, 0x03});
    CHECK(CalcItemLength(item) == 5 + 1 + 3);
}

TEST_CASE("socket count is the low nibble only")
{
    // High nibble of the socket header (bonus slot flags) must not leak into
    // the length.
    auto item = WithTail(kFlagHasSockets, {0xF2, 0x01, 0x02});
    CHECK(CalcItemLength(item) == 5 + 1 + 2);
}

TEST_CASE("length never exceeds the span -- truncated socket block clamps")
{
    // The old implementation read the socket byte past the span end and
    // subspanned out of range on exactly this shape: a reply cut short
    // mid-item (truncated full-inventory resync).
    auto full = WithTail(kFlagHasSockets, {0x0F, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
    for (std::size_t cut = 0; cut <= full.size(); ++cut)
    {
        std::span<const unsigned char> view(full.data(), cut);
        const int length = CalcItemLength(view);
        CHECK(length >= 0);
        CHECK(length <= static_cast<int>(cut));
    }
}

TEST_CASE("complete base with an option byte missing from the span clamps")
{
    // Base(HasOption) claims 6 bytes but the span carries only 5: the caller
    // gets the whole (undersized) remainder instead of an out-of-range count.
    std::vector<unsigned char> truncated{0x0A, 0x00, 0x05, 0x0C, kFlagHasOption};
    CHECK(CalcItemLength(truncated) == 5);
}

TEST_CASE("undersized spans clamp to the span itself")
{
    const std::vector<unsigned char> tiny{0x01, 0x02, 0x03};
    for (std::size_t size = 0; size <= tiny.size(); ++size)
    {
        std::span<const unsigned char> view(tiny.data(), size);
        CHECK(CalcItemLength(view) == static_cast<int>(size));
    }
}

TEST_CASE("empty span yields zero")
{
    CHECK(CalcItemLength({}) == 0);
}

TEST_CASE("base length constant matches the packed wire layout")
{
    // WORD + BYTE + BYTE + BYTE (PITEM_EXTENDED_BASE).
    CHECK(kItemExtendedBaseLength == 5);
}
