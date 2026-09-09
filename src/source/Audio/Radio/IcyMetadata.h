#pragma once

// Pure parsers for the ICY metadata side-channel used by Shoutcast/Icecast
// MP3 streams. Kept free of WinHTTP/SDL/DirectSound dependencies so the
// doctest suite (tests/radio) can exercise them without audio or network.

#include <cstddef>
#include <string>

namespace Audio::Radio::IcyMetadata
{
    constexpr std::size_t npos = static_cast<std::size_t>(-1);

    // Byte offset of the first body byte after the HTTP response header
    // block (terminated by CRLFCRLF, with a bare LFLF fallback used by a
    // few Shoutcast servers). Returns npos when no terminator is present.
    std::size_t FindHeaderEnd(const char* data, std::size_t size);

    // Case-insensitive lookup of one header inside the response header
    // block ("icy-metaint:16000" has no space after the colon, so both
    // spellings must parse). Returns the trimmed value, or an empty string.
    std::string GetHeaderValue(const char* data, std::size_t size, const char* name);

    // The ICY metadata period in bytes (every N stream bytes a length
    // byte + 16*N metadata bytes follow). Returns 0 when the server did
    // not send icy-metaint (no metadata support).
    int GetMetaInterval(const char* data, std::size_t size);

    // Extracts StreamTitle='...' from one ICY metadata block (the block
    // is padded with NULs and may be unterminated). Returns false when the
    // block carries no title.
    bool ExtractStreamTitle(const char* data, std::size_t size, std::string& outTitle);

    // UTF-8 -> UTF-16 for display. Invalid sequences become U+FFFD so a
    // malformed title can never crash or truncate the renderer.
    void Utf8ToWide(const std::string& text, std::wstring& outWide);
}
