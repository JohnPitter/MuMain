#pragma once

// Radio streaming engine: downloads a Shoutcast/Icecast MP3 stream over
// WinHTTP, decodes it with minimp3 and feeds PCM into an SDL_mixer track on
// the game's shared mixer (the same mixer as game music — AudioPlayer — but a
// dedicated track with its own gain, so the radio volume is independent).
//
// Threading contract:
//   - Start/Stop/SetVolume/GetStatus/Shutdown are called from the main thread.
//   - All WinHTTP + decode work runs on one worker thread owned by the
//     engine. The main thread NEVER blocks on network or decode.
//   - Every connection attempt is tagged with a generation counter; Start and
//     Stop bump the generation so a stale connection loop unwinds without
//     touching state owned by a newer session, and Stop closes the request
//     handle so a blocked read aborts immediately instead of at the timeout.
//   - Failure policy: log + retry every kReconnectDelayMs, silently, forever
//     (never ExitProcess, never a fatal MessageBox).

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <winhttp.h>

#include "Audio/Radio/TitleTimeline.h"

struct MIX_Track;
struct SDL_AudioStream;
// minimp3's mp3dec_t is an anonymous-struct typedef, so it cannot be
// forward-declared; the header owns it as an opaque handle instead.

namespace Audio::Radio
{
    class RadioEngine
    {
    public:
        enum class State
        {
            Off,            // no session
            Connecting,     // request in flight, no audio yet
            Buffering,      // audible stream stalled: re-priming before resume
            Playing,        // PCM flowing to the mixer
            Reconnecting,   // stream died, next retry scheduled
        };

        struct Snapshot
        {
            State state = State::Off;
            wchar_t nowPlaying[160] = L"";
            wchar_t stationName[64] = L"";
        };

        // Streaming telemetry, sampled by the worker while it runs (cheap
        // counters; a monitor can poll GetDiagnostics at any rate). Exposed so
        // stutter/"parou do nada" reports can be diagnosed from real numbers
        // instead of guesses: feed cadence (push gaps), network stalls (chunk
        // times), buffer level, and the state machine's recovery counters.
        struct Diagnostics
        {
            State state = State::Off;
            int bufferedBytes = 0;          // decoded PCM waiting in the stream (dst format)
            int prebufferTargetBytes = 0;   // "started" threshold in the same unit
            int srcBytesPerSecond = 0;      // stream dst-format rate (station rate, F32)
            int underrunCount = 0;          // times the mixer track drained dry
            int resumeCount = 0;            // successful re-kicks/re-primes
            int playFailures = 0;           // MIX_PlayTrack refused
            int streamRebuilds = 0;         // station changed rate/channels midstream
            int sessions = 0;               // StreamConnection attempts
            int chunkReads = 0;             // network chunks pulled from WinHTTP
            int lastChunkMs = 0;            // blocked time of the last chunk read
            int maxChunkMs = 0;             // worst chunk read of the current session
            int lastPushGapMs = 0;          // gap between two PCM push bursts
            int maxPushGapMs = 0;
            std::uint32_t generation = 0;   // bumped by Start()/Stop()
        };

        static RadioEngine& Instance();

        // Starts (or restarts) streaming `url`. Safe to call while another
        // session is running: the old connection is torn down first.
        void Start(const wchar_t* url, const wchar_t* stationName);

        // Stops streaming and parks the worker thread. Idempotent.
        void Stop();

        // Full teardown: Stop() + release the mixer track/stream. Wired into
        // DestroySound() (Winmain.cpp) BEFORE AudioPlayer::Shutdown, because
        // the radio track hangs off the shared mixer.
        void Shutdown();

        // Volume on the UI's 0..100 scale, applied as the mixer track gain.
        void SetVolume(int level0to100);

        void GetStatus(Snapshot& out) const;
        void GetDiagnostics(Diagnostics& out) const;

    private:
        RadioEngine() = default;
        ~RadioEngine();
        RadioEngine(const RadioEngine&) = delete;
        RadioEngine& operator=(const RadioEngine&) = delete;

        void ThreadMain(std::uint32_t generation);
        bool StreamConnection(std::uint32_t generation);
        bool StreamLoop(HINTERNET request, std::uint32_t generation);
        bool ExtractTitleWide(const char* data, std::size_t size, std::wstring& outWide);
        void PublishState(State state);

        // Closes the in-flight request (if any) so a worker blocked inside a
        // WinHTTP read wakes immediately. Used by Stop() AND Start() — Start
        // joins the old worker on the caller (UI) thread, and without the
        // abort that join could block for a full receive timeout.
        void AbortActiveRequest();

        bool EnsureMixerTrack();
        bool EnsureAudioStream(int channels, int sampleRate);
        int DestinationBytesPerSecond() const;
        int PrebufferTargetBytes() const;
        bool ReachedPrebuffer() const;
        void DestroyMixerTrack();

        mutable std::mutex m_stateMutex;
        Snapshot m_snapshot;
        std::wstring m_url;
        std::wstring m_stationName;
        int m_volume = 0;               // 0..100, guarded by m_stateMutex

        // Now-playing sync (see TitleTimeline.h): m_snapshot.nowPlaying holds
        // the LATEST DOWNLOADED title, which runs one pre-buffer (~4s, up to
        // the backlog cap) ahead of the audible audio. m_titles binds every
        // title to the stream position where its audio begins (microseconds
        // of decoded audio, worker-written under m_stateMutex) and GetStatus
        // answers with the title that owns the CURRENT PLAYBACK position:
        // produced (m_producedUs) minus still-queued (SDL_GetAudioStream-
        // Available). We count our own stream accounting instead of
        // MIX_GetTrackPlaybackPosition because that input position is not
        // defined for MIX_SetTrackAudioStream tracks (the seek API refuses
        // them) — our counters are exact by construction.
        TitleTimeline m_titles;
        std::atomic<std::int64_t> m_producedUs { 0 };  // decoded PCM pushed to m_stream, us

        // The in-flight WinHTTP request, closed by Stop() to abort a blocked
        // read immediately.
        std::mutex m_netMutex;
        HINTERNET m_request = nullptr;

        std::thread m_thread;
        std::atomic<std::uint32_t> m_generation { 0 };

        // Mixer objects: created lazily by the worker on first PCM, destroyed
        // on the caller thread from Shutdown() after the worker has been
        // joined.
        MIX_Track* m_track = nullptr;
        SDL_AudioStream* m_stream = nullptr;
        int m_streamChannels = 0;
        int m_streamRate = 0;
        void* m_decoder = nullptr;   // opaque minimp3 mp3dec_t, owned by the .cpp

        // One minimp3 frame of interleaved stereo (1152 * 2 samples max).
        static constexpr int kMaxSamplesPerFrame = 1152 * 2;
        int16_t m_pcmBuffer[kMaxSamplesPerFrame * 2] = {};

        // Underrun telemetry. Worker-thread only (StreamLoop is the single
        // writer/reader); surfaced through g_ErrorReport lines and
        // GetDiagnostics (copied under m_stateMutex).
        int m_underrunCount = 0;

        // Worker-written diagnostics copy; GetDiagnostics snapshots it under
        // m_stateMutex. The worker updates it at chunk boundaries only (a few
        // times per second), so the lock cost is negligible.
        Diagnostics m_diag;
    };
}
