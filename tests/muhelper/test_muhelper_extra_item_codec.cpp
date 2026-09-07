#include "doctest.h"

#include "MUHelper/MuHelperExtraItemCodec.h"

#include <cstring>

using namespace MUHelper::ItemFilter;

// Root-cause regression coverage for the "Add extra" item-name list not
// surviving a save/relog (bug report: "o Set Item e o Add extra nao ta
// salvando"). Production DB evidence (character SeuAntonio, account
// testando, 2026-09-06): the "Add extra" checkbox (AddExtraItem, byte 1 bit
// 7 of the 257-byte MuHelperConfiguration blob) read back set, but all 12
// ExtraItems[15] wire slots read back as all-zero bytes -- the enable flag
// persisted, the names the player typed did not.
//
// Root cause: ConfigDataSerDe::Serialize() (MuHelperData.cpp) encoded each
// name with wcstombs() into the 15-byte slot and, whenever the encoded name
// did not fit (wcstombs() returns the buffer size with no null terminator
// written), wiped the *entire* slot to zero instead of keeping a shortened
// name. Real MU item names are frequently 15+ characters (most multi-word
// names), so this was not an edge case -- it was the common case. This file
// pins the fixed behavior: EncodeUtf8Truncated() must always keep something
// for a non-empty input once destCapacity > 1, never discarding the whole
// name outright, and it must produce standard UTF-8 (mirroring
// Deserialize()'s CMultiLanguage::ConvertFromUtf8), not locale-dependent
// bytes.

TEST_CASE("a short ASCII name round-trips exactly with a null terminator")
{
    char dest[15];
    std::memset(dest, 0xCD, sizeof(dest)); // poison, so leftover bytes are visible

    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), L"Divine Sword");

    CHECK(n == 12);
    CHECK(std::strcmp(dest, "Divine Sword") == 0);
    CHECK(dest[12] == '\0');
}

TEST_CASE("a name that exactly fills the 14 usable bytes keeps its terminator")
{
    // Exactly 14 ASCII characters -- the largest name that fits untruncated
    // in a 15-byte slot.
    char dest[15];
    const wchar_t* name = L"14-characters!"; // 14 chars

    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), name);

    CHECK(n == 14);
    CHECK(std::strcmp(dest, "14-characters!") == 0);
}

TEST_CASE("a name too long for the slot is truncated, never wiped to empty")
{
    // A realistic MU item name well past the 14-byte usable capacity of one
    // ExtraItems slot. Before the fix, wcstombs()'s truncation return value
    // (== destCapacity) made Serialize() memset the whole 15-byte slot to
    // zero -- this is exactly the "Add extra" symptom from the bug report.
    char dest[15];
    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), L"Bright Full Armor of the Guardian");

    // Never empty: this is the actual regression this codec exists to fix.
    CHECK(n > 0);
    CHECK(n <= 14);
    CHECK(dest[n] == '\0');
    // Truncated, not garbage: the kept bytes are an exact prefix of the source.
    CHECK(std::strncmp(dest, "Bright Full Ar", n) == 0);
}

TEST_CASE("destCapacity of zero writes nothing and does not crash")
{
    char dest[1] = { 'x' };
    const std::size_t n = EncodeUtf8Truncated(dest, 0, L"anything");
    CHECK(n == 0);
    CHECK(dest[0] == 'x'); // untouched -- no room even for a terminator
}

TEST_CASE("an empty source name encodes to an empty, null-terminated string")
{
    char dest[15];
    std::memset(dest, 0xCD, sizeof(dest));

    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), L"");

    CHECK(n == 0);
    CHECK(dest[0] == '\0');
}

TEST_CASE("a null source is treated as empty, not a crash")
{
    char dest[15];
    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), nullptr);
    CHECK(n == 0);
    CHECK(dest[0] == '\0');
}

TEST_CASE("accented characters encode as standard UTF-8, matching ConvertFromUtf8's decoder")
{
    // "Anel Sagrado" (Portuguese-localized item-style name) with two
    // 2-byte UTF-8 code points (a with acute U+00E1 = 0xC3 0xA1, plus 'c') to
    // catch the wcstombs()-vs-UTF-8 encoding mismatch this codec replaces:
    // wcstombs() encodes using the current C locale (commonly a single-byte
    // ANSI code page on this client), which does not agree byte-for-byte
    // with the UTF-8 CMultiLanguage::ConvertFromUtf8() expects on the way
    // back in -- so accented names would decode as mojibake even when they
    // technically fit in 15 bytes.
    char dest[15];
    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), L"Análise"); // "Análise"

    // 'A','n' (1 byte each) + U+00E1 (2 bytes: 0xC3 0xA1) + "lise" (4 bytes) = 8
    CHECK(n == 8);
    const unsigned char expected[] = { 'A', 'n', 0xC3, 0xA1, 'l', 'i', 's', 'e', '\0' };
    CHECK(std::memcmp(dest, expected, sizeof(expected)) == 0);
}

TEST_CASE("truncation never splits a multi-byte UTF-8 sequence")
{
    // 13 ASCII characters followed by an accented character whose 2-byte
    // UTF-8 encoding would push the total past the 14-byte usable budget.
    // The encoder must drop the whole accented character rather than emit
    // its first byte alone (which would corrupt the next name read back on
    // the server, since ExtraItems slots are parsed as independent
    // null-terminated runs -- see MuHelperSettingsSerializer.TryDeserialize).
    char dest[15];
    const std::size_t n = EncodeUtf8Truncated(dest, sizeof(dest), L"1234567890123á"); // 13 ASCII + 'a-acute'

    CHECK(n == 13);
    CHECK(std::strcmp(dest, "1234567890123") == 0);
}

TEST_CASE("a 12-slot ExtraItems array keeps every earlier slot when a later one overflows")
{
    // Mirrors ConfigDataSerDe::Serialize()'s per-slot loop: each of the 12
    // slots is encoded independently, so one long name must not affect its
    // neighbors (unlike the pre-fix code, which zeroed only the overflowing
    // slot but via a mechanism -- wcstombs()'s truncation signal -- that is
    // trivially reachable by any real item name).
    char slots[3][15];
    EncodeUtf8Truncated(slots[0], 15, L"Jewel of Bless");
    EncodeUtf8Truncated(slots[1], 15, L"A Very Long Item Name That Overflows");
    EncodeUtf8Truncated(slots[2], 15, L"Jewel of Soul");

    CHECK(std::strcmp(slots[0], "Jewel of Bless") == 0);
    CHECK(slots[1][0] != '\0'); // truncated, but not wiped
    CHECK(std::strcmp(slots[2], "Jewel of Soul") == 0);
}
