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
            Playing,        // PCM flowing to the mixer
            Reconnecting,   // stream died, next retry scheduled
        };

        struct Snapshot
        {
            State state = State::Off;
            wchar_t nowPlaying[160] = L"";
            wchar_t stationName[64] = L"";
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

    private:
        RadioEngine() = default;
        ~RadioEngine();
        RadioEngine(const RadioEngine&) = delete;
        RadioEngine& operator=(const RadioEngine&) = delete;

        void ThreadMain(std::uint32_t generation);
        bool StreamConnection(std::uint32_t generation);
        bool StreamLoop(HINTERNET request, std::uint32_t generation);
        void PublishTitle(const char* data, std::size_t size);
        void PublishState(State state);

        bool EnsureMixerTrack();
        bool EnsureAudioStream(int channels, int sampleRate);
        bool ReachedPrebuffer() const;
        void KickTrackIfDue();
        void DestroyMixerTrack();

        mutable std::mutex m_stateMutex;
        Snapshot m_snapshot;
        std::wstring m_url;
        std::wstring m_stationName;
        int m_volume = 0;               // 0..100, guarded by m_stateMutex

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
    };
}
