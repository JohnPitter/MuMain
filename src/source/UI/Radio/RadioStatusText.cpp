#include "stdafx.h"
#include "UI/Radio/RadioStatusText.h"

#include <cwchar>

namespace UI::Radio
{
    void BuildRadioStatusText(RadioStatusKind kind, const wchar_t* station,
        const wchar_t* nowPlaying, wchar_t* out, std::size_t outChars)
    {
        if (out == nullptr || outChars == 0)
        {
            return;
        }
        out[0] = L'\0';

        const bool hasStation = station != nullptr && station[0] != L'\0';
        const bool hasTitle = nowPlaying != nullptr && nowPlaying[0] != L'\0';

        switch (kind)
        {
        case RadioStatusKind::Playing:
            if (hasTitle)
            {
                _snwprintf_s(out, outChars, _TRUNCATE, L"%ls", nowPlaying);
                return;
            }
            if (hasStation)
            {
                _snwprintf_s(out, outChars, _TRUNCATE, L"%ls", station);
                return;
            }
            wcsncpy_s(out, outChars, L"Ao vivo", _TRUNCATE);
            return;

        case RadioStatusKind::Connecting:
            // Name the station: "Conectando" alone reads like a hang when the
            // dial takes a few seconds.
            if (hasStation)
            {
                _snwprintf_s(out, outChars, _TRUNCATE, L"Conectando a %ls...", station);
                return;
            }
            wcsncpy_s(out, outChars, L"Conectando...", _TRUNCATE);
            return;

        case RadioStatusKind::Reconnecting:
            // State AND station in one line: a flaky stream (Rádio Bossa Nova
            // Brazil blips) must read as "this station is offline, the client
            // is on it" — not as a broken radio.
            if (hasStation)
            {
                _snwprintf_s(out, outChars, _TRUNCATE,
                    L"%ls (offline, reconectando...)", station);
                return;
            }
            wcsncpy_s(out, outChars, L"Offline, reconectando...", _TRUNCATE);
            return;

        case RadioStatusKind::Off:
        default:
            wcsncpy_s(out, outChars, L"R\u00e1dio desligada", _TRUNCATE);
            return;
        }
    }
}
