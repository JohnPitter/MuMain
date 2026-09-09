// Unit tests for the RadioStations.ini parser (Audio/Radio/RadioStationList).
// The owner edits that file by hand, so every malformed shape a text editor
// can produce must parse to "fewer stations", never to a crash.

#include "doctest.h"

#include "Audio/Radio/RadioStationList.h"

#include <string>
#include <vector>

namespace
{
    // The shipped Data/Local/RadioStations.ini shape (UTF-8, accents,
    // comments, a section header).
    const char* kShippedFile =
        "; LuxView MU - Estacoes\n"
        "[Stations]\n"
        "\n"
        "; --- Brasil ---\n"
        "Radio Bossa Nova Brazil=http://54.38.43.201:8009/stream\n"
        "M\303\241quina do Tempo MPB=http://servidor28.brlogic.com:8032/live\n"
        "\n"
        "# hash comment\n"
        "SomaFM=https://ice1.somafm.com/groovesalad-128-mp3\n";
}

TEST_CASE("ParseStations reads the shipped file shape")
{
    Audio::Radio::RadioStation stations[16] = {};
    const int count = Audio::Radio::ParseStations(kShippedFile, stations, 16);
    REQUIRE(count == 3);

    CHECK(stations[0].Name == L"Radio Bossa Nova Brazil");
    CHECK(stations[0].Url == L"http://54.38.43.201:8009/stream");

    // UTF-8 accents survive the widening.
    CHECK(stations[1].Name == L"M\u00e1quina do Tempo MPB");
    CHECK(stations[1].Url == L"http://servidor28.brlogic.com:8032/live");

    CHECK(stations[2].Name == L"SomaFM");
}

TEST_CASE("ParseStations skips junk lines without stopping")
{
    const char* content =
        "no separator here\n"
        "=url without name\n"
        "name without url=\n"
        "\r\n"
        "  \t \n"
        "Boa=http://boa.example/stream\n";
    Audio::Radio::RadioStation stations[8] = {};
    const int count = Audio::Radio::ParseStations(content, stations, 8);
    REQUIRE(count == 1);
    CHECK(stations[0].Name == L"Boa");
}

TEST_CASE("ParseStations trims whitespace around names and urls")
{
    const char* content = "  Espacos  =  http://espacos.example/a  \r\n";
    Audio::Radio::RadioStation stations[4] = {};
    REQUIRE(Audio::Radio::ParseStations(content, stations, 4) == 1);
    CHECK(stations[0].Name == L"Espacos");
    CHECK(stations[0].Url == L"http://espacos.example/a");
}

TEST_CASE("ParseStations keeps later '=' inside the url intact")
{
    // Stream URLs can carry query strings; only the FIRST '=' separates.
    const char* content = "Query=http://x.example/stream?token=a=b&c=d\n";
    Audio::Radio::RadioStation stations[4] = {};
    REQUIRE(Audio::Radio::ParseStations(content, stations, 4) == 1);
    CHECK(stations[0].Url == L"http://x.example/stream?token=a=b&c=d");
}

TEST_CASE("ParseStations honors the output cap")
{
    const char* content = "A=http://a\nB=http://b\nC=http://c\n";
    Audio::Radio::RadioStation stations[2] = {};
    CHECK(Audio::Radio::ParseStations(content, stations, 2) == 2);
    CHECK(Audio::Radio::ParseStations(content, nullptr, 2) == 0);
    CHECK(Audio::Radio::ParseStations(content, stations, 0) == 0);
}

TEST_CASE("ParseStations tolerates a UTF-8 BOM before the first entry")
{
    const char* content = "\xEF\xBB\xBFPrimeira=http://primeira.example/a\n";
    Audio::Radio::RadioStation stations[4] = {};
    REQUIRE(Audio::Radio::ParseStations(content, stations, 4) == 1);
    CHECK(stations[0].Name == L"Primeira");
}

TEST_CASE("ParseStations handles CRLF and a missing trailing newline")
{
    const char* content = "Um=http://um\r\nDois=http://dois";
    Audio::Radio::RadioStation stations[4] = {};
    REQUIRE(Audio::Radio::ParseStations(content, stations, 4) == 2);
    CHECK(stations[0].Name == L"Um");
    CHECK(stations[1].Url == L"http://dois");
}

TEST_CASE("ParseStations returns zero for an empty or garbage file")
{
    Audio::Radio::RadioStation stations[4] = {};
    CHECK(Audio::Radio::ParseStations("", stations, 4) == 0);
    CHECK(Audio::Radio::ParseStations(";;;\n[[[\n###\n", stations, 4) == 0);
}
