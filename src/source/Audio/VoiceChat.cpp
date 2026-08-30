#include "stdafx.h"
#include "Audio/VoiceChat.h"

#include <SDL3/SDL_audio.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"

namespace
{
    constexpr int kPcmBytesPerFrame = VoiceChat::SamplesPerFrame * static_cast<int>(sizeof(int16_t));
    constexpr int kAdpcmDataBytes = VoiceChat::EncodedFrameBytes - 3;
    constexpr int kPacketHeaderBytes = 4;
    constexpr int kServerSenderBytes = 2;
    constexpr int kOutgoingPacketBytes = kPacketHeaderBytes + VoiceChat::EncodedFrameBytes;
    constexpr int kIncomingPayloadBytes = kServerSenderBytes + VoiceChat::EncodedFrameBytes;
    constexpr int kVoiceActivityThreshold = 300;
    constexpr int kMaxQueuedPlaybackFrames = 12;

    constexpr std::array<int, 89> kStepTable = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
        34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
        143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
        494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
        1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
        4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
        11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
        27086, 29794, 32767,
    };

    constexpr std::array<int, 16> kIndexAdjustment = {
        -1, -1, -1, -1, 2, 4, 6, 8,
        -1, -1, -1, -1, 2, 4, 6, 8,
    };

    struct AdpcmState
    {
        int Predictor = 0;
        int StepIndex = 0;
    };

    struct VoiceState
    {
        SDL_AudioStream* Capture = nullptr;
        SDL_AudioStream* Playback = nullptr;
        bool MicrophoneEnabled = false;
        bool ListeningEnabled = true;
    };

    VoiceState g_voice;

    SDL_AudioSpec CreateVoiceSpec()
    {
        SDL_AudioSpec spec {};
        spec.format = SDL_AUDIO_S16LE;
        spec.channels = 1;
        spec.freq = VoiceChat::SampleRate;
        return spec;
    }

    int ClampPredictor(int value)
    {
        return std::clamp(value, static_cast<int>(INT16_MIN), static_cast<int>(INT16_MAX));
    }

    unsigned char EncodeSample(int sample, AdpcmState& state)
    {
        const int step = kStepTable[state.StepIndex];
        int difference = sample - state.Predictor;
        unsigned char code = 0;
        if (difference < 0)
        {
            code = 8;
            difference = -difference;
        }

        int delta = step >> 3;
        if (difference >= step)
        {
            code |= 4;
            difference -= step;
            delta += step;
        }
        if (difference >= (step >> 1))
        {
            code |= 2;
            difference -= step >> 1;
            delta += step >> 1;
        }
        if (difference >= (step >> 2))
        {
            code |= 1;
            delta += step >> 2;
        }

        state.Predictor = ClampPredictor(state.Predictor + ((code & 8) != 0 ? -delta : delta));
        state.StepIndex = std::clamp(state.StepIndex + kIndexAdjustment[code], 0, static_cast<int>(kStepTable.size() - 1));
        return code;
    }

    int16_t DecodeSample(unsigned char code, AdpcmState& state)
    {
        const int step = kStepTable[state.StepIndex];
        int delta = step >> 3;
        if ((code & 4) != 0)
            delta += step;
        if ((code & 2) != 0)
            delta += step >> 1;
        if ((code & 1) != 0)
            delta += step >> 2;

        state.Predictor = ClampPredictor(state.Predictor + ((code & 8) != 0 ? -delta : delta));
        state.StepIndex = std::clamp(state.StepIndex + kIndexAdjustment[code], 0, static_cast<int>(kStepTable.size() - 1));
        return static_cast<int16_t>(state.Predictor);
    }

    void EncodeFrame(const std::array<int16_t, VoiceChat::SamplesPerFrame>& pcm, std::array<unsigned char, VoiceChat::EncodedFrameBytes>& output)
    {
        AdpcmState state { pcm.front(), 0 };
        output[0] = static_cast<unsigned char>(state.Predictor & 0xFF);
        output[1] = static_cast<unsigned char>((state.Predictor >> 8) & 0xFF);
        output[2] = static_cast<unsigned char>(state.StepIndex);

        for (int i = 0; i < VoiceChat::SamplesPerFrame; i += 2)
        {
            const unsigned char low = EncodeSample(pcm[i], state);
            const unsigned char high = EncodeSample(pcm[i + 1], state);
            output[3 + (i / 2)] = static_cast<unsigned char>(low | (high << 4));
        }
    }

    bool DecodeFrame(const unsigned char* input, std::array<int16_t, VoiceChat::SamplesPerFrame>& pcm)
    {
        const int predictor = static_cast<int>(static_cast<int16_t>(input[0] | (static_cast<int>(input[1]) << 8)));
        const int stepIndex = input[2];
        if (stepIndex >= static_cast<int>(kStepTable.size()))
            return false;

        AdpcmState state { predictor, stepIndex };
        for (int i = 0; i < VoiceChat::SamplesPerFrame; i += 2)
        {
            const unsigned char packed = input[3 + (i / 2)];
            pcm[i] = DecodeSample(packed & 0x0F, state);
            pcm[i + 1] = DecodeSample((packed >> 4) & 0x0F, state);
        }
        return true;
    }

    bool ContainsVoiceActivity(const std::array<int16_t, VoiceChat::SamplesPerFrame>& pcm)
    {
        int64_t total = 0;
        for (const int16_t sample : pcm)
            total += std::abs(static_cast<int>(sample));
        return total / VoiceChat::SamplesPerFrame >= kVoiceActivityThreshold;
    }

    bool OpenCapture()
    {
        if (g_voice.Capture)
            return true;

        const SDL_AudioSpec spec = CreateVoiceSpec();
        g_voice.Capture = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, nullptr, nullptr);
        if (!g_voice.Capture)
            return false;

        SDL_ResumeAudioStreamDevice(g_voice.Capture);
        return true;
    }

    bool OpenPlayback()
    {
        if (g_voice.Playback)
            return true;

        const SDL_AudioSpec spec = CreateVoiceSpec();
        g_voice.Playback = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!g_voice.Playback)
            return false;

        SDL_ResumeAudioStreamDevice(g_voice.Playback);
        return true;
    }

    void CloseCapture()
    {
        if (!g_voice.Capture)
            return;
        SDL_DestroyAudioStream(g_voice.Capture);
        g_voice.Capture = nullptr;
    }

    void ClosePlayback()
    {
        if (!g_voice.Playback)
            return;
        SDL_DestroyAudioStream(g_voice.Playback);
        g_voice.Playback = nullptr;
    }

    void SendFrame(const std::array<unsigned char, VoiceChat::EncodedFrameBytes>& frame)
    {
        if (!SocketClient || !SocketClient->IsConnected())
            return;

        std::array<unsigned char, kOutgoingPacketBytes> packet {};
        packet[0] = 0xC1;
        packet[1] = static_cast<unsigned char>(packet.size());
        packet[2] = VoiceChat::PacketCode;
        packet[3] = VoiceChat::FrameSubCode;
        std::copy(frame.begin(), frame.end(), packet.begin() + kPacketHeaderBytes);
        SocketClient->Send(packet.data(), static_cast<int32_t>(packet.size()));
    }

    void CaptureAndSendFrames()
    {
        if (!g_voice.MicrophoneEnabled || !g_voice.Capture)
            return;

        while (SDL_GetAudioStreamAvailable(g_voice.Capture) >= kPcmBytesPerFrame)
        {
            std::array<int16_t, VoiceChat::SamplesPerFrame> pcm {};
            if (SDL_GetAudioStreamData(g_voice.Capture, pcm.data(), kPcmBytesPerFrame) != kPcmBytesPerFrame)
                return;
            if (!ContainsVoiceActivity(pcm))
                continue;

            std::array<unsigned char, VoiceChat::EncodedFrameBytes> frame {};
            EncodeFrame(pcm, frame);
            SendFrame(frame);
        }
    }
}

namespace VoiceChat
{
    void Initialize()
    {
        g_voice.ListeningEnabled = true;
    }

    void Shutdown()
    {
        g_voice.MicrophoneEnabled = false;
        CloseCapture();
        ClosePlayback();
    }

    void Update()
    {
        CaptureAndSendFrames();
    }

    void ToggleMicrophone()
    {
        if (g_voice.MicrophoneEnabled)
        {
            g_voice.MicrophoneEnabled = false;
            CloseCapture();
            return;
        }

        g_voice.MicrophoneEnabled = OpenCapture();
    }

    void ToggleListening()
    {
        g_voice.ListeningEnabled = !g_voice.ListeningEnabled;
        if (!g_voice.ListeningEnabled)
            ClosePlayback();
    }

    bool IsMicrophoneEnabled()
    {
        return g_voice.MicrophoneEnabled;
    }

    bool IsListeningEnabled()
    {
        return g_voice.ListeningEnabled;
    }

    void ReceiveFrame(const unsigned char* payload, std::size_t size)
    {
        if (!g_voice.ListeningEnabled || !payload || size != kIncomingPayloadBytes)
            return;
        if (!OpenPlayback())
            return;
        if (SDL_GetAudioStreamQueued(g_voice.Playback) >= kMaxQueuedPlaybackFrames * kPcmBytesPerFrame)
            return;

        std::array<int16_t, SamplesPerFrame> pcm {};
        const unsigned char* frame = payload + kServerSenderBytes;
        if (!DecodeFrame(frame, pcm))
            return;
        SDL_PutAudioStreamData(g_voice.Playback, pcm.data(), kPcmBytesPerFrame);
    }
}
