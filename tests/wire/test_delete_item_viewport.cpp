#include "doctest.h"

#include "Network/Server/DeleteItemViewport.h"

#include <cstdint>
#include <vector>

using Network::Wire::ParseDeleteItemViewport;

namespace
{
// The client's ground-item table size (MAX_ITEMS, Core/Globals/_define.h).
constexpr int kMaxItems = 1000;

// A well-formed C2 0x21: [C2][SizeH][SizeL][0x21][count][id big-endian]...
std::vector<std::uint8_t> Packet(const std::vector<int>& ids)
{
    std::vector<std::uint8_t> packet{0xC2, 0x00, 0x00, 0x21, static_cast<std::uint8_t>(ids.size())};
    for (const int id : ids)
    {
        packet.push_back(static_cast<std::uint8_t>((id >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(id & 0xFF));
    }

    const auto size = packet.size();
    packet[1] = static_cast<std::uint8_t>((size >> 8) & 0xFF);
    packet[2] = static_cast<std::uint8_t>(size & 0xFF);
    return packet;
}

std::vector<int> Collect(const std::span<const std::uint8_t> data, int* outSkipped = nullptr, bool* outOk = nullptr)
{
    std::vector<int> delivered;
    const bool ok = ParseDeleteItemViewport(data, kMaxItems, [&](const int id) { delivered.push_back(id); }, outSkipped);
    if (outOk != nullptr)
    {
        *outOk = ok;
    }
    return delivered;
}
}

TEST_CASE("valid single-entry packet delivers the id")
{
    const auto packet = Packet({7});
    bool ok = false;
    const auto delivered = Collect(packet, nullptr, &ok);
    CHECK(ok);
    CHECK(delivered == std::vector<int>{7});
}

TEST_CASE("valid batch delivers every id in wire order")
{
    const auto packet = Packet({0, 7, 255, 999});
    int skipped = -1;
    const auto delivered = Collect(packet, &skipped);
    CHECK(delivered == std::vector<int>{0, 7, 255, 999});
    CHECK(skipped == 0);
}

TEST_CASE("empty packet (count 0) delivers nothing and is not truncated")
{
    const auto packet = Packet({});
    bool ok = false;
    const auto delivered = Collect(packet, nullptr, &ok);
    CHECK(ok);
    CHECK(delivered.empty());
}

TEST_CASE("out-of-range ids are skipped, never clamped to slot 0")
{
    // 1000 == kMaxItems (first invalid), 0xFFFF (uninitialized memory pattern):
    // the old handler redirected both to slot 0 and deleted an innocent drop.
    const auto packet = Packet({1000, 5, 0xFFFF, 6});
    int skipped = 0;
    const auto delivered = Collect(packet, &skipped);
    CHECK(delivered == std::vector<int>{5, 6});
    CHECK(skipped == 2);
}

TEST_CASE("declared count beyond the payload clamps to the span")
{
    auto packet = Packet({7, 8, 9});
    packet[4] = 30; // claims 30 entries, payload carries 3
    int skipped = 0;
    const auto delivered = Collect(packet, &skipped);
    CHECK(delivered == std::vector<int>{7, 8, 9});
    CHECK(skipped == 0);
}

TEST_CASE("odd trailing byte cannot push a read past the span")
{
    std::vector<std::uint8_t> packet = Packet({7, 8});
    packet.push_back(0xAA); // half an entry
    packet[4] = 3;          // claims 3 entries, only 2 complete + 1 orphan byte
    const auto delivered = Collect(packet);
    CHECK(delivered == std::vector<int>{7, 8});
}

TEST_CASE("header-truncated packets deliver nothing and report failure")
{
    const auto full = Packet({7});
    for (std::size_t cut = 0; cut < Network::Wire::kDeleteItemViewportHeaderLength; ++cut)
    {
        std::span<const std::uint8_t> view(full.data(), cut);
        bool ok = true;
        int skipped = -1;
        const auto delivered = Collect(view, &skipped, &ok);
        CHECK_FALSE(ok);
        CHECK(delivered.empty());
        CHECK(skipped == 0);
    }
}

TEST_CASE("every prefix of a valid packet parses without delivering an out-of-range id")
{
    const auto full = Packet({3, 1000, 4});
    for (std::size_t cut = 0; cut <= full.size(); ++cut)
    {
        std::span<const std::uint8_t> view(full.data(), cut);
        int skipped = 0;
        const auto delivered = Collect(view, &skipped);
        for (const int id : delivered)
        {
            CHECK(id >= 0);
            CHECK(id < kMaxItems);
        }
        if (cut < Network::Wire::kDeleteItemViewportHeaderLength)
        {
            CHECK(delivered.empty());
        }
    }
}

TEST_CASE("refusal-cycle spam: mixed removal floods parse with state intact")
{
    // The regression shape: a player spams SPACE on a protected drop; every
    // cycle ends with removal packets for the expired/reused drops. Ten
    // thousand mixed packets -- valid, truncated, out-of-range -- must parse
    // total (no crash, no OOB) and deliver exactly the in-range ids.
    std::vector<int> delivered;
    int skippedTotal = 0;
    for (int round = 0; round < 1000; ++round)
    {
        const auto valid = Packet({round % kMaxItems});              // 1 id
        const auto batch = Packet({0, 42, 999});                     // 3 ids
        auto lying = Packet({1, 2, 3});
        lying[4] = 99;                                               // claims 99, carries 3
        const auto wild = Packet({0xFFFF, 1000, 7});                 // 1 id, 2 skipped
        const auto empty = Packet({});                               // 0 ids

        int skipped = 0;
        auto part = Collect(valid, &skipped);
        skippedTotal += skipped;
        delivered.insert(delivered.end(), part.begin(), part.end());
        part = Collect(batch, &skipped);
        skippedTotal += skipped;
        delivered.insert(delivered.end(), part.begin(), part.end());
        part = Collect(lying, &skipped);
        skippedTotal += skipped;
        delivered.insert(delivered.end(), part.begin(), part.end());
        part = Collect(wild, &skipped);
        skippedTotal += skipped;
        delivered.insert(delivered.end(), part.begin(), part.end());
        CHECK(Collect(empty, &skipped).empty());
        skippedTotal += skipped;
    }

    CHECK(delivered.size() == 1000 * 8);  // 1 + 3 + 3 + 1 per round
    for (const int id : delivered)
    {
        CHECK(id >= 0);
        CHECK(id < kMaxItems);
    }
    CHECK(skippedTotal == 1000 * 2);      // 2 out-of-range ids per round
}
