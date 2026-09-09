#include "doctest.h"

#include "UI/NewUI/Maintenance/MaintenanceLayout.h"

#include <cstring>
#include <string>
#include <vector>

using namespace maintenance_layout;

namespace
{
// Builds a C2 F3 EF payload the same way the server's ShowMaintenanceNoticePlugIn does.
std::vector<std::uint8_t> build_packet(std::uint8_t active, const std::string& schedule, const std::string& message)
{
    std::vector<std::uint8_t> packet(kHeaderBytes, 0);
    packet[0] = 0xC2;
    packet[3] = kGroup;
    packet[4] = kSubCode;
    packet[5] = active;

    packet.push_back(static_cast<std::uint8_t>(schedule.size()));
    packet.insert(packet.end(), schedule.begin(), schedule.end());

    const int messageLength = static_cast<int>(message.size());
    packet.push_back(static_cast<std::uint8_t>((messageLength >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(messageLength & 0xFF));
    packet.insert(packet.end(), message.begin(), message.end());

    const int length = static_cast<int>(packet.size());
    packet[1] = static_cast<std::uint8_t>((length >> 8) & 0xFF);
    packet[2] = static_cast<std::uint8_t>(length & 0xFF);
    return packet;
}
}

TEST_CASE("parse_notice decodes active flag, schedule and message with accents")
{
    auto packet = build_packet(1, "Hoje às 03:00", "Servidor em manutenção, volte às 05:00.");

    ParsedNotice notice = {};
    REQUIRE(parse_notice(packet.data(), static_cast<int>(packet.size()), &notice) == 0);

    CHECK(notice.Active);
    CHECK(std::wcscmp(notice.Schedule, L"Hoje às 03:00") == 0);
    CHECK(std::wcscmp(notice.Message, L"Servidor em manutenção, volte às 05:00.") == 0);
}

TEST_CASE("parse_notice rejects a broken header")
{
    ParsedNotice notice = {};
    std::vector<std::uint8_t> tooShort(4, 0);
    CHECK(parse_notice(tooShort.data(), static_cast<int>(tooShort.size()), &notice) == -1);
    CHECK(parse_notice(nullptr, 0, &notice) == -1);
}

TEST_CASE("parse_notice tolerates a truncated tail")
{
    // Full packet cut down to: header + schedule length byte only.
    auto packet = build_packet(1, "03:00", "mensagem completa que não vai caber");
    packet.resize(kHeaderBytes + 1);

    ParsedNotice notice = {};
    REQUIRE(parse_notice(packet.data(), static_cast<int>(packet.size()), &notice) == 0);
    CHECK(notice.Active);
    CHECK(notice.Schedule[0] == L'\0');
    CHECK(notice.Message[0] == L'\0');
}

TEST_CASE("parse_notice treats a flag-0 packet as inactive")
{
    auto packet = build_packet(0, "", "alguém errou o push");

    ParsedNotice notice = {};
    REQUIRE(parse_notice(packet.data(), static_cast<int>(packet.size()), &notice) == 0);
    CHECK_FALSE(notice.Active);
    CHECK(std::wcscmp(notice.Message, L"alguém errou o push") == 0);
}

TEST_CASE("parse_notice reads a big-endian 16-bit message length")
{
    // 300 'm' characters: the low length byte alone would say 44, so this
    // locks in the big-endian pair.
    auto packet = build_packet(1, "", std::string(300, 'm'));

    ParsedNotice notice = {};
    REQUIRE(parse_notice(packet.data(), static_cast<int>(packet.size()), &notice) == 0);
    CHECK(std::wcslen(notice.Message) == 300);
}

TEST_CASE("the shared wrap keeps the notice inside the window line budget")
{
    // A realistic short notice fits into the visible line budget.
    wchar_t lines[kMaxMessageLines][changelog_layout::kWrapBufferChars] = {};
    const int used = changelog_layout::wrap_text(
        L"O servidor entra em manutenção às 03:00 e volta às 05:00. Guardem os itens!",
        kMessageLineChars, lines, kMaxMessageLines);
    CHECK(used <= kMaxMessageLines);
    CHECK(std::wcslen(lines[0]) <= static_cast<std::size_t>(kMessageLineChars));

    // A pathological 512-byte message overflows the budget: the wrapper
    // clamps into the last visible row instead of running past the window.
    std::wstring longMessage(400, L'a');
    const int clamped = changelog_layout::wrap_text(
        longMessage.c_str(), kMessageLineChars, lines, kMaxMessageLines);
    CHECK(clamped == kMaxMessageLines);
}
