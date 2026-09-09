#include "stdafx.h"
#include "Audio/Radio/RadioStationList.h"

#include "Audio/Radio/IcyMetadata.h"

#include <filesystem>
#include <fstream>

namespace
{
    // "Name=URL" — everything before the first '=' is the display name, the
    // rest (trimmed) is the stream URL. Names/URLs are kept as raw bytes here
    // and widened once at the end so an UTF-8 BOM or ANSI name both survive.
    void TrimAscii(std::string& text)
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

    std::string StripUtf8Bom(const std::string& content)
    {
        if (content.size() >= 3
            && static_cast<unsigned char>(content[0]) == 0xEF
            && static_cast<unsigned char>(content[1]) == 0xBB
            && static_cast<unsigned char>(content[2]) == 0xBF)
        {
            return content.substr(3);
        }
        return content;
    }

    std::wstring ToWide(const std::string& utf8OrAnsi)
    {
        // RadioStations.ini ships as UTF-8 (the default names carry accents);
        // hand edits in a plain ANSI editor degrade to mojibake on a few names
        // but never crash — the same trade the other Data/Local text uses.
        std::wstring wide;
        Audio::Radio::IcyMetadata::Utf8ToWide(utf8OrAnsi, wide);
        return wide;
    }

    std::wstring ExecutableDirectory()
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        std::wstring directory(exePath);
        const std::wstring::size_type slash = directory.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
        {
            directory.resize(slash + 1);
        }
        return directory;
    }
}

namespace Audio::Radio
{
    int ParseStations(const std::string& utf8Content, RadioStation* outStations, int maxStations)
    {
        if (outStations == nullptr || maxStations <= 0)
        {
            return 0;
        }

        const std::string content = StripUtf8Bom(utf8Content);
        int count = 0;
        std::size_t lineStart = 0;
        const std::size_t size = content.size();

        while (lineStart <= size && count < maxStations)
        {
            std::size_t lineEnd = lineStart;
            while (lineEnd < size && content[lineEnd] != '\n' && content[lineEnd] != '\r')
            {
                ++lineEnd;
            }

            std::string line = content.substr(lineStart, lineEnd - lineStart);
            TrimAscii(line);

            const bool isUsable = !line.empty()
                && line[0] != '[' && line[0] != ';' && line[0] != '#';
            if (isUsable)
            {
                const std::string::size_type separator = line.find('=');
                if (separator != std::string::npos)
                {
                    std::string name = line.substr(0, separator);
                    std::string url = line.substr(separator + 1);
                    TrimAscii(name);
                    TrimAscii(url);
                    if (!name.empty() && !url.empty())
                    {
                        outStations[count].Name = ToWide(name);
                        outStations[count].Url = ToWide(url);
                        ++count;
                    }
                }
            }

            if (lineEnd >= size)
            {
                break;
            }
            lineStart = lineEnd + 1;
            while (lineStart < size && (content[lineStart] == '\n' || content[lineStart] == '\r'))
            {
                ++lineStart;
            }
        }

        return count;
    }

    std::vector<RadioStation> LoadStationsFromFile(const std::wstring& path)
    {
        std::vector<RadioStation> stations;

        std::ifstream input(std::filesystem::path(path), std::ios::binary);
        if (!input)
        {
            return stations;
        }

        const std::string content((std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());

        stations.resize(64);
        const int count = ParseStations(content, stations.data(), static_cast<int>(stations.size()));
        stations.resize(count);
        return stations;
    }

    std::wstring GetStationsFilePath()
    {
        return ExecutableDirectory() + L"Data\\Local\\RadioStations.ini";
    }
}
