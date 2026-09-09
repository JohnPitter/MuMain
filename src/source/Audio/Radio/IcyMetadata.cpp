#include "stdafx.h"
#include "Audio/Radio/IcyMetadata.h"

#include <cstring>

namespace
{
    // StreamTitle='Artist - Title';StreamUrl='...';
    constexpr const char* kTitleKey = "StreamTitle='";
    constexpr int kTitleKeyLength = 13;

    char LowerAscii(char c)
    {
        if (c >= 'A' && c <= 'Z')
        {
            return static_cast<char>(c - 'A' + 'a');
        }
        return c;
    }

    bool EqualsNoCase(const char* a, const char* b, std::size_t length)
    {
        for (std::size_t i = 0; i < length; ++i)
        {
            if (LowerAscii(a[i]) != LowerAscii(b[i]))
            {
                return false;
            }
        }
        return true;
    }

    void Trim(std::string& text)
    {
        constexpr const char* kWhitespace = " \t\r\n";
        const std::string::size_type begin = text.find_first_not_of(kWhitespace);
        if (begin == std::string::npos)
        {
            text.clear();
            return;
        }
        const std::string::size_type end = text.find_last_not_of(kWhitespace);
        text = text.substr(begin, end - begin + 1);
    }
}

namespace Audio::Radio::IcyMetadata
{
    std::size_t FindHeaderEnd(const char* data, std::size_t size)
    {
        if (data == nullptr)
        {
            return npos;
        }

        // CRLFCRLF is the spec answer; a few Shoutcast builds answer with a
        // bare LFLF. Whichever terminator comes first wins.
        std::size_t crlfEnd = npos;
        std::size_t lfEnd = npos;
        for (std::size_t i = 0; i < size; ++i)
        {
            if (crlfEnd == npos && i + 3 < size
                && data[i] == '\r' && data[i + 1] == '\n'
                && data[i + 2] == '\r' && data[i + 3] == '\n')
            {
                crlfEnd = i + 4;
            }
            if (lfEnd == npos && i + 1 < size
                && data[i] == '\n' && data[i + 1] == '\n')
            {
                lfEnd = i + 2;
            }
            if (crlfEnd != npos && lfEnd != npos)
            {
                break;
            }
        }

        if (crlfEnd != npos && lfEnd != npos)
        {
            return crlfEnd < lfEnd ? crlfEnd : lfEnd;
        }
        return crlfEnd != npos ? crlfEnd : lfEnd;
    }

    std::string GetHeaderValue(const char* data, std::size_t size, const char* name)
    {
        std::string value;
        if (data == nullptr || name == nullptr)
        {
            return value;
        }

        const std::size_t headerEnd = FindHeaderEnd(data, size);
        if (headerEnd == npos)
        {
            return value;
        }

        const std::size_t nameLength = std::strlen(name);
        std::size_t lineStart = 0;
        while (lineStart < headerEnd)
        {
            std::size_t lineEnd = lineStart;
            while (lineEnd < headerEnd && data[lineEnd] != '\r' && data[lineEnd] != '\n')
            {
                ++lineEnd;
            }

            if (lineEnd - lineStart > nameLength
                && EqualsNoCase(data + lineStart, name, nameLength)
                && data[lineStart + nameLength] == ':')
            {
                value.assign(data + lineStart + nameLength + 1, lineEnd - lineStart - nameLength - 1);
                Trim(value);
                return value;
            }

            lineStart = lineEnd + 1;
        }

        return value;
    }

    int GetMetaInterval(const char* data, std::size_t size)
    {
        const std::string value = GetHeaderValue(data, size, "icy-metaint");
        if (value.empty())
        {
            return 0;
        }

        const int interval = std::atoi(value.c_str());
        if (interval <= 0 || interval > 64 * 1024)
        {
            return 0;
        }
        return interval;
    }

    bool ExtractStreamTitle(const char* data, std::size_t size, std::string& outTitle)
    {
        outTitle.clear();
        if (data == nullptr || size < static_cast<std::size_t>(kTitleKeyLength))
        {
            return false;
        }

        const std::size_t limit = size - kTitleKeyLength;
        for (std::size_t i = 0; i <= limit; ++i)
        {
            if (!EqualsNoCase(data + i, kTitleKey, kTitleKeyLength))
            {
                continue;
            }

            const std::size_t valueStart = i + kTitleKeyLength;
            std::size_t valueEnd = valueStart;
            while (valueEnd < size && data[valueEnd] != '\'')
            {
                ++valueEnd;
            }
            if (valueEnd >= size)
            {
                // Unterminated block: servers pad metadata with NULs, so treat
                // a run of NULs as the terminator instead of dropping the title.
                while (valueEnd > valueStart && data[valueEnd - 1] == '\0')
                {
                    --valueEnd;
                }
                if (valueEnd == valueStart)
                {
                    return false;
                }
            }

            outTitle.assign(data + valueStart, valueEnd - valueStart);
            return !outTitle.empty();
        }

        return false;
    }

    void Utf8ToWide(const std::string& text, std::wstring& outWide)
    {
        outWide.clear();
        if (text.empty())
        {
            return;
        }

        // Small inline decoder: titles are <= a couple hundred bytes, and the
        // metadata blocks arrive from a worker thread where MultiByteToWideChar
        // would be fine too, but keeping this pure makes the malformed-input
        // behaviour deterministic and testable.
        outWide.reserve(text.size());
        for (std::size_t i = 0; i < text.size();)
        {
            const unsigned char lead = static_cast<unsigned char>(text[i]);
            std::uint32_t codePoint = 0xFFFD;
            std::size_t sequenceLength = 1;

            if (lead < 0x80)
            {
                codePoint = lead;
            }
            else if ((lead & 0xE0) == 0xC0 && i + 1 < text.size())
            {
                const unsigned char cont = static_cast<unsigned char>(text[i + 1]);
                if ((cont & 0xC0) == 0x80)
                {
                    codePoint = ((lead & 0x1Fu) << 6) | (cont & 0x3Fu);
                    sequenceLength = 2;
                }
            }
            else if ((lead & 0xF0) == 0xE0 && i + 2 < text.size())
            {
                const unsigned char cont1 = static_cast<unsigned char>(text[i + 1]);
                const unsigned char cont2 = static_cast<unsigned char>(text[i + 2]);
                if ((cont1 & 0xC0) == 0x80 && (cont2 & 0xC0) == 0x80)
                {
                    codePoint = ((lead & 0x0Fu) << 12) | ((cont1 & 0x3Fu) << 6) | (cont2 & 0x3Fu);
                    sequenceLength = 3;
                }
            }
            else if ((lead & 0xF8) == 0xF0)
            {
                // Astral planes cannot fit in one UTF-16 wchar_t on Windows:
                // degrade the whole 4-byte sequence to a single replacement char
                // instead of mis-parsing the continuation bytes as new text.
                sequenceLength = 4;
                if (i + 3 >= text.size())
                {
                    sequenceLength = text.size() - i;
                }
            }

            outWide.push_back(static_cast<wchar_t>(codePoint));
            i += sequenceLength;
        }
    }
}
