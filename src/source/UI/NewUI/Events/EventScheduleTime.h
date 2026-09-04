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

// --- Opening date (events more than a day away) -----------------------------

// Above one day the hour count ("95h19m") stops being useful; the window shows
// the opening weekday and date instead ("dom 07/09"). Exactly 24h still reads
// "24h00m".
inline bool should_show_date(std::uint32_t remainingSeconds)
{
    return remainingSeconds > kDaySeconds;
}

// Days since 1970-01-01 in the proleptic Gregorian calendar (Howard Hinnant's
// civil-calendar algorithms). Doing the arithmetic in epoch days is what keeps
// month and year rollovers correct — there is no way to produce 31/09.
inline std::int64_t days_from_civil(std::uint32_t year, std::uint32_t month, std::uint32_t day)
{
    const std::int64_t y = static_cast<std::int64_t>(year) - (month <= 2 ? 1 : 0);
    const std::int64_t era = y / 400;
    const std::int64_t yoe = y - era * 400;                                  // [0, 399]
    const std::int64_t doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const std::int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
    return era * 146097 + doe - 719468;
}

// Weekday of an epoch day: 0 = seg (Monday) .. 6 = dom (Sunday).
// 1970-01-01 was a Thursday.
inline std::uint32_t weekday_index(std::int64_t epochDays)
{
    const std::int64_t mod = epochDays % 7 + 3 + 7; // +3: Thu -> Mon-based index
    return static_cast<std::uint32_t>(mod % 7);
}

struct CivilDate
{
    std::uint32_t year;
    std::uint32_t month; // 1..12
    std::uint32_t day;   // 1..31
    std::uint32_t weekdayIndex; // 0 = seg .. 6 = dom
};

inline CivilDate civil_from_days(std::int64_t epochDays)
{
    const std::int64_t z = epochDays + 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const std::int64_t doe = z - era * 146097;                                          // [0, 146096]
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;     // [0, 399]
    const std::int64_t y = yoe + era * 400;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);                   // [0, 365]
    const std::int64_t mp = (5 * doy + 2) / 153;                                        // [0, 11]
    const std::int64_t d = doy - (153 * mp + 2) / 5 + 1;                                // [1, 31]
    const std::int64_t m = mp < 10 ? mp + 3 : mp - 9;                                   // [1, 12]
    CivilDate date;
    date.year = static_cast<std::uint32_t>(y + (m <= 2 ? 1 : 0));
    date.month = static_cast<std::uint32_t>(m);
    date.day = static_cast<std::uint32_t>(d);
    date.weekdayIndex = weekday_index(epochDays);
    return date;
}

// Calendar date of the opening instant: now's epoch day and time-of-day plus
// the remaining countdown. Same clock the countdown decays on, so the date
// always agrees with the rendered hh:mm.
struct OpeningDate
{
    std::uint32_t weekdayIndex; // 0 = seg .. 6 = dom
    std::uint32_t day;          // 1..31
    std::uint32_t month;        // 1..12
};

inline OpeningDate opening_date(std::int64_t nowEpochDays, std::uint32_t remainingSeconds, std::uint32_t wallNowOfDaySec)
{
    const std::int64_t openingEpochDays =
        (nowEpochDays * kDaySeconds + wallNowOfDaySec + remainingSeconds) / kDaySeconds;
    const CivilDate date = civil_from_days(openingEpochDays);
    OpeningDate result;
    result.weekdayIndex = date.weekdayIndex;
    result.day = date.day;
    result.month = date.month;
    return result;
}

// "seg".."dom", indexed by weekday_index.
inline void format_weekday(std::uint32_t weekdayIndex, wchar_t* target, std::size_t targetCount)
{
    static const wchar_t* kWeekdays[7] = { L"seg", L"ter", L"qua", L"qui", L"sex", L"s\xe1""b", L"dom" };

    if (target == nullptr || targetCount == 0)
    {
        return;
    }

    wchar_t* out = target;
    const wchar_t* end = target + targetCount - 1;
    const wchar_t* name = kWeekdays[weekdayIndex % 7];
    while (*name != L'\0' && out < end)
    {
        *out++ = *name++;
    }
    *out = L'\0';
}

// "07/09" style zero-padded dd/mm.
inline void format_date(std::uint32_t day, std::uint32_t month, wchar_t* target, std::size_t targetCount)
{
    if (target == nullptr || targetCount == 0)
    {
        return;
    }

    wchar_t* out = target;
    const wchar_t* end = target + targetCount - 1;
    if (day < 10)
    {
        detail::append_char(L'0', out, end);
    }
    detail::append_u32(day, out, end);
    detail::append_char(L'/', out, end);
    if (month < 10)
    {
        detail::append_char(L'0', out, end);
    }
    detail::append_u32(month, out, end);
    *out = L'\0';
}
}
