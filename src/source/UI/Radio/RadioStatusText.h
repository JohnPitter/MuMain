#pragma once

// Pure builder of the radio's user-facing status text (the HUD marquee and
// the config window's status line share it — one source of truth for wording).
// Split out of the renderers so the state-to-text mapping is unit-tested
// (tests/radio/test_radio_status_text.cpp).

#include <cstddef>

namespace UI::Radio
{
    // Minimal mirror of Audio::Radio::RadioEngine::State. Kept separate so
    // this module (and its tests) never drag the engine/WinHTTP headers in.
    enum class RadioStatusKind
    {
        Off,
        Connecting,
        Playing,
        Reconnecting,
    };

    // Writes the status text for `kind` into `out` (always NUL-terminated,
    // safely truncated to `outChars`). `station` / `nowPlaying` may be null
    // or empty; the Playing branch prefers the live ICY title, then the
    // station name, then a bare on-air fallback.
    void BuildRadioStatusText(RadioStatusKind kind, const wchar_t* station,
        const wchar_t* nowPlaying, wchar_t* out, std::size_t outChars);
}
