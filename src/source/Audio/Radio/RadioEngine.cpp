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

    // Silence between retries while a station is offline.
    constexpr int kReconnectDelayMs = 10000;
    constexpr int kRetryPollMs = 100;

    constexpr DWORD kReadChunkBytes = 8192;
    constexpr std::size_t kMaxHeaderWaitBytes = 32 * 1024;

    // Absorb network jitter: hold this much decoded PCM (in mixer-format
    // bytes) before the track starts, and stop reading from the socket while
    // the backlog exceeds the cap (TCP flow control paces the server).
    constexpr int kPrebufferBytes = 320 * 1024;   // ~0.9s of 44.1kHz stereo S16
    constexpr int kMaxBufferedBytes = 3 * 1024 * 1024;
    constexpr int kCongestionSleepMs = 30;

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
        const std::uint32_t generation = ++m_generation;

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
        {
            std::lock_guard netLock(m_netMutex);
            if (m_request != nullptr)
            {
                WinHttpCloseHandle(m_request);
                m_request = nullptr;
            }
        }

        PublishState(State::Off);

        if (m_thread.joinable())
        {
            m_thread.join();
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

    bool RadioEngine::ReachedPrebuffer() const
    {
        return m_stream != nullptr && SDL_GetAudioStreamAvailable(m_stream) >= kPrebufferBytes;
    }

    void RadioEngine::KickTrackIfDue()
    {
        if (m_track == nullptr || m_stream == nullptr)
        {
            return;
        }

        // Underrun recovery: the stream went dry and the mixer stopped the
        // track; once a little audio is queued again, start pulling.
        const int buffered = SDL_GetAudioStreamAvailable(m_stream);
        if (!MIX_TrackPlaying(m_track) && buffered >= kPrebufferBytes / 4)
        {
            MIX_PlayTrack(m_track, 0);
        }
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

        while (generation == m_generation.load(std::memory_order_acquire))
        {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request, &available) || available == 0)
            {
                break;
            }
            const DWORD toRead = std::min<DWORD>(available, kReadChunkBytes);
            DWORD readBytes = 0;
            if (!WinHttpReadData(request, chunk.data(), toRead, &readBytes) || readBytes == 0)
            {
                break;
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
            }

            if (!started)
            {
                if (ReachedPrebuffer())
                {
                    started = true;
                    if (m_track != nullptr)
                    {
                        MIX_PlayTrack(m_track, 0);
                    }
                    PublishState(State::Playing);
                    g_ErrorReport.Write(L"[radio] playing\r\n");
                }
            }
            else
            {
                KickTrackIfDue();

                // Back off while the backlog exceeds the cap: don't read more
                // from the socket until the mixer drains below it.
                while (m_stream != nullptr
                    && SDL_GetAudioStreamAvailable(m_stream) > kMaxBufferedBytes
                    && generation == m_generation.load(std::memory_order_acquire))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(kCongestionSleepMs));
                }
            }
        }

        return started;
    }
}
