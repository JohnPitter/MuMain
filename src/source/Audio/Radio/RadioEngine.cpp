#include "stdafx.h"
#include "Audio/Radio/RadioEngine.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>

#include "Audio/AudioPlayer.h"
#include "Audio/Radio/IcyMetadata.h"
#include "Core/Utilities/Log/ErrorReport.h"

#ifdef _WIN32
#pragma comment(lib, "winhttp.lib")
#endif

#define MINIMP3_IMPLEMENTATION
#pragma warning(push)
// minimp3 is vendored public-domain C; its integer narrowing is intentional.
#pragma warning(disable : 4244 4267 4018 4456 4457)
#include "Audio/Radio/minimp3.h"
#pragma warning(pop)

namespace
{
    // Timeouts keep a dead server from stalling the worker forever; Stop()
    // additionally closes the request handle so a session switch is instant.
    constexpr int kResolveTimeoutMs = 10000;
    constexpr int kConnectTimeoutMs = 10000;
    constexpr int kSendTimeoutMs = 10000;
    constexpr int kReceiveTimeoutMs = 15000;

    // Silence between retries while a station is offline. 5s: fast enough that
    // a blip on a flaky stream (Bossa Nova Brazil) heals before the owner
    // notices, slow enough to never hammer a dead server.
    constexpr int kReconnectDelayMs = 5000;
    constexpr int kRetryPollMs = 100;

    // WinHTTP delivers whatever the socket has; 16KB reads amortize the loop
    // overhead without stalling on slow stations.
    constexpr DWORD kReadChunkBytes = 16 * 1024;
    constexpr std::size_t kMaxHeaderWaitBytes = 32 * 1024;

    // Absorb internet jitter: hold this many SECONDS of decoded PCM before
    // the track starts, and stop reading from the socket while the backlog
    // exceeds the cap (TCP flow control paces the server). The old fixed
    // ~0.9s window stuttered ("picotando") on real streams — home Wi-Fi
    // jitter spikes past a second regularly, so a stream that prebuffers in
    // exactly one burst has nothing left when the next burst is late. 4s
    // covers the observed jitter at 128-192 kbps with a tolerable start lag.
    // Expressed in seconds and converted to BYTES of the mixer's destination
    // format at runtime (SDL_GetAudioStreamAvailable counts destination
    // bytes: F32 stereo 48kHz = 384000 B/s, S16 stereo 44.1kHz = 176400 B/s).
    constexpr float kPrebufferSeconds = 4.f;
    // Playback backlog cap. MEASURED (harness, 10 min, Bossa Nova Brazil —
    // before_bossa.csv): real stations deliver in bursts with multi-second
    // dead gaps; sustained production was measured as low as 0.14x realtime
    // over 28s windows. The old 3MB cap (~8.5s) threw away exactly the
    // backlog that bridges those droughts. 12MB ≈ 30s+ at 44.1k stereo F32 —
    // the buffer only grows this deep while the server bursts, and memory is
    // cheap next to silence.
    constexpr int kMaxBufferedBytes = 12 * 1024 * 1024;
    constexpr int kCongestionSleepMs = 30;

    // Underrun recovery (rodada 2, measured): after the mixer track drains
    // dry, re-prime to the FULL window again (round 1's target was right —
    // the harness showed a half-window resume keeps the bank at ~0.5s, which
    // the station's sub-second delivery blips kill 3x more often) but with a
    // SAFETY TIMEOUT. Round 1's real fault was that the full-window wait had
    // NO timeout: a stream trickling slower than realtime stretched it to
    // ~28s of silence while the UI still said "Playing" — the owner's
    // "parou do nada". Measured (before_bossa.csv): delivery blips of 0.5-1s
    // where the server sends NOTHING; the bank must be as deep as possible
    // at every blip.
    constexpr int kReprimeTimeoutMs = 5000;
    constexpr int kMaxTimeoutResumesBeforeReconnect = 3;

    // Same valve for the very first start: don't sit in "Conectando" forever
    // when the connection landed inside a production drought.
    constexpr int kStartTimeoutMs = 8000;

    // Fallback only: ReachedPrebuffer/KickTrackIfDue run after the audio
    // stream exists, which itself requires a live mixer format. Kept so a
    // format probe failure degrades to a sane window instead of zero.
    constexpr int kFallbackPrebufferBytes = 1024 * 1024;

    const wchar_t* const kUserAgent = L"LuxViewRadio/1.0";
    const wchar_t* const kIcyMetaHeader = L"Icy-MetaData: 1\r\n";
}

namespace Audio::Radio
{
    RadioEngine& RadioEngine::Instance()
    {
        static RadioEngine instance;
        return instance;
    }

    RadioEngine::~RadioEngine()
    {
        Shutdown();
    }

    void RadioEngine::Start(const wchar_t* url, const wchar_t* stationName)
    {
        if (url == nullptr || url[0] == L'\0')
        {
            return;
        }

        {
            std::lock_guard lock(m_stateMutex);
            m_url = url;
            m_stationName = stationName != nullptr ? stationName : L"";
            wcsncpy_s(m_snapshot.stationName, m_stationName.c_str(), _TRUNCATE);
            m_snapshot.state = State::Connecting;
        }

        // Bumping the generation fences the previous session: its worker sees
        // the mismatch at the next check and unwinds without touching state.
        // The abort matters: Start() joins the old worker on the UI thread,
        // and a worker blocked in a WinHTTP read would only wake at the
        // receive timeout (~15s UI freeze) without it.
        const std::uint32_t generation = ++m_generation;
        AbortActiveRequest();

        if (m_thread.joinable())
        {
            m_thread.join();
        }
        m_thread = std::thread(&RadioEngine::ThreadMain, this, generation);
    }

    void RadioEngine::Stop()
    {
        ++m_generation;

        // Closing the request aborts a blocking WinHTTP read immediately, so
        // the join below never waits out the full receive timeout.
        AbortActiveRequest();

        PublishState(State::Off);

        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void RadioEngine::AbortActiveRequest()
    {
        std::lock_guard netLock(m_netMutex);
        if (m_request != nullptr)
        {
            WinHttpCloseHandle(m_request);
            m_request = nullptr;
        }
    }

    void RadioEngine::Shutdown()
    {
        Stop();
        DestroyMixerTrack();
    }

    void RadioEngine::SetVolume(int level0to100)
    {
        const int clamped = std::clamp(level0to100, 0, 100);
        std::lock_guard lock(m_stateMutex);
        m_volume = clamped;
        if (m_track != nullptr)
        {
            MIX_SetTrackGain(m_track, static_cast<float>(clamped) / 100.f);
        }
    }

    void RadioEngine::GetStatus(Snapshot& out) const
    {
        std::lock_guard lock(m_stateMutex);
        out = m_snapshot;
    }

    void RadioEngine::GetDiagnostics(Diagnostics& out) const
    {
        std::lock_guard lock(m_stateMutex);
        out = m_diag;
        out.underrunCount = m_underrunCount;
        out.generation = m_generation.load(std::memory_order_acquire);
        // Best-effort live buffer level: 0 when no stream exists (idle).
        out.bufferedBytes = m_stream != nullptr ? SDL_GetAudioStreamAvailable(m_stream) : 0;
        out.prebufferTargetBytes = PrebufferTargetBytes();
        if (m_stream != nullptr)
        {
            SDL_AudioSpec dst {};
            SDL_GetAudioStreamFormat(m_stream, nullptr, &dst);
            out.srcBytesPerSecond = SDL_AUDIO_BYTESIZE(dst.format) * dst.channels * dst.freq;
        }
        else
        {
            out.srcBytesPerSecond = 0;
        }
    }

    void RadioEngine::PublishTitle(const char* data, std::size_t size)
    {
        std::string title;
        if (!IcyMetadata::ExtractStreamTitle(data, size, title))
        {
            return;
        }

        std::wstring wide;
        IcyMetadata::Utf8ToWide(title, wide);
        if (wide.empty())
        {
            return;
        }

        std::lock_guard lock(m_stateMutex);
        wcsncpy_s(m_snapshot.nowPlaying, wide.c_str(), _TRUNCATE);
    }

    void RadioEngine::PublishState(State state)
    {
        std::lock_guard lock(m_stateMutex);
        m_snapshot.state = state;
        m_diag.state = state;
        if (state == State::Off)
        {
            m_snapshot.nowPlaying[0] = L'\0';
        }
    }

    bool RadioEngine::EnsureMixerTrack()
    {
        MIX_Mixer* mixer = AudioPlayer::GetMixer();
        if (mixer == nullptr)
        {
            return false;
        }

        if (m_track != nullptr)
        {
            return true;
        }

        m_track = MIX_CreateTrack(mixer);
        if (m_track == nullptr)
        {
            g_ErrorReport.Write(L"[radio] MIX_CreateTrack failed: %hs\r\n", SDL_GetError());
            return false;
        }

        int volume = 0;
        {
            std::lock_guard lock(m_stateMutex);
            volume = m_volume;
        }
        MIX_SetTrackGain(m_track, static_cast<float>(volume) / 100.f);
        return true;
    }

    bool RadioEngine::EnsureAudioStream(int channels, int sampleRate)
    {
        if (channels <= 0 || sampleRate <= 0)
        {
            return false;
        }

        // Stations differ in rate/channel count; (re)build the conversion
        // stream whenever the decode format changes.
        if (m_stream != nullptr && m_streamChannels == channels && m_streamRate == sampleRate)
        {
            return true;
        }

        MIX_Mixer* mixer = AudioPlayer::GetMixer();
        SDL_AudioSpec dstSpec {};
        if (mixer == nullptr || !MIX_GetMixerFormat(mixer, &dstSpec))
        {
            return false;
        }

        if (m_stream != nullptr)
        {
            if (m_track != nullptr)
            {
                MIX_SetTrackAudioStream(m_track, nullptr);
            }
            SDL_DestroyAudioStream(m_stream);
            m_stream = nullptr;
            ++m_diag.streamRebuilds;  // station changed rate/channels midstream
        }

        const SDL_AudioSpec srcSpec { SDL_AUDIO_S16LE, channels, sampleRate };
        m_stream = SDL_CreateAudioStream(&srcSpec, &dstSpec);
        if (m_stream == nullptr)
        {
            g_ErrorReport.Write(L"[radio] SDL_CreateAudioStream failed: %hs\r\n", SDL_GetError());
            return false;
        }
        m_streamChannels = channels;
        m_streamRate = sampleRate;

        if (m_track != nullptr && !MIX_SetTrackAudioStream(m_track, m_stream))
        {
            g_ErrorReport.Write(L"[radio] MIX_SetTrackAudioStream failed: %hs\r\n", SDL_GetError());
            SDL_DestroyAudioStream(m_stream);
            m_stream = nullptr;
            return false;
        }

        return true;
    }

    int RadioEngine::DestinationBytesPerSecond() const
    {
        MIX_Mixer* mixer = AudioPlayer::GetMixer();
        SDL_AudioSpec spec {};
        if (mixer == nullptr || !MIX_GetMixerFormat(mixer, &spec))
        {
            return 0;
        }
        return SDL_AUDIO_BYTESIZE(spec.format) * spec.channels * spec.freq;
    }

    int RadioEngine::PrebufferTargetBytes() const
    {
        // Measure against the STREAM's destination format, not the mixer's:
        // SDL_mixer pins the track input stream's output format to F32 at the
        // STATION's rate/channels (MIX_SetTrackAudioStream rewrites it), so a
        // 44.1 kHz station buffers 352800 B/s of audio — targetting the
        // mixer's 384000 B/s made "4 seconds" silently mean 4.35.
        if (m_stream != nullptr)
        {
            SDL_AudioSpec dst {};
            if (SDL_GetAudioStreamFormat(m_stream, nullptr, &dst))
            {
                const int bytesPerSecond =
                    SDL_AUDIO_BYTESIZE(dst.format) * dst.channels * dst.freq;
                if (bytesPerSecond > 0)
                {
                    return static_cast<int>(static_cast<float>(bytesPerSecond) * kPrebufferSeconds);
                }
            }
        }

        const int bytesPerSecond = DestinationBytesPerSecond();
        if (bytesPerSecond <= 0)
        {
            return kFallbackPrebufferBytes;
        }
        return static_cast<int>(static_cast<float>(bytesPerSecond) * kPrebufferSeconds);
    }

    bool RadioEngine::ReachedPrebuffer() const
    {
        return m_stream != nullptr && SDL_GetAudioStreamAvailable(m_stream) >= PrebufferTargetBytes();
    }

    void RadioEngine::DestroyMixerTrack()
    {
        if (m_stream != nullptr)
        {
            if (m_track != nullptr)
            {
                MIX_SetTrackAudioStream(m_track, nullptr);
            }
            SDL_DestroyAudioStream(m_stream);
            m_stream = nullptr;
        }
        m_streamChannels = 0;
        m_streamRate = 0;

        if (m_track != nullptr)
        {
            MIX_DestroyTrack(m_track);
            m_track = nullptr;
        }

        if (m_decoder != nullptr)
        {
            delete static_cast<mp3dec_t*>(m_decoder);
            m_decoder = nullptr;
        }
    }

    void RadioEngine::ThreadMain(std::uint32_t generation)
    {
        // The renderer hammers the CPU every frame; this thread's cadence IS
        // the buffer refill, so keep it above the default pool. It idles >99%
        // of the time (blocked on the socket), so this costs nothing else.
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

        while (generation == m_generation.load(std::memory_order_acquire))
        {
            {
                std::lock_guard lock(m_stateMutex);
                m_snapshot.nowPlaying[0] = L'\0';
            }

            if (!EnsureMixerTrack())
            {
                g_ErrorReport.Write(L"[radio] mixer unavailable, radio stays off\r\n");
                break;
            }

            StreamConnection(generation);
            if (generation != m_generation.load(std::memory_order_acquire))
            {
                break;
            }

            // Every exit from StreamConnection (server closed, dial-up blip,
            // station offline) becomes a silent retry. The wait polls the
            // generation so Start()/Stop() cut it short.
            PublishState(State::Reconnecting);
            g_ErrorReport.Write(L"[radio] stream ended, retrying in %d ms\r\n", kReconnectDelayMs);

            const auto deadline = std::chrono::steady_clock::now()
                + std::chrono::milliseconds(kReconnectDelayMs);
            while (std::chrono::steady_clock::now() < deadline
                && generation == m_generation.load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(kRetryPollMs));
            }
        }
    }

    bool RadioEngine::StreamConnection(std::uint32_t generation)
    {
        {
            std::lock_guard lock(m_stateMutex);
            ++m_diag.sessions;
        }

        std::wstring url;
        {
            std::lock_guard lock(m_stateMutex);
            url = m_url;
        }

        URL_COMPONENTS parts {};
        parts.dwStructSize = sizeof(parts);
        wchar_t host[256] = {};
        wchar_t path[1024] = {};
        parts.lpszHostName = host;
        parts.dwHostNameLength = 255;
        parts.lpszUrlPath = path;
        parts.dwUrlPathLength = 1023;

        if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts))
        {
            g_ErrorReport.Write(L"[radio] WinHttpCrackUrl failed for %ls\r\n", url.c_str());
            return false;
        }

        const bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
        HINTERNET session = WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (session == nullptr)
        {
            g_ErrorReport.Write(L"[radio] WinHttpOpen failed (0x%08X)\r\n", GetLastError());
            return false;
        }
        WinHttpSetTimeouts(session, kResolveTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs, kReceiveTimeoutMs);

        HINTERNET connect = WinHttpConnect(session, host, parts.nPort, 0);
        HINTERNET request = nullptr;
        bool streamed = false;

        if (connect != nullptr)
        {
            request = WinHttpOpenRequest(connect, L"GET", path[0] != L'\0' ? path : L"/",
                nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                secure ? WINHTTP_FLAG_SECURE : 0);
        }
        else
        {
            g_ErrorReport.Write(L"[radio] connect to %ls:%u failed (0x%08X)\r\n",
                host, parts.nPort, GetLastError());
        }

        if (request != nullptr)
        {
            {
                std::lock_guard netLock(m_netMutex);
                m_request = request;
            }

            const BOOL sent = WinHttpSendRequest(request, kIcyMetaHeader,
                static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
            if (sent && WinHttpReceiveResponse(request, nullptr))
            {
                streamed = StreamLoop(request, generation);
            }
            else if (generation == m_generation.load(std::memory_order_acquire))
            {
                g_ErrorReport.Write(L"[radio] request to %ls failed (0x%08X)\r\n", url.c_str(), GetLastError());
            }
        }

        // If Stop() already closed the handle to abort a blocked read, do not
        // close the same handle value a second time here.
        bool requestClosedByStop = false;
        {
            std::lock_guard netLock(m_netMutex);
            if (m_request != nullptr)
            {
                m_request = nullptr;
            }
            else
            {
                requestClosedByStop = true;
            }
        }
        if (request != nullptr && !requestClosedByStop)
        {
            WinHttpCloseHandle(request);
        }
        if (connect != nullptr)
        {
            WinHttpCloseHandle(connect);
        }
        WinHttpCloseHandle(session);

        // An ended connection leaves buffered audio from the dead session;
        // flush it and rebind the stream so the retry starts clean.
        if (m_stream != nullptr)
        {
            SDL_ClearAudioStream(m_stream);
            if (m_track != nullptr)
            {
                MIX_SetTrackAudioStream(m_track, nullptr);
                MIX_SetTrackAudioStream(m_track, m_stream);
            }
        }

        return streamed;
    }

    bool RadioEngine::StreamLoop(HINTERNET request, std::uint32_t generation)
    {
        if (m_decoder == nullptr)
        {
            m_decoder = new mp3dec_t {};
        }
        mp3dec_t* decoder = static_cast<mp3dec_t*>(m_decoder);
        mp3dec_init(decoder);

        std::vector<char> chunk(kReadChunkBytes);
        std::vector<char> pending;
        std::vector<uint8_t> mp3Buffer;

        int metaInterval = 0;
        int bytesUntilMeta = 0;
        bool headerDone = false;
        bool started = false;
        bool awaitingReprime = false;

        // Kick helper: shared by the start path and underrun recovery so a
        // refused MIX_PlayTrack is counted (a silent refusal here reads as
        // "the radio stopped out of nowhere" with nothing in the log).
        auto KickTrack = [this]() -> bool
        {
            if (m_track == nullptr)
            {
                return false;
            }
            if (!MIX_PlayTrack(m_track, 0))
            {
                std::lock_guard lock(m_stateMutex);
                ++m_diag.playFailures;
                g_ErrorReport.Write(L"[radio] MIX_PlayTrack failed: %hs\r\n", SDL_GetError());
                return false;
            }
            return true;
        };

        auto lastPush = std::chrono::steady_clock::now();
        auto underrunSince = std::chrono::steady_clock::now();
        auto startSince = std::chrono::steady_clock::now();
        int timeoutResumeStreak = 0;

        while (generation == m_generation.load(std::memory_order_acquire))
        {
            const auto chunkStart = std::chrono::steady_clock::now();
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request, &available) || available == 0)
            {
                g_ErrorReport.Write(L"[radio] read loop end: query avail=%lu err=0x%08X\r\n",
                    available, GetLastError());
                break;
            }
            const DWORD toRead = std::min<DWORD>(available, kReadChunkBytes);
            DWORD readBytes = 0;
            if (!WinHttpReadData(request, chunk.data(), toRead, &readBytes) || readBytes == 0)
            {
                g_ErrorReport.Write(L"[radio] read loop end: read %lu/%lu bytes err=0x%08X\r\n",
                    readBytes, toRead, GetLastError());
                break;
            }

            {
                const auto chunkMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - chunkStart).count();
                std::lock_guard lock(m_stateMutex);
                ++m_diag.chunkReads;
                m_diag.lastChunkMs = static_cast<int>(chunkMs);
                if (chunkMs > m_diag.maxChunkMs)
                {
                    m_diag.maxChunkMs = static_cast<int>(chunkMs);
                }
            }

            if (!headerDone)
            {
                pending.insert(pending.end(), chunk.data(), chunk.data() + readBytes);
                const std::size_t headerEnd = IcyMetadata::FindHeaderEnd(pending.data(), pending.size());
                if (headerEnd == IcyMetadata::npos)
                {
                    if (pending.size() > kMaxHeaderWaitBytes)
                    {
                        break;  // not an HTTP response at all
                    }
                    continue;
                }

                metaInterval = IcyMetadata::GetMetaInterval(pending.data(), headerEnd);
                bytesUntilMeta = metaInterval;
                const std::string icyName = IcyMetadata::GetHeaderValue(pending.data(), headerEnd, "icy-name");
                if (!icyName.empty())
                {
                    std::wstring station;
                    IcyMetadata::Utf8ToWide(icyName, station);
                    std::lock_guard lock(m_stateMutex);
                    wcsncpy_s(m_snapshot.stationName, station.c_str(), _TRUNCATE);
                }

                mp3Buffer.assign(pending.begin() + headerEnd, pending.end());
                pending.clear();
                headerDone = true;
            }
            else
            {
                pending.insert(pending.end(), chunk.data(), chunk.data() + readBytes);
            }

            // Split pending into MP3 payloads and ICY metadata blocks
            // (metaint stream bytes, then 1 length byte + length*16 bytes).
            std::size_t offset = 0;
            while (offset < pending.size())
            {
                if (metaInterval == 0)
                {
                    mp3Buffer.insert(mp3Buffer.end(), pending.begin() + offset, pending.end());
                    offset = pending.size();
                    break;
                }

                if (bytesUntilMeta > 0)
                {
                    const std::size_t take = std::min<std::size_t>(
                        bytesUntilMeta, pending.size() - offset);
                    mp3Buffer.insert(mp3Buffer.end(), pending.begin() + offset,
                        pending.begin() + offset + take);
                    offset += take;
                    bytesUntilMeta -= static_cast<int>(take);
                    continue;
                }

                if (offset >= pending.size())
                {
                    break;
                }

                const int metaLength = static_cast<unsigned char>(pending[offset]) * 16;
                if (offset + 1 + metaLength > pending.size())
                {
                    break;  // wait for the rest of the metadata block
                }
                if (metaLength > 0)
                {
                    PublishTitle(pending.data() + offset + 1, metaLength);
                }
                offset += 1 + metaLength;
                bytesUntilMeta = metaInterval;
            }
            pending.erase(pending.begin(), pending.begin() + offset);

            if (!headerDone)
            {
                continue;
            }

            // Decode every complete MP3 frame currently buffered.
            std::size_t decoded = 0;
            while (decoded + 4 <= mp3Buffer.size())
            {
                mp3dec_frame_info_t info {};
                const int samples = mp3dec_decode_frame(decoder,
                    mp3Buffer.data() + decoded, static_cast<int>(mp3Buffer.size() - decoded),
                    m_pcmBuffer, &info);
                if (info.frame_bytes <= 0)
                {
                    break;  // need more data (resyncs on the next read)
                }
                decoded += info.frame_bytes;

                if (samples > 0 && EnsureAudioStream(info.channels, info.hz))
                {
                    const int bytes = samples * info.channels * static_cast<int>(sizeof(int16_t));
                    SDL_PutAudioStreamData(m_stream, m_pcmBuffer, bytes);
                }
            }
            if (decoded > 0)
            {
                mp3Buffer.erase(mp3Buffer.begin(), mp3Buffer.begin() + decoded);

                const auto now = std::chrono::steady_clock::now();
                const auto gapMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - lastPush).count();
                lastPush = now;
                std::lock_guard lock(m_stateMutex);
                m_diag.lastPushGapMs = static_cast<int>(gapMs);
                if (gapMs > m_diag.maxPushGapMs)
                {
                    m_diag.maxPushGapMs = static_cast<int>(gapMs);
                }
            }

            if (!started)
            {
                // Full window OR the drought safety valve: a bursty station
                // can land the first connection inside a production drought,
                // and demanding the full 4s there means "Conectando" for
                // minutes. After 8s with audible audio queued, just start.
                const auto waitedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startSince).count();
                const int floor = PrebufferTargetBytes() / 16;
                const int bufferedNow = m_stream != nullptr
                    ? SDL_GetAudioStreamAvailable(m_stream) : 0;
                if (ReachedPrebuffer()
                    || (waitedMs >= kStartTimeoutMs && bufferedNow > floor))
                {
                    started = true;
                    if (KickTrack())
                    {
                        PublishState(State::Playing);
                        g_ErrorReport.Write(L"[radio] playing (%.1fs prebuffered%s, %d B queued)\r\n",
                            kPrebufferSeconds,
                            ReachedPrebuffer() ? L"" : L" — drought start",
                            bufferedNow);
                    }
                }
            }
            else
            {
                // Underrun recovery with a full re-prime: when the stream runs
                // dry the mixer stops the track. The old logic re-kicked after
                // a quarter of the (tiny) prebuffer, so it started again with
                // ~0.2s of slack and stuttered in a start/stop ping-pong. Now:
                //   - buffer still substantial  -> resume right away;
                //   - buffer essentially drained -> count the underrun, wait
                //     for the FULL prebuffer window again, then resume.
                const int buffered = m_stream != nullptr
                    ? SDL_GetAudioStreamAvailable(m_stream) : 0;
                if (!MIX_TrackPlaying(m_track) && m_track != nullptr)
                {
                    const auto now = std::chrono::steady_clock::now();
                    // Instant resume only with a HEALTHY bank (~1s). Round 1
                    // re-kicked at prebuffer/16 (~0.25s): the station's
                    // sub-second delivery blips then produced the thin-kick
                    // machine-gun measured in before_bossa.csv (201 track
                    // re-kicks in 10 minutes). Below the floor it is a real
                    // underrun: count it, show Buffering, re-prime deeply.
                    if (buffered > PrebufferTargetBytes() / 4)
                    {
                        if (KickTrack())
                        {
                            std::lock_guard lock(m_stateMutex);
                            ++m_diag.resumeCount;
                        }
                    }
                    else if (!awaitingReprime)
                    {
                        ++m_underrunCount;
                        awaitingReprime = true;
                        underrunSince = now;
                        PublishState(State::Buffering);
                        g_ErrorReport.Write(
                            L"[radio] underrun #%d (buffered %d B) — re-priming %.0fs before resume\r\n",
                            m_underrunCount, buffered, kPrebufferSeconds);
                    }
                    else
                    {
                        // Re-prime to the FULL window; the timeout below is
                        // the safety valve for streams that deliver slower
                        // than realtime (measured: Bossa Nova Brazil sends
                        // NOTHING in 65% of 250ms windows, with 0.5-1s
                        // blips — the client can only bank through them).
                        const int reprimeTarget = PrebufferTargetBytes();
                        const bool primed = buffered >= reprimeTarget;
                        const auto waitedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - underrunSince).count();
                        const bool timedOut = waitedMs >= kReprimeTimeoutMs
                            && buffered > PrebufferTargetBytes() / 16;

                        if (primed || timedOut)
                        {
                            awaitingReprime = false;
                            if (KickTrack())
                            {
                                PublishState(State::Playing);
                                if (timedOut && !primed)
                                {
                                    ++timeoutResumeStreak;
                                    std::lock_guard lock(m_stateMutex);
                                    ++m_diag.resumeCount;
                                    g_ErrorReport.Write(
                                        L"[radio] resumed after underrun #%d on timeout (%lld ms, %d B) — streak %d\r\n",
                                        m_underrunCount, static_cast<long long>(waitedMs),
                                        buffered, timeoutResumeStreak);
                                }
                                else
                                {
                                    timeoutResumeStreak = 0;
                                    std::lock_guard lock(m_stateMutex);
                                    ++m_diag.resumeCount;
                                    g_ErrorReport.Write(L"[radio] resumed after underrun #%d\r\n",
                                        m_underrunCount);
                                }
                            }

                            // A connection that keeps starving even while open
                            // is wedged (NAT half-open, throttled server):
                            // drop it and let the retry open a fresh one.
                            if (timeoutResumeStreak >= kMaxTimeoutResumesBeforeReconnect)
                            {
                                g_ErrorReport.Write(
                                    L"[radio] %d consecutive timeout resumes — dropping wedged connection\r\n",
                                    timeoutResumeStreak);
                                break;
                            }
                        }
                    }
                }

                // Back off while the backlog exceeds the cap: don't read more
                // from the socket until the mixer drains below it. Bail out
                // early if the track stopped — the underrun branch above must
                // get a chance to run instead of sleeping through it.
                while (m_stream != nullptr
                    && SDL_GetAudioStreamAvailable(m_stream) > kMaxBufferedBytes
                    && MIX_TrackPlaying(m_track)
                    && generation == m_generation.load(std::memory_order_acquire))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(kCongestionSleepMs));
                }
            }
        }

        return started;
    }
}
