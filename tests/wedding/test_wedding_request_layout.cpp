#include "doctest.h"

#include "UI/NewUI/Wedding/WeddingRequestLayout.h"

#include <cstring>
#include <string>
#include <vector>

using namespace wedding_request;

namespace
{
// Builds a C1 F3 EE packet the same way the server's ShowWeddingRequestPlugIn does.
std::vector<std::uint8_t> build_packet(std::uint16_t proponentId, const std::string& name)
{
    std::vector<std::uint8_t> packet(kHeaderBytes, 0);
    packet[0] = 0xC1;
    packet[1] = static_cast<std::uint8_t>(kHeaderBytes + name.size());
    packet[2] = kGroup;
    packet[3] = kSubCode;
    packet[4] = static_cast<std::uint8_t>((proponentId >> 8) & 0xFF);
    packet[5] = static_cast<std::uint8_t>(proponentId & 0xFF);
    packet[6] = static_cast<std::uint8_t>(name.size());
    packet.insert(packet.end(), name.begin(), name.end());
    return packet;
}
}

TEST_CASE("parse_request decodes the proponent id and the name with accents")
{
    // "Ação" (2-byte UTF-8 accented characters), proponent id 0x1234.
    auto packet = build_packet(0x1234, "A\xC3\xA7""or");

    ParsedRequest request = {};
    const bool ok = parse_request(packet.data(), static_cast<int>(packet.size()), &request);

    REQUIRE(ok);
    CHECK(request.ProponentId == 0x1234);
    CHECK(std::wstring(request.Name) == L"A\xE7" L"or");
}

TEST_CASE("parse_request rejects broken packets")
{
    ParsedRequest request = {};

    SUBCASE("empty buffer")
    {
        std::vector<std::uint8_t> packet;
        CHECK_FALSE(parse_request(packet.data(), 0, &request));
    }

    SUBCASE("truncated header")
    {
        auto packet = build_packet(0x0200, "Frodo");
        packet.resize(4);
        CHECK_FALSE(parse_request(packet.data(), static_cast<int>(packet.size()), &request));
    }

    SUBCASE("wrong group key")
    {
        auto packet = build_packet(0x0200, "Frodo");
        packet[2] = 0xF1;
        CHECK_FALSE(parse_request(packet.data(), static_cast<int>(packet.size()), &request));
    }

    SUBCASE("wrong sub-code")
    {
        auto packet = build_packet(0x0200, "Frodo");
        packet[3] = 0xEC;
        CHECK_FALSE(parse_request(packet.data(), static_cast<int>(packet.size()), &request));
    }

    SUBCASE("not a C1 packet")
    {
        auto packet = build_packet(0x0200, "Frodo");
        packet[0] = 0xC2;
        CHECK_FALSE(parse_request(packet.data(), static_cast<int>(packet.size()), &request));
    }
}

TEST_CASE("parse_request tolerates an empty name")
{
    auto packet = build_packet(0x0200, "");

    ParsedRequest request = {};
    const bool ok = parse_request(packet.data(), static_cast<int>(packet.size()), &request);

    REQUIRE(ok);
    CHECK(request.ProponentId == 0x0200);
    CHECK(request.Name[0] == L'\0');
}

TEST_CASE("parse_request trusts the payload over an oversized announced name length")
{
    // A hostile/buggy server announces 255 name bytes; the parser must clamp to
    // kNameBytes and never read past the actual payload.
    auto packet = build_packet(0x0200, "Frodo");
    packet[6] = 255;

    ParsedRequest request = {};
    const bool ok = parse_request(packet.data(), static_cast<int>(packet.size()), &request);

    REQUIRE(ok);
    CHECK(std::wstring(request.Name) == L"Frodo");
}

TEST_CASE("parse_request tolerates a truncated name tail")
{
    auto packet = build_packet(0x0200, "VeryLongName");
    packet.resize(kHeaderBytes + 5); // announced 12 bytes, only 5 present

    ParsedRequest request = {};
    const bool ok = parse_request(packet.data(), static_cast<int>(packet.size()), &request);

    REQUIRE(ok);
    CHECK(std::wstring(request.Name) == L"VeryL");
}
