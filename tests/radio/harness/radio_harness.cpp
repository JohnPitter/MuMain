// Radio streaming measurement harness.
//
// Runs the PRODUCTION RadioEngine (the exact .cpp the client ships) against a
// real station URL for N minutes, samples the engine's diagnostics at a fixed
// rate, and writes a CSV plus a verdict summary. This is how the "picota /
// para do nada" reports get pinned on real numbers: chunk stall times, push
// gaps, buffer level, underrun/resume counts and state history.
//
// Usage:  radio_harness.exe <url> [minutes] [csvPath]
//
// Not wired into ctest: it needs a live internet stream and minutes of wall
// time. Built behind MU_BUILD_RADIO_HARNESS=ON.

#include "stdafx.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "Audio/AudioPlayer.h"
#include "Audio/Radio/RadioEngine.h"
#include "Core/Utilities/Log/ErrorReport.h"

namespace
{
    FILE* g_logFile = nullptr;

    void LogLine(const wchar_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        vwprintf(format, args);
        va_end(args);
        wprintf(L"\n");
        if (g_logFile != nullptr)
        {
            va_start(args, format);
            vfwprintf(g_logFile, format, args);
            va_end(args);
            fwprintf(g_logFile, L"\n");
            fflush(g_logFile);
        }
    }
}

// ErrorReport shim definition (the radio code's only logging dependency).
CErrorReport g_ErrorReport;

void CErrorReport::Write(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    vwprintf(format, args);
    va_end(args);
    wprintf(L"\n");
    if (g_logFile != nullptr)
    {
        va_start(args, format);
        vfwprintf(g_logFile, format, args);
        va_end(args);
        fwprintf(g_logFile, L"\n");
        fflush(g_logFile);
    }
}

// AudioPlayer shim definition: a real mixer, built exactly like the client's
// AudioPlayer::Initialize.
namespace AudioPlayer
{
    MIX_Mixer* GetMixer()
    {
        static MIX_Mixer* mixer = nullptr;
        if (mixer == nullptr)
        {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
            {
                return nullptr;
            }
            if (!MIX_Init())
            {
                return nullptr;
            }
            mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        }
        return mixer;
    }
}

namespace
{
    std::wstring Widen(const char* text)
    {
        std::wstring wide;
        const int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        if (len > 0)
        {
            wide.resize(static_cast<std::size_t>(len));
            MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), len);
            wide.pop_back();
        }
        return wide;
    }

    const wchar_t* StateName(Audio::Radio::RadioEngine::State state)
    {
        using State = Audio::Radio::RadioEngine::State;
        switch (state)
        {
        case State::Connecting: return L"Connecting";
        case State::Buffering: return L"Buffering";
        case State::Playing: return L"Playing";
        case State::Reconnecting: return L"Reconnecting";
        case State::Off: return L"Off";
        }
        return L"?";
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        wprintf(L"usage: radio_harness.exe <url> [minutes] [csvPath]\n");
        return 2;
    }

    const std::wstring url = Widen(argv[1]);
    const int minutes = argc > 2 ? atoi(argv[2]) : 10;
    const char* csvPath = argc > 3 ? argv[3] : "radio_harness.csv";

    _wfopen_s(&g_logFile, L"radio_harness.log", L"w, ccs=UTF-8");

    MIX_Mixer* mixer = AudioPlayer::GetMixer();
    if (mixer == nullptr)
    {
        wprintf(L"fatal: no audio device (%hs)\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec spec {};
    if (!MIX_GetMixerFormat(mixer, &spec))
    {
        wprintf(L"fatal: no mixer format\n");
        return 1;
    }
    LogLine(L"[harness] mixer format: %d Hz, %d ch, format %d",
        spec.freq, spec.channels, spec.format);

    Audio::Radio::RadioEngine& engine = Audio::Radio::RadioEngine::Instance();
    engine.SetVolume(80);
    engine.Start(url.c_str(), L"Harness");

    FILE* csv = nullptr;
    fopen_s(&csv, csvPath, "w");
    if (csv != nullptr)
    {
        fprintf(csv, "t_ms,state,buffered_bytes,buffered_ms,underruns,resumes,play_failures,"
            "stream_rebuilds,sessions,chunk_reads,last_chunk_ms,max_chunk_ms,"
            "last_push_gap_ms,max_push_gap_ms\n");
    }

    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + std::chrono::minutes(minutes);
    int silentPlayingSamples = 0;   // Playing but < 250 ms of audio buffered
    int playingSamples = 0;
    int nextSample = 0;

    while (std::chrono::steady_clock::now() < deadline)
    {
        Sleep(50);
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsedMs < nextSample)
        {
            continue;
        }
        nextSample += 250;

        Audio::Radio::RadioEngine::Diagnostics diag;
        engine.GetDiagnostics(diag);
        const int bps = diag.srcBytesPerSecond > 0 ? diag.srcBytesPerSecond : 384000;
        const int bufferedMs = static_cast<int>(static_cast<long long>(diag.bufferedBytes) * 1000 / bps);

        if (diag.state == Audio::Radio::RadioEngine::State::Playing)
        {
            ++playingSamples;
            if (bufferedMs < 250)
            {
                ++silentPlayingSamples;
            }
        }

        if (csv != nullptr)
        {
            fprintf(csv, "%lld,%ls,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                static_cast<long long>(elapsedMs), StateName(diag.state),
                diag.bufferedBytes, bufferedMs, diag.underrunCount, diag.resumeCount,
                diag.playFailures, diag.streamRebuilds, diag.sessions, diag.chunkReads,
                diag.lastChunkMs, diag.maxChunkMs, diag.lastPushGapMs, diag.maxPushGapMs);
            fflush(csv);
        }
    }

    Audio::Radio::RadioEngine::Diagnostics diag;
    engine.GetDiagnostics(diag);
    Audio::Radio::RadioEngine::Snapshot snap;
    engine.GetStatus(snap);

    const int bps = diag.srcBytesPerSecond > 0 ? diag.srcBytesPerSecond : 384000;
    const int bufferedMs = static_cast<int>(static_cast<long long>(diag.bufferedBytes) * 1000 / bps);

    LogLine(L"\n[harness] ===== SUMMARY (%d min, %ls) =====", minutes, url.c_str());
    LogLine(L"[harness] final state: %ls (nowPlaying: '%ls')", StateName(snap.state), snap.nowPlaying);
    LogLine(L"[harness] underruns: %d | resumes: %d | play failures: %d",
        diag.underrunCount, diag.resumeCount, diag.playFailures);
    LogLine(L"[harness] sessions: %d | stream rebuilds (midstream format change): %d",
        diag.sessions, diag.streamRebuilds);
    LogLine(L"[harness] chunk reads: %d | last/max chunk stall ms: %d / %d",
        diag.chunkReads, diag.lastChunkMs, diag.maxChunkMs);
    LogLine(L"[harness] push gap ms last/max: %d / %d",
        diag.lastPushGapMs, diag.maxPushGapMs);
    LogLine(L"[harness] buffered: %d B (~%d ms) | prebuffer target: %d B",
        diag.bufferedBytes, bufferedMs, diag.prebufferTargetBytes);
    LogLine(L"[harness] Playing samples: %d of which silent (<250ms buffered): %d",
        playingSamples, silentPlayingSamples);
    LogLine(L"[harness] VERDICT: %ls",
        diag.underrunCount == 0 && silentPlayingSamples == 0
            ? L"SOLID (0 underruns, no silent Playing samples)"
            : L"FAULTS OBSERVED (see counts above)");

    engine.Stop();
    if (csv != nullptr) fclose(csv);
    if (g_logFile != nullptr) fclose(g_logFile);
    return diag.underrunCount == 0 && silentPlayingSamples == 0 ? 0 : 1;
}
