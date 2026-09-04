#pragma once

// Pure time + formatting rules for the event schedule window ("Eventos", hotkey O).
//
// Everything here is a pure function of its inputs so the countdown can be unit
// tested (tests/events/test_event_schedule_time.cpp). The window feeds these
// functions with ONE wall-clock source (local seconds-of-day) and per-entry
// anchors received from the server; there is no second clock ticking anywhere,
// so two frames drawn in the same wall second always render identical text and
// the (opening hour, countdown) pair can never disagree by a minute.

#include <cstddef>
#include <cstdint>

namespace event_schedule_time
{
inline constexpr std::uint32_t kDaySeconds = 86400;

// Seconds elapsed since the anchor, safe across local midnight
// (e.g. anchor 23:59:00, now 00:01:00 -> 120).
inline std::uint32_t elapsed_seconds(std::uint32_t wallNowOfDaySec, std::uint32_t anchorWallOfDaySec)
{
    return (wallNowOfDaySec + kDaySeconds - (anchorWallOfDaySec % kDaySeconds)) % kDaySeconds;
}

// Remaining seconds for one entry: the server anchor decayed by wall-clock
// time, floored once, here and nowhere else. Two calls with the same
// wallNowOfDaySec return the same value, and crossing a wall-second boundary
// moves the result by exactly one second.
inline std::uint32_t remaining_seconds(std::uint32_t anchorSeconds, std::uint32_t wallNowOfDaySec, std::uint32_t anchorWallOfDaySec)
{
    const std::uint32_t elapsed = elapsed_seconds(wallNowOfDaySec, anchorWallOfDaySec);
    return anchorSeconds > elapsed ? anchorSeconds - elapsed : 0;
}

// Refresh merge policy. The client re-requests the list every 15 s while the
// window is open; between requests it displays the previous anchor decayed by
// the wall clock. A fresh server value must never bounce the display upward:
// the server ceils to whole seconds and each plugin's clock differs slightly,
// so small bumps are jitter, only a minute-scale jump is a real schedule
// change (phase shift of a timetable event).
inline constexpr std::uint32_t kJitterToleranceSeconds = 2;
inline constexpr std::uint32_t kRealChangeSeconds = 60;

inline std::uint32_t merge_refresh_seconds(std::uint32_t currentDisplaySeconds, std::uint32_t freshSeconds)
{
    if (freshSeconds <= currentDisplaySeconds + kJitterToleranceSeconds)
    {
        return freshSeconds; // normal decay between refreshes (or ceil-level bump)
    }
    if (freshSeconds <= currentDisplaySeconds + kRealChangeSeconds)
    {
        return currentDisplaySeconds; // sub-minute bounce (incl. exactly one minute): keep decaying value
    }
    return freshSeconds; // bigger than a minute: real schedule change, accept it
}

// Local wall-clock time-of-day (seconds) at which the remaining hits zero.
inline std::uint32_t open_wall_clock(std::uint32_t remainingSeconds, std::uint32_t wallNowOfDaySec)
{
    return (wallNowOfDaySec + remainingSeconds) % kDaySeconds;
}

namespace detail
{
inline void append_u32(std::uint32_t value, wchar_t*& out, const wchar_t* end)
{
    wchar_t digits[10];
    int count = 0;
    do
    {
        digits[count++] = static_cast<wchar_t>(L'0' + (value % 10));
        value /= 10;
    } while (value > 0);

    while (count > 0 && out < end)
    {
        *out++ = digits[--count];
    }
}

inline void append_char(wchar_t c, wchar_t*& out, const wchar_t* end)
{
    if (out < end)
    {
        *out++ = c;
    }
}
}

// Same shapes the window always showed: "--", "3h48m", "31:26", "45s".
// Zero-padded fields use exactly two digits ("1h01m", "4:07").
inline void format_duration(std::uint32_t seconds, wchar_t* target, std::size_t targetCount)
{
    if (target == nullptr || targetCount == 0)
    {
        return;
    }

    wchar_t* out = target;
    const wchar_t* end = target + targetCount - 1;

    if (seconds == 0)
    {
        detail::append_char(L'-', out, end);
        detail::append_char(L'-', out, end);
    }
    else if (seconds >= 3600)
    {
        detail::append_u32(seconds / 3600, out, end);
        detail::append_char(L'h', out, end);
        const std::uint32_t minutes = (seconds % 3600) / 60;
        if (minutes < 10)
        {
            detail::append_char(L'0', out, end);
        }
        detail::append_u32(minutes, out, end);
        detail::append_char(L'm', out, end);
    }
    else if (seconds >= 60)
    {
        detail::append_u32(seconds / 60, out, end);
        detail::append_char(L':', out, end);
        const std::uint32_t secs = seconds % 60;
        if (secs < 10)
        {
            detail::append_char(L'0', out, end);
        }
        detail::append_u32(secs, out, end);
    }
    else
    {
        detail::append_u32(seconds, out, end);
        detail::append_char(L's', out, end);
    }

    *out = L'\0';
}

// "19:59" style hh:mm of a time-of-day in seconds.
inline void format_clock(std::uint32_t timeOfDaySec, wchar_t* target, std::size_t targetCount)
{
    if (target == nullptr || targetCount == 0)
    {
        return;
    }

    timeOfDaySec %= kDaySeconds;
    const std::uint32_t hours = timeOfDaySec / 3600;
    const std::uint32_t minutes = (timeOfDaySec % 3600) / 60;

    wchar_t* out = target;
    const wchar_t* end = target + targetCount - 1;
    if (hours < 10)
    {
        detail::append_char(L'0', out, end);
    }
    detail::append_u32(hours, out, end);
    detail::append_char(L':', out, end);
    if (minutes < 10)
    {
        detail::append_char(L'0', out, end);
    }
    detail::append_u32(minutes, out, end);
    *out = L'\0';
}
}
