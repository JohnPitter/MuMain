#pragma once

// Pure parsing + layout rules for the in-game changelog window ("Novidades").
//
// The server pushes the newest entries once per login (C2 F3 ED, group 0xF3
// sub-code 0xED, mirrored from MUnique.OpenMU.GameServer.RemoteView.Changelog
// .ShowChangelogPlugIn) and answers a "show all" request with the whole list.
// Everything here is a pure function of its inputs so the packet parser, the
// word wrap and the date rendering can be unit tested
// (tests/changelog/test_changelog_layout.cpp) without a running game.

#include <cstddef>
#include <cstdint>
#include <cwchar>

namespace changelog_layout
{
// Wire limits — keep in sync with InGameChangelogService.cs on the server.
inline constexpr int kMaxEntries = 30;
inline constexpr int kTitleBytes = 48;
inline constexpr int kDescriptionBytes = 200;
inline constexpr int kHeaderBytes = 6;          // C2 + 16-bit length + group + sub-code + count
inline constexpr int kEntryPrefixBytes = 4;     // 3 date bytes + title length byte
inline constexpr std::uint8_t kGroup = 0xF3;
inline constexpr std::uint8_t kSubCode = 0xED;

// Popup layout: one entry block is the bold date+title line, up to
// kDescLinesPerEntry wrapped description lines and one blank gap line.
inline constexpr int kDescLinesPerEntry = 3;
inline constexpr int kDescLineChars = 34;
inline constexpr int kWrapBufferChars = 48;
inline constexpr int kDateTextChars = 12;

struct ParsedEntry
{
    wchar_t Title[kTitleBytes + 1];
    wchar_t Description[kDescriptionBytes + 1];
    std::uint8_t YearOffset; // year - 2000
    std::uint8_t Month;
    std::uint8_t Day;
};

// Minimal UTF-8 -> UTF-16 for the packet's text fields. Handles 1-3 byte
// sequences (BMP, which covers every player-facing string on this server);
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

// Parses a C2 F3 ED payload into out. Returns the number of entries parsed
// (clamped to min(count, maxCount)), or -1 when the header itself is broken.
// A truncated tail is tolerated: the parse trusts the actual payload, not the
// announced count, exactly like the event schedule window does.
inline int parse_entries(const std::uint8_t* buffer, int size, ParsedEntry* out, int maxCount)
{
    if (buffer == nullptr || out == nullptr || size < kHeaderBytes)
    {
        return -1;
    }

    int count = buffer[5];
    if (count > kMaxEntries)
    {
        count = kMaxEntries;
    }

    if (count > maxCount)
    {
        count = maxCount;
    }

    int offset = kHeaderBytes;
    int parsed = 0;
    while (parsed < count)
    {
        if (offset + kEntryPrefixBytes > size)
        {
            break;
        }

        ParsedEntry& entry = out[parsed];
        entry.YearOffset = buffer[offset];
        entry.Month = buffer[offset + 1];
        entry.Day = buffer[offset + 2];
        const int titleLength = buffer[offset + 3];
        offset += kEntryPrefixBytes;

        if (offset + titleLength > size || titleLength < 0)
        {
            break;
        }

        utf8_to_wide(buffer + offset, titleLength, entry.Title, kTitleBytes + 1);
        offset += titleLength;

        if (offset + 1 > size)
        {
            break;
        }

        const int descriptionLength = buffer[offset];
        ++offset;

        if (offset + descriptionLength > size || descriptionLength < 0)
        {
            break;
        }

        utf8_to_wide(buffer + offset, descriptionLength, entry.Description, kDescriptionBytes + 1);
        offset += descriptionLength;

        ++parsed;
    }

    return parsed;
}

// Greedy word wrap at whole spaces, hard-splitting words longer than maxChars.
// lines must be an array of maxLines rows with kWrapBufferChars columns.
// Returns the number of rows used (never more than maxLines); when the text
// does not fit, the last row holds the hard-split overflow tail.
inline int wrap_text(const wchar_t* text, int maxChars, wchar_t (*lines)[kWrapBufferChars], int maxLines)
{
    if (lines == nullptr || maxLines <= 0)
    {
        return 0;
    }

    if (maxChars < 1)
    {
        maxChars = 1;
    }

    if (maxChars > kWrapBufferChars - 1)
    {
        maxChars = kWrapBufferChars - 1;
    }

    for (int reset = 0; reset < maxLines; ++reset)
    {
        lines[reset][0] = L'\0';
    }

    if (text == nullptr)
    {
        return 1;
    }

    int row = 0;
    int column = 0;
    const wchar_t* cursor = text;
    bool overflow = false;
    while (*cursor != L'\0' && !overflow)
    {
        if (*cursor == L' ')
        {
            // Spaces are not emitted on sight: a word that starts a new line
            // must not inherit a trailing space from the previous one.
            ++cursor;
            continue;
        }

        int wordLength = 0;
        while (cursor[wordLength] != L'\0' && cursor[wordLength] != L' ')
        {
            ++wordLength;
        }

        if (column > 0 && column + 1 + wordLength > maxChars)
        {
            ++row;
            column = 0;
            if (row >= maxLines)
            {
                overflow = true;
                break;
            }
        }

        if (column > 0)
        {
            // The word joins the current row, so the pending space fits.
            lines[row][column++] = L' ';
        }

        for (int i = 0; i < wordLength; ++i)
        {
            if (column >= maxChars)
            {
                ++row;
                column = 0;
                if (row >= maxLines)
                {
                    overflow = true;
                    break;
                }
            }

            lines[row][column++] = cursor[i];
            lines[row][column] = L'\0';
        }

        cursor += wordLength;
    }

    if (overflow)
    {
        row = maxLines - 1;
    }

    return row + 1;
}

// "dd/mm" of a parsed entry, or "--" when the date bytes are degenerate.
inline void format_date(std::uint8_t yearOffset, std::uint8_t month, std::uint8_t day, wchar_t* out, int outSize)
{
    if (out == nullptr || outSize <= 0)
    {
        return;
    }

    out[0] = L'\0';
    const int year = 2000 + yearOffset;
    if (yearOffset == 0 || month < 1 || month > 12 || day < 1 || day > 31)
    {
        if (outSize > 2)
        {
            out[0] = L'-';
            out[1] = L'-';
            out[2] = L'\0';
        }
        return;
    }

    // snprintf-free: keep it wchar and dependency-free (values are tiny).
    wchar_t* cursor = out;
    int remaining = outSize;
    auto put = [&cursor, &remaining](const wchar_t* text)
    {
        while (*text != L'\0' && remaining > 1)
        {
            *cursor++ = *text++;
            --remaining;
        }
    };
    auto put2 = [&cursor, &remaining, &put](int value)
    {
        wchar_t digits[3] = { static_cast<wchar_t>(L'0' + (value / 10) % 10), static_cast<wchar_t>(L'0' + value % 10), L'\0' };
        put(digits);
    };

    put2(day);
    put(L"/");
    put2(month);
    (void)year; // the popup shows dd/mm; the year is implicit (news are recent)
    *cursor = L'\0';
}

// Standard clamp used for the block scroll of the expanded list.
inline int clamp_scroll(int scroll, int totalBlocks, int visibleBlocks)
{
    const int maxScroll = totalBlocks > visibleBlocks ? totalBlocks - visibleBlocks : 0;
    if (scroll < 0)
    {
        return 0;
    }

    return scroll > maxScroll ? maxScroll : scroll;
}
}
