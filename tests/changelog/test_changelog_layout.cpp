#include "doctest.h"

#include "UI/NewUI/Changelog/ChangelogLayout.h"

#include <cstring>
#include <string>
#include <vector>

using namespace changelog_layout;

namespace
{
// Builds a C2 F3 ED payload the same way the server's ShowChangelogPlugIn does.
std::vector<std::uint8_t> build_packet(const std::vector<std::vector<std::uint8_t>>& entries)
{
    std::vector<std::uint8_t> packet(kHeaderBytes, 0);
    packet[0] = 0xC2;
    packet[3] = kGroup;
    packet[4] = kSubCode;
    packet[5] = static_cast<std::uint8_t>(entries.size());
    for (const auto& entry : entries)
    {
        packet.insert(packet.end(), entry.begin(), entry.end());
    }

    const int length = static_cast<int>(packet.size());
    packet[1] = static_cast<std::uint8_t>((length >> 8) & 0xFF);
    packet[2] = static_cast<std::uint8_t>(length & 0xFF);
    return packet;
}

std::vector<std::uint8_t> entry_bytes(std::uint8_t year, std::uint8_t month, std::uint8_t day,
    const std::string& title, const std::string& description)
{
    std::vector<std::uint8_t> bytes;
    bytes.push_back(year);
    bytes.push_back(month);
    bytes.push_back(day);
    bytes.push_back(static_cast<std::uint8_t>(title.size()));
    bytes.insert(bytes.end(), title.begin(), title.end());
    bytes.push_back(static_cast<std::uint8_t>(description.size()));
    bytes.insert(bytes.end(), description.begin(), description.end());
    return bytes;
}

std::wstring joined(const wchar_t (*lines)[kWrapBufferChars], int count)
{
    std::wstring result;
    for (int i = 0; i < count; ++i)
    {
        if (i > 0)
        {
            result += L"|";
        }
        result += lines[i];
    }

    return result;
}
}

TEST_CASE("parse_entries decodes date, title and description with accents")
{
    // "Novo" plain + "Ação" (2-byte UTF-8 accented characters).
    auto packet = build_packet({
        entry_bytes(26, 9, 7, "Selo VIP", "Ação e coração no nome."),
    });

    ParsedEntry entries[kMaxEntries] = {};
    const int count = parse_entries(packet.data(), static_cast<int>(packet.size()), entries, kMaxEntries);

    REQUIRE(count == 1);
    CHECK(entries[0].YearOffset == 26);
    CHECK(entries[0].Month == 9);
    CHECK(entries[0].Day == 7);
    CHECK(std::wcscmp(entries[0].Title, L"Selo VIP") == 0);
    CHECK(std::wcscmp(entries[0].Description, L"Ação e coração no nome.") == 0);
}

TEST_CASE("parse_entries rejects a broken header and tolerates a truncated tail")
{
    ParsedEntry entries[4] = {};
    std::vector<std::uint8_t> tooShort(4, 0);
    CHECK(parse_entries(tooShort.data(), static_cast<int>(tooShort.size()), entries, 4) == -1);
    CHECK(parse_entries(nullptr, 0, entries, 4) == -1);

    // The announced count is 2, but the payload only carries one complete entry.
    auto packet = build_packet({
        entry_bytes(26, 9, 7, "a", "b"),
        entry_bytes(26, 9, 8, "c", "d"),
    });
    packet.resize(kHeaderBytes + kEntryPrefixBytes + 1 + 1 + 1 + 1); // header + first entry (1-char title and description)
    const int count = parse_entries(packet.data(), static_cast<int>(packet.size()), entries, 4);
    CHECK(count == 1);
    CHECK(std::wcscmp(entries[0].Title, L"a") == 0);
}

TEST_CASE("parse_entries trusts the payload over an oversized announced count")
{
    auto packet = build_packet({ entry_bytes(26, 1, 1, "t", "d") });
    packet[5] = 200; // lie about the count

    ParsedEntry entries[kMaxEntries] = {};
    const int count = parse_entries(packet.data(), static_cast<int>(packet.size()), entries, kMaxEntries);

    CHECK(count == 1);
}

TEST_CASE("utf8_to_wide keeps BMP sequences and replaces malformed leads")
{
    // "ã" = 0xC3 0xA3 (2 bytes); "ç" = 0xE7 in Latin-1 would be malformed alone.
    const std::uint8_t mixed[] = { 0x41, 0xC3, 0xA3, 0xE2, 0x82, 0xAC, 0xFF, 0x42 };
    wchar_t out[16] = {};
    utf8_to_wide(mixed, sizeof(mixed), out, 16);

    CHECK(std::wcscmp(out, L"Aã€?B") == 0);
}

TEST_CASE("utf8_to_wide never overruns the destination")
{
    const std::uint8_t ascii[10] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j' };
    wchar_t tiny[4] = {};
    utf8_to_wide(ascii, 10, tiny, 4);
    CHECK(std::wcscmp(tiny, L"abc") == 0);
}

TEST_CASE("wrap_text keeps a short line whole")
{
    wchar_t lines[3][kWrapBufferChars] = {};
    const int used = wrap_text(L"Corrigido o crash", 34, lines, 3);
    CHECK(used == 1);
    CHECK(joined(lines, used) == L"Corrigido o crash");
}

TEST_CASE("wrap_text wraps at spaces and drops the gap")
{
    wchar_t lines[4][kWrapBufferChars] = {};
    const int used = wrap_text(L"Contas VIP agora mostram um selo dourado ao lado do nome.", 20, lines, 4);

    CHECK(used == 4);
    CHECK(std::wcslen(lines[0]) <= 20);
    CHECK(std::wcslen(lines[1]) <= 20);
    CHECK(std::wcslen(lines[2]) <= 20);
    CHECK(std::wcslen(lines[3]) <= 20);
    // Every word survives the wrap, in order, spaces only at the joins.
    CHECK(joined(lines, used) == L"Contas VIP agora|mostram um selo|dourado ao lado do|nome.");
}

TEST_CASE("wrap_text hard-splits a word longer than the line")
{
    wchar_t lines[4][kWrapBufferChars] = {};
    const wchar_t* longWord = L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // 40 chars
    const int used = wrap_text(longWord, 34, lines, 4);

    CHECK(used >= 2);
    CHECK(std::wcslen(lines[0]) == 34);
    CHECK(std::wcslen(lines[1]) == 6);
    CHECK(joined(lines, used) == L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa|aaaaaa");
}

TEST_CASE("wrap_text overflow keeps the tail in the last row without overrunning")
{
    wchar_t lines[2][kWrapBufferChars] = {};
    const wchar_t* longWord = L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // 40 chars
    const int used = wrap_text(longWord, 34, lines, 2);

    CHECK(used == 2);
    CHECK(std::wcslen(lines[0]) == 34);
    CHECK(std::wcslen(lines[1]) <= 34);
}

TEST_CASE("wrap_text handles empty and null text")
{
    wchar_t lines[2][kWrapBufferChars] = {};
    CHECK(wrap_text(L"", 34, lines, 2) == 1);
    CHECK(lines[0][0] == L'\0');
    CHECK(wrap_text(nullptr, 34, lines, 2) == 1);
    CHECK(wrap_text(L"abc", 34, nullptr, 2) == 0);
}

TEST_CASE("format_date renders dd/mm and degrades to --")
{
    wchar_t text[kDateTextChars] = {};
    format_date(26, 9, 7, text, kDateTextChars);
    CHECK(std::wcscmp(text, L"07/09") == 0);

    format_date(26, 0, 7, text, kDateTextChars);
    CHECK(std::wcscmp(text, L"--") == 0);

    format_date(0, 9, 7, text, kDateTextChars);
    CHECK(std::wcscmp(text, L"--") == 0);
}

TEST_CASE("clamp_scroll bounds the block scroll")
{
    CHECK(clamp_scroll(-5, 30, 4) == 0);
    CHECK(clamp_scroll(3, 30, 4) == 3);
    CHECK(clamp_scroll(99, 30, 4) == 26);
    CHECK(clamp_scroll(2, 3, 4) == 0);
}
