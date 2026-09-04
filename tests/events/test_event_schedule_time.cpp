#include "doctest.h"

#include "UI/NewUI/Events/EventScheduleTime.h"

#include <cwchar>

using namespace event_schedule_time;

namespace
{
struct Formatted
{
    wchar_t text[16] = {};

    bool operator==(const wchar_t* other) const { return wcscmp(text, other) == 0; }
};

Formatted duration_of(std::uint32_t seconds)
{
    Formatted f;
    format_duration(seconds, f.text, 16);
    return f;
}

Formatted clock_of(std::uint32_t timeOfDaySec)
{
    Formatted f;
    format_clock(timeOfDaySec, f.text, 16);
    return f;
}
}

TEST_CASE("format_duration keeps the shapes shown in the window")
{
    CHECK((duration_of(0) == L"--"));
    CHECK((duration_of(45) == L"45s"));
    CHECK((duration_of(59) == L"59s"));
    CHECK((duration_of(60) == L"1:00"));
    CHECK((duration_of(1859) == L"30:59"));
    CHECK((duration_of(3600) == L"1h00m"));
    CHECK((duration_of(3661) == L"1h01m"));
    CHECK((duration_of(13680) == L"3h48m"));
    CHECK((duration_of(13740) == L"3h49m"));
}

TEST_CASE("format_duration floors: 3h48m59s renders 3h48m, never 3h49m")
{
    // The product owner's flicker case: one logical instant, one string.
    const std::uint32_t threeHours48Minutes59Seconds = 3 * 3600 + 48 * 60 + 59;
    CHECK((duration_of(threeHours48Minutes59Seconds) == L"3h48m"));
}

TEST_CASE("format_duration truncates to the buffer without overrunning")
{
    wchar_t tiny[4] = {};
    format_duration(13680, tiny, 4);
    CHECK(wcscmp(tiny, L"3h4") == 0);
}

TEST_CASE("format_clock renders zero-padded hh:mm and wraps the day")
{
    CHECK((clock_of(0) == L"00:00"));
    CHECK((clock_of(71940) == L"19:59"));
    CHECK((clock_of(86399) == L"23:59"));
    CHECK((clock_of(86460) == L"00:01"));
}

TEST_CASE("remaining_seconds decays exactly once per wall second")
{
    const std::uint32_t anchor = 13680;
    const std::uint32_t anchorWall = 71940; // 19:59:00

    // Frames drawn inside the same wall second read the same value: no flicker.
    CHECK(remaining_seconds(anchor, anchorWall + 0, anchorWall) == 13680);
    CHECK(remaining_seconds(anchor, anchorWall + 0, anchorWall) == remaining_seconds(anchor, anchorWall + 0, anchorWall));

    // Crossing the boundary moves the value by exactly one second.
    CHECK(remaining_seconds(anchor, anchorWall + 1, anchorWall) == 13679);
    CHECK(remaining_seconds(anchor, anchorWall + 48, anchorWall) == 13632);

    // Clamped at zero once elapsed.
    CHECK(remaining_seconds(10, anchorWall + 20, anchorWall) == 0);
}

TEST_CASE("opening hour and countdown agree across a wall-second boundary")
{
    // The pair (clock, countdown) describes one absolute instant. When the wall
    // second advances, the clock gains a second and the countdown loses one,
    // so the rendered hh:mm may not flip by a minute on its own.
    const std::uint32_t anchor = 13680;
    const std::uint32_t anchorWall = 71940;

    const std::uint32_t before = open_wall_clock(remaining_seconds(anchor, anchorWall, anchorWall), anchorWall);
    const std::uint32_t after = open_wall_clock(remaining_seconds(anchor, anchorWall + 1, anchorWall), anchorWall + 1);
    CHECK(before == after);
    CHECK((clock_of(before) == L"23:47"));
    CHECK((clock_of(after) == L"23:47"));
}

TEST_CASE("elapsed and open clock survive local midnight")
{
    CHECK(elapsed_seconds(60, 86340) == 120);
    CHECK(elapsed_seconds(0, 0) == 0);
    CHECK(remaining_seconds(3600, 60, 86340) == 3480);
    CHECK(open_wall_clock(120, 86340) == 60);
    CHECK((clock_of(open_wall_clock(120, 86340)) == L"00:01"));
}

TEST_CASE("refresh merge keeps the display monotonic")
{
    // Normal 15 s decay: accept.
    CHECK(merge_refresh_seconds(13680, 13665) == 13665);
    // Server ceil/phase jitter of a second or two: accept, invisible.
    CHECK(merge_refresh_seconds(13680, 13681) == 13681);
    CHECK(merge_refresh_seconds(13680, 13682) == 13682);
    // Sub-minute bounce the owner saw as "variando 1 minuto": clamp.
    CHECK(merge_refresh_seconds(13680, 13740) == 13680);
    CHECK(merge_refresh_seconds(13680, 13739) == 13680);
    // Minute-scale jump: a real schedule change, accept it.
    CHECK(merge_refresh_seconds(13680, 13741) == 13741);
    CHECK(merge_refresh_seconds(13680, 20000) == 20000);
}

TEST_CASE("a full refresh cycle never increases the displayed minute")
{
    // Receive 3h49m, let it decay 15 s, refresh with a jittered value: the
    // displayed countdown must not bounce back up.
    const std::uint32_t anchorWall = 71940;
    std::uint32_t anchor = 13740; // 3h49m

    const std::uint32_t wallAtRefresh = anchorWall + 15;
    const std::uint32_t displayed = remaining_seconds(anchor, wallAtRefresh, anchorWall);
    CHECK(displayed == 13725); // 3h48m45s -> "3h48m"

    anchor = merge_refresh_seconds(displayed, 13739); // jittery server value (+14 vs displayed)
    const std::uint32_t afterRefresh = remaining_seconds(anchor, wallAtRefresh, wallAtRefresh);
    CHECK(afterRefresh == displayed); // display did not jump up
}

TEST_CASE("civil calendar round-trips and weekday mapping")
{
    // 1970-01-01 was a Thursday (qui).
    CHECK(days_from_civil(1970, 1, 1) == 0);
    CHECK(weekday_index(0) == 3);
    // 2026-09-04 is a Friday (sex).
    const std::int64_t fri = days_from_civil(2026, 9, 4);
    CHECK(weekday_index(fri) == 4);
    const CivilDate back = civil_from_days(fri);
    CHECK(back.year == 2026);
    CHECK(back.month == 9);
    CHECK(back.day == 4);
}

TEST_CASE("above 24 hours shows the date, at 24h it stays a countdown")
{
    CHECK_FALSE(should_show_date(86400));        // exactly 24h -> "24h00m"
    CHECK_FALSE(should_show_date(86400 - 1));    // 23h59m59s
    CHECK(should_show_date(86400 + 1));          // past one day -> date
    CHECK(should_show_date(343140));             // the 95h19m Crywolf case
}

TEST_CASE("opening date lands on the right weekday and day (>24h)")
{
    // Now: Friday 2026-09-04, 19:59:00. Crywolf 95h19m away = 3d 23h 19m ->
    // opens Tuesday 2026-09-08 at 19:18 -> "ter 08/09".
    const std::int64_t nowDays = days_from_civil(2026, 9, 4);
    const std::uint32_t remaining = 95 * 3600 + 19 * 60;
    const OpeningDate date = opening_date(nowDays, remaining, 71940);
    CHECK(date.weekdayIndex == 1); // ter
    CHECK(date.day == 8);
    CHECK(date.month == 9);
}

TEST_CASE("opening date rolls over the month and the year")
{
    // Wed 2026-09-30 12:00 + 3d -> Saturday 2026-10-03 (no 31/09 nonsense).
    const std::int64_t sep30 = days_from_civil(2026, 9, 30);
    const OpeningDate october = opening_date(sep30, 3 * 86400, 12 * 3600);
    CHECK(october.weekdayIndex == 5); // sáb
    CHECK(october.day == 3);
    CHECK(october.month == 10);

    // Thu 2026-12-31 23:00 + 2h (next day) -> Friday 2027-01-01.
    const std::int64_t dec31 = days_from_civil(2026, 12, 31);
    const OpeningDate january = opening_date(dec31, 2 * 3600, 23 * 3600);
    CHECK(january.weekdayIndex == 4); // sex
    CHECK(january.day == 1);
    CHECK(january.month == 1);
}

TEST_CASE("opening date stays on today when the countdown is small")
{
    const std::int64_t nowDays = days_from_civil(2026, 9, 4);
    const OpeningDate today = opening_date(nowDays, 3 * 3600, 71940); // 19:59 + 3h
    CHECK(today.day == 4);
    CHECK(today.month == 9);
    CHECK(today.weekdayIndex == 4); // sex
}

TEST_CASE("weekday and date formatting")
{
    struct Formatted
    {
        wchar_t text[16] = {};
        bool operator==(const wchar_t* other) const { return wcscmp(text, other) == 0; }
    };

    Formatted weekday;
    format_weekday(0, weekday.text, 16);
    CHECK((weekday == L"seg"));
    format_weekday(5, weekday.text, 16);
    CHECK((weekday == L"s\xe1" L"b")); // sáb
    format_weekday(6, weekday.text, 16);
    CHECK((weekday == L"dom"));

    Formatted date;
    format_date(7, 9, date.text, 16);
    CHECK((date == L"07/09"));
    format_date(31, 12, date.text, 16);
    CHECK((date == L"31/12"));
    format_date(1, 1, date.text, 16);
    CHECK((date == L"01/01"));
}
