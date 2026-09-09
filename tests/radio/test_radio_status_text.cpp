// Unit tests for the radio status text builder (UI/Radio/RadioStatusText).
// The HUD marquee and the config window's status line share this single
// wording map; the owner's bonus ask was that tocando / offline /
// reconectando are unambiguous — including a flaky station that shows its
// name while the engine silently reconnects.

#include "doctest.h"

#include "UI/Radio/RadioStatusText.h"

#include <cwchar>
#include <string>

namespace
{
    std::wstring Build(UI::Radio::RadioStatusKind kind, const wchar_t* station,
        const wchar_t* nowPlaying, std::size_t capacity = 160)
    {
        std::wstring buffer(capacity, L'\0');
        UI::Radio::BuildRadioStatusText(kind, station, nowPlaying, buffer.data(), capacity);
        return std::wstring(buffer.data());
    }
}

TEST_CASE("off state is explicit")
{
    CHECK(Build(UI::Radio::RadioStatusKind::Off, nullptr, nullptr)
        == L"R\u00e1dio desligada");
    CHECK(Build(UI::Radio::RadioStatusKind::Off, L"SomaFM", L"Title")
        == L"R\u00e1dio desligada");
}

TEST_CASE("connecting names the station")
{
    CHECK(Build(UI::Radio::RadioStatusKind::Connecting, nullptr, nullptr)
        == L"Conectando...");

    const std::wstring text = Build(UI::Radio::RadioStatusKind::Connecting,
        L"R\u00e1dio Bossa Nova Brazil", nullptr);
    CHECK(text == L"Conectando a R\u00e1dio Bossa Nova Brazil...");
}

TEST_CASE("playing prefers the live title, then the station, then on-air")
{
    CHECK(Build(UI::Radio::RadioStatusKind::Playing, L"SomaFM", L"Artist - Song")
        == L"Artist - Song");
    CHECK(Build(UI::Radio::RadioStatusKind::Playing, L"SomaFM", L"")
        == L"SomaFM");
    CHECK(Build(UI::Radio::RadioStatusKind::Playing, nullptr, nullptr)
        == L"Ao vivo");
}

TEST_CASE("reconnecting states BOTH the failure and the station")
{
    const std::wstring text = Build(UI::Radio::RadioStatusKind::Reconnecting,
        L"R\u00e1dio Bossa Nova Brazil", nullptr);
    CHECK(text.find(L"R\u00e1dio Bossa Nova Brazil") != std::wstring::npos);
    CHECK(text.find(L"offline") != std::wstring::npos);
    CHECK(text.find(L"reconectando") != std::wstring::npos);

    CHECK(Build(UI::Radio::RadioStatusKind::Reconnecting, nullptr, nullptr)
        == L"Offline, reconectando...");
}

// Rodada 2: while the engine rebuilds slack after an underrun the UI must say
// so — a silent "Playing" line is exactly what read as "a rádio parou do nada".
TEST_CASE("buffering names the stabilization instead of lying about playing")
{
    const std::wstring text = Build(UI::Radio::RadioStatusKind::Buffering,
        L"R\u00e1dio Bossa Nova Brazil", L"Old Title");
    CHECK(text.find(L"R\u00e1dio Bossa Nova Brazil") != std::wstring::npos);
    CHECK(text.find(L"estabilizando") != std::wstring::npos);
    // The stale ICY title must NOT survive the stall: it no longer plays.
    CHECK(text.find(L"Old Title") == std::wstring::npos);

    CHECK(Build(UI::Radio::RadioStatusKind::Buffering, nullptr, nullptr)
        == L"Estabilizando...");
    CHECK(Build(UI::Radio::RadioStatusKind::Buffering, L"", L"Song")
        == L"Estabilizando...");
}

TEST_CASE("null pointers and empty strings never crash or leak into the text")
{
    CHECK(Build(UI::Radio::RadioStatusKind::Playing, nullptr, nullptr) == L"Ao vivo");
    CHECK(Build(UI::Radio::RadioStatusKind::Playing, L"", L"") == L"Ao vivo");
    CHECK(Build(UI::Radio::RadioStatusKind::Connecting, L"", L"")
        == L"Conectando...");
}

TEST_CASE("output is safely truncated and always NUL-terminated")
{
    const wchar_t* longTitle = L"0123456789012345678901234567890123456789";

    // Capacity 10: at most 9 chars + terminator survive.
    std::wstring tiny(10, L'x');
    UI::Radio::BuildRadioStatusText(UI::Radio::RadioStatusKind::Playing,
        nullptr, longTitle, tiny.data(), 10);
    CHECK(wcsnlen_s(tiny.c_str(), 10) == 9);
    CHECK(tiny[9] == L'\0');

    // A suffix that would overflow a small buffer truncates instead of
    // aborting (the swprintf_s trap this builder deliberately avoids).
    std::wstring small(16, L'x');
    UI::Radio::BuildRadioStatusText(UI::Radio::RadioStatusKind::Reconnecting,
        L"A very long station name that cannot fit", nullptr, small.data(), 16);
    CHECK(small[15] == L'\0');
    CHECK(std::wstring(small.data()).size() == 15);
}

TEST_CASE("zero capacity and null output are no-ops")
{
    wchar_t one = L'z';
    UI::Radio::BuildRadioStatusText(UI::Radio::RadioStatusKind::Playing,
        L"SomaFM", L"Song", &one, 0);
    CHECK(one == L'z');  // untouched: no write without capacity

    wchar_t buffer[8] = {};
    UI::Radio::BuildRadioStatusText(UI::Radio::RadioStatusKind::Playing,
        L"SomaFM", L"Song", nullptr, 8);
    UI::Radio::BuildRadioStatusText(UI::Radio::RadioStatusKind::Playing,
        L"SomaFM", L"Song", buffer, 8);
    CHECK(buffer[0] == L'S');
}
