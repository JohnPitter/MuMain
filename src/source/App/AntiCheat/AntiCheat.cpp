#include "App/AntiCheat.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <tlhelp32.h>
#include <winhttp.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "winhttp.lib")
#endif

namespace
{
    constexpr auto kDefaultHeartbeatSeconds = 60;
    constexpr size_t kHashChunkSize = 64 * 1024;
    const char* const kBlacklist[] = {
        "cheatengine.exe", "cheatengine-i386.exe", "cheatengine-x86_64.exe", "speedhack.exe",
        "artmoney.exe", "ollydbg.exe", "x64dbg.exe", "x32dbg.exe", "ida.exe", "ida64.exe",
        "wireshark.exe", "processhacker.exe", "process hacker.exe", "wpe.exe", "rpe.exe"
    };

    std::atomic_bool g_running = false;
    std::thread g_worker;

#ifdef _WIN32
    std::string HexDigest(const std::vector<unsigned char>& digest)
    {
        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (unsigned char value : digest)
            output << std::setw(2) << static_cast<unsigned int>(value);
        return output.str();
    }

    std::string Sha256(const std::wstring& path)
    {
        std::ifstream file(std::filesystem::path(path), std::ios::binary);
        if (!file)
            return {};

        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        DWORD objectSize = 0;
        DWORD resultSize = 0;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0 ||
            BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &resultSize, 0) != 0)
        {
            if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
            return {};
        }

        std::vector<unsigned char> object(objectSize);
        std::vector<unsigned char> digest(32);
        if (BCryptCreateHash(algorithm, &hash, object.data(), objectSize, nullptr, 0, 0) != 0)
        {
            BCryptCloseAlgorithmProvider(algorithm, 0);
            return {};
        }

        std::vector<char> buffer(kHashChunkSize);
        while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || file.gcount() != 0)
        {
            if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()),
                static_cast<ULONG>(file.gcount()), 0) != 0)
            {
                BCryptDestroyHash(hash);
                BCryptCloseAlgorithmProvider(algorithm, 0);
                return {};
            }
        }
        const bool valid = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) == 0;
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return valid ? HexDigest(digest) : std::string{};
    }

    std::string Narrow(const wchar_t* value)
    {
        if (!value) return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1) return {};
        std::string result(static_cast<size_t>(size - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), static_cast<int>(result.size()), nullptr, nullptr);
        return result;
    }

    void RecordDetection(const char* processName)
    {
        std::ofstream log("anticheat.log", std::ios::app);
        log << "blacklisted process detected: " << processName << '\\n';
        OutputDebugStringA("MuMain AntiCheat: blacklisted process detected\\n");
    }

    std::string Scan()
    {
        std::ostringstream report;
        bool detected = false;
        report << "{\"processes\":[";
        bool first = true;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W entry{ sizeof(entry) };
            if (Process32FirstW(snapshot, &entry)) do
            {
                std::wstring name(entry.szExeFile);
                for (const char* blocked : kBlacklist)
                {
                    std::wstring wanted;
                    int size = MultiByteToWideChar(CP_UTF8, 0, blocked, -1, nullptr, 0);
                    wanted.resize(static_cast<size_t>(size > 0 ? size - 1 : 0));
                    if (!wanted.empty()) MultiByteToWideChar(CP_UTF8, 0, blocked, -1, wanted.data(), static_cast<int>(wanted.size()));
                    if (_wcsicmp(name.c_str(), wanted.c_str()) == 0)
                    {
                        if (!first) report << ',';
                        report << '"' << blocked << '"';
                        first = false;
                        detected = true;
                        RecordDetection(blocked);
                        break;
                    }
                }
            } while (Process32NextW(snapshot, &entry));
            CloseHandle(snapshot);
        }
        if (detected)
        {
            RecordDetection("client terminated");
            ExitProcess(0xAC);
        }
        report << "],\"modules\":[";
        first = true;
        wchar_t executable[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executable, MAX_PATH);
        const std::string executableHash = Sha256(executable);
        if (!executableHash.empty())
            report << "{\"path\":\"exe\",\"sha256\":\"" << executableHash << "\"}";

        HANDLE modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
        if (modules != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32W entry{ sizeof(entry) };
            if (Module32FirstW(modules, &entry)) do
            {
                const std::string hash = Sha256(entry.szExePath);
                if (hash.empty()) continue;
                if (!first || !executableHash.empty()) report << ',';
                report << "{\"path\":\"" << Narrow(entry.szExePath) << "\",\"sha256\":\"" << hash << "\"}";
                first = false;
            } while (Module32NextW(modules, &entry));
            CloseHandle(modules);
        }
        report << "]}";
        return report.str();
    }

    bool SendHeartbeat(const std::wstring& url, const std::string& body)
    {
        URL_COMPONENTS parts{ sizeof(parts) };
        wchar_t host[256]{}, path[2048]{};
        parts.lpszHostName = host; parts.dwHostNameLength = ARRAYSIZE(host);
        parts.lpszUrlPath = path; parts.dwUrlPathLength = ARRAYSIZE(path);
        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) return false;
        HINTERNET session = WinHttpOpen(L"MuMain-AntiCheat/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) return false;
        HINTERNET connection = WinHttpConnect(session, host, parts.nPort, 0);
        HINTERNET request = connection ? WinHttpOpenRequest(connection, L"POST", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0) : nullptr;
        const wchar_t headers[] = L"Content-Type: application/json\r\n";
        const bool sent = request && WinHttpSendRequest(request, headers, ARRAYSIZE(headers) - 1,
            const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) &&
            WinHttpReceiveResponse(request, nullptr);
        if (request) WinHttpCloseHandle(request);
        if (connection) WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return sent;
    }
#endif

    void Worker()
    {
#ifdef _WIN32
        // This thread runs outside any exception boundary: an unhandled
        // exception here (e.g. std::bad_alloc while the 32-bit address space
        // is fragmented or exhausted) reaches std::terminate and fail-fasts
        // the whole game (0xc0000409) even though the main thread is healthy.
        // Every failure in a scan must be non-fatal.
        const char* configuredUrl = std::getenv("MU_ANTICHEAT_HEARTBEAT_URL");
        int seconds = kDefaultHeartbeatSeconds;
        if (const char* value = std::getenv("MU_ANTICHEAT_INTERVAL_SECONDS"))
            seconds = (std::max)(10, std::atoi(value));
        std::string body;
        try
        {
            body = Scan();
        }
        catch (...)
        {
            body.clear();
        }
        std::wstring url;
        if (configuredUrl && *configuredUrl)
        {
            const int size = MultiByteToWideChar(CP_UTF8, 0, configuredUrl, -1, nullptr, 0);
            url.resize(static_cast<size_t>(size > 0 ? size - 1 : 0));
            if (!url.empty()) MultiByteToWideChar(CP_UTF8, 0, configuredUrl, -1, url.data(), static_cast<int>(url.size()));
        }
        while (g_running)
        {
            if (!url.empty() && !body.empty()) SendHeartbeat(url, body);
            for (int i = 0; i < seconds && g_running; ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!g_running) break;
            try
            {
                body = Scan();
            }
            catch (...)
            {
                body.clear();
            }
        }
#endif
    }
}

namespace AntiCheat
{
    void Start()
    {
        if (g_running.exchange(true)) return;
        g_worker = std::thread(Worker);
    }

    void Stop()
    {
        if (!g_running.exchange(false)) return;
        if (g_worker.joinable()) g_worker.join();
    }
}
