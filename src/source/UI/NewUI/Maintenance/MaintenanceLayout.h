#pragma once

// Pure parsing + layout rules for the in-game maintenance notice window
// ("Manutenção") — the little brother of the changelog window.
//
// The server pushes the active notice once per login (C2 F3 EF, group 0xF3
// sub-code 0xEF, mirrored from MUnique.OpenMU.GameServer.RemoteView.Maintenance
// .ShowMaintenanceNoticePlugIn) a few seconds after the player enters the
// world. Everything here is a pure function of its inputs so the packet parser
// can be unit tested (tests/maintenance/test_maintenance_layout.cpp) without a
// running game.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "UI/NewUI/Changelog/ChangelogLayout.h" // shared utf8_to_wide + wrap_text

namespace maintenance_layout
{
// Wire limits — keep in sync with MaintenanceNoticeService.cs on the server.
inline constexpr std::uint8_t kGroup = 0xF3;
inline constexpr std::uint8_t kSubCode = 0xEF;
inline constexpr int kHeaderBytes = 6;        // C2 + 16-bit length + group + sub-code + active flag
inline constexpr int kMaxScheduleBytes = 64;  // UTF-8 safety cap of the schedule detail
inline constexpr int kMaxMessageBytes = 512;  // UTF-8 safety cap of the message

// Wide-character buffers (the shared utf8_to_wide always NUL-terminates).
inline constexpr int kScheduleChars = kMaxScheduleBytes + 1;
inline constexpr int kMessageChars = kMaxMessageBytes + 1;

// Popup layout: the wrapped message shows at most kMaxMessageLines lines of
// kMessageLineChars columns (window 260 px wide, ~8 px per glyph — same ratio
// as the changelog popup), reusing the changelog wrap buffer size.
inline constexpr int kMessageLineChars = 27;
inline constexpr int kMaxMessageLines = 7;
inline constexpr int kScheduleLineChars = 30;

struct ParsedNotice
{
    bool Active;
    wchar_t Schedule[kScheduleChars];
    wchar_t Message[kMessageChars];
};

// Parses a C2 F3 EF payload into out. Returns 0 on success, -1 when the header
// itself is broken. A truncated tail is tolerated: a missing schedule or
// message simply parses as empty, exactly like the changelog parser trusts the
// actual payload over the announced lengths.
inline int parse_notice(const std::uint8_t* buffer, int size, ParsedNotice* out)
{
    if (buffer == nullptr || out == nullptr || size < kHeaderBytes)
    {
        return -1;
    }

    out->Active = buffer[5] != 0;
    out->Schedule[0] = L'\0';
    out->Message[0] = L'\0';

    int offset = kHeaderBytes;

    int scheduleLength = 0;
    if (offset + 1 <= size)
    {
        scheduleLength = buffer[offset];
        ++offset;
    }

    if (offset + scheduleLength <= size)
    {
        changelog_layout::utf8_to_wide(buffer + offset, scheduleLength, out->Schedule, kScheduleChars);
    }

    offset += scheduleLength;

    int messageLength = 0;
    if (offset + 2 <= size)
    {
        messageLength = (buffer[offset] << 8) | buffer[offset + 1];
        offset += 2;
    }

    if (offset + messageLength <= size)
    {
        changelog_layout::utf8_to_wide(buffer + offset, messageLength, out->Message, kMessageChars);
    }

    return 0;
}
}
