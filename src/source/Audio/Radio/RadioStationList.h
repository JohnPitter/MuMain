#pragma once

// The radio station list lives in a client file the owner can edit without a
// rebuild: Data/Local/RadioStations.ini, one "Name=URL" line per station.
// Parsing is a pure function so the doctest suite can feed it raw bytes.

#include <string>
#include <vector>

namespace Audio::Radio
{
    struct RadioStation
    {
        std::wstring Name;
        std::wstring Url;
    };

    // Parses "Name=URL" lines out of raw INI text (UTF-8 or ANSI). Section
    // headers ([...]) are ignored so the owner may group stations under a
    // [Stations] section; ';' and '#' start comments; blank lines are skipped.
    // Lines without '=' are skipped, never fatal. Returns the number of
    // stations written to outStations (capped by maxStations).
    int ParseStations(const std::string& utf8Content, RadioStation* outStations, int maxStations);

    // Convenience wrapper used by production code. The file is read from the
    // client Data dir; a missing or empty file yields an empty list (the UI
    // then shows "no stations" instead of failing).
    std::vector<RadioStation> LoadStationsFromFile(const std::wstring& path);

    // Resolved at runtime against the executable directory.
    std::wstring GetStationsFilePath();
}
