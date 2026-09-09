// Unit tests for the pure ICY metadata parsers in Audio/Radio/IcyMetadata.
// These run against real bytes captured from Shoutcast/Icecast servers (see
// the fixtures below) plus malformed-input cases, because the radio engine
// feeds them straight from the network and a crash here takes the game down.

#include "doctest.h"

#include "Audio/Radio/IcyMetadata.h"

#include <cstring>
#include <string>

namespace
{
    // Real-shaped Icecast response (SomaFM/Radio Paradise shape): HTTP/1.0,
    // icy-* headers, CRLFCRLF, then MP3 bytes.
    const char* kIcecastResponse =
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: audio/mpeg\r\n"
        "icy-name:SomaFM Groove Salad\r\n"
        "icy-br:128\r\n"
        "icy-metaint:16000\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "\xFF\xFB\x90\x00 fake mp3 frame bytes";

    // BRLogix-style Shoutcast: no space after the colon, no trailing header
    // extras, LFLF terminator (some Shoutcast builds answer bare-LF).
    const char* kShoutcastLfResponse =
        "HTTP/1.0 200 OK\n"
        "content-type:audio/mpeg\n"
        "icy-name:Radio Teste\n"
        "icy-metaint:8192\n"
        "\n"
        "body";
}

TEST_CASE("FindHeaderEnd locates the CRLFCRLF body start")
{
    const std::size_t end = Audio::Radio::IcyMetadata::FindHeaderEnd(
        kIcecastResponse, std::strlen(kIcecastResponse));
    REQUIRE(end != Audio::Radio::IcyMetadata::npos);
    CHECK(std::strncmp(kIcecastResponse + end, "\xFF\xFB\x90\x00", 4) == 0);
}

TEST_CASE("FindHeaderEnd accepts a bare LFLF terminator")
{
    const std::size_t end = Audio::Radio::IcyMetadata::FindHeaderEnd(
        kShoutcastLfResponse, std::strlen(kShoutcastLfResponse));
    REQUIRE(end != Audio::Radio::IcyMetadata::npos);
    CHECK(std::string(kShoutcastLfResponse + end) == "body");
}

TEST_CASE("FindHeaderEnd returns npos on truncated headers")
{
    const char* truncated = "HTTP/1.0 200 OK\r\nContent-Type: audio/mpeg\r\n";
    CHECK(Audio::Radio::IcyMetadata::FindHeaderEnd(truncated, std::strlen(truncated))
          == Audio::Radio::IcyMetadata::npos);
    CHECK(Audio::Radio::IcyMetadata::FindHeaderEnd(nullptr, 10) == Audio::Radio::IcyMetadata::npos);
}

TEST_CASE("GetMetaInterval parses icy-metaint with and without spaces")
{
    CHECK(Audio::Radio::IcyMetadata::GetMetaInterval(
              kIcecastResponse, std::strlen(kIcecastResponse)) == 16000);
    CHECK(Audio::Radio::IcyMetadata::GetMetaInterval(
              kShoutcastLfResponse, std::strlen(kShoutcastLfResponse)) == 8192);
}

TEST_CASE("GetMetaInterval rejects absent, zero and absurd intervals")
{
    const char* noMeta =
        "HTTP/1.0 200 OK\r\nContent-Type: audio/mpeg\r\n\r\nbody";
    CHECK(Audio::Radio::IcyMetadata::GetMetaInterval(noMeta, std::strlen(noMeta)) == 0);

    const char* zeroMeta =
        "HTTP/1.0 200 OK\r\nicy-metaint:0\r\n\r\nbody";
    CHECK(Audio::Radio::IcyMetadata::GetMetaInterval(zeroMeta, std::strlen(zeroMeta)) == 0);

    const char* absurdMeta =
        "HTTP/1.0 200 OK\r\nicy-metaint:99999999\r\n\r\nbody";
    CHECK(Audio::Radio::IcyMetadata::GetMetaInterval(absurdMeta, std::strlen(absurdMeta)) == 0);
}

TEST_CASE("GetHeaderValue is case-insensitive on both name and lookup")
{
    const std::string name = Audio::Radio::IcyMetadata::GetHeaderValue(
        kIcecastResponse, std::strlen(kIcecastResponse), "Icy-Name");
    CHECK(name == "SomaFM Groove Salad");

    const std::string lower = Audio::Radio::IcyMetadata::GetHeaderValue(
        kShoutcastLfResponse, std::strlen(kShoutcastLfResponse), "Content-Type");
    CHECK(lower == "audio/mpeg");

    const std::string missing = Audio::Radio::IcyMetadata::GetHeaderValue(
        kIcecastResponse, std::strlen(kIcecastResponse), "icy-genre");
    CHECK(missing.empty());
}

TEST_CASE("ExtractStreamTitle parses the classic single-title block")
{
    const char* block = "StreamTitle='Legiao Urbana - Tempo Perdido';StreamUrl='';";
    std::string title;
    REQUIRE(Audio::Radio::IcyMetadata::ExtractStreamTitle(block, std::strlen(block), title));
    CHECK(title == "Legiao Urbana - Tempo Perdido");
}

TEST_CASE("ExtractStreamTitle handles UTF-8 titles and padded blocks")
{
    // Real blocks are 16*n bytes, NUL-padded after the last ';'.
    char block[64] = {};
    std::strcpy(block, "StreamTitle='Sepultura - Refuse/Resist';");
    std::memset(block + std::strlen(block), 0, sizeof(block) - std::strlen(block));

    std::string title;
    REQUIRE(Audio::Radio::IcyMetadata::ExtractStreamTitle(block, sizeof(block), title));
    CHECK(title == "Sepultura - Refuse/Resist");
}

TEST_CASE("ExtractStreamTitle keeps inner quotes-adjacent characters intact")
{
    const char* block = "StreamTitle='It's a \"Test\" (Live)';";
    std::string title;
    // The block ends at the first apostrophe after the opening one, so a
    // title containing an escaped quote degrades to a prefix — it must not
    // crash or hang.
    REQUIRE(Audio::Radio::IcyMetadata::ExtractStreamTitle(block, std::strlen(block), title));
    CHECK(title == "It");
}

TEST_CASE("ExtractStreamTitle rejects blocks without a title")
{
    const char* block = "StreamUrl='http://example.com';length='1';";
    std::string title;
    CHECK_FALSE(Audio::Radio::IcyMetadata::ExtractStreamTitle(block, std::strlen(block), title));
    CHECK_FALSE(Audio::Radio::IcyMetadata::ExtractStreamTitle(nullptr, 10, title));
}

TEST_CASE("Utf8ToWide decodes accents from real station names")
{
    std::wstring wide;
    Audio::Radio::IcyMetadata::Utf8ToWide("Máquina do Tempo", wide);
    CHECK(wide == L"Máquina do Tempo");
}

TEST_CASE("Utf8ToWide survives malformed sequences")
{
    std::wstring wide;
    // Truncated 2-byte sequence + stray continuation byte.
    Audio::Radio::IcyMetadata::Utf8ToWide("R\xC3", wide);
    CHECK(wide.size() == 2);

    Audio::Radio::IcyMetadata::Utf8ToWide("\x80\x80", wide);
    CHECK(wide.size() == 2);

    // Empty input -> empty output, never a crash.
    Audio::Radio::IcyMetadata::Utf8ToWide("", wide);
    CHECK(wide.empty());
}
