#pragma once

#include <cstddef>

namespace VoiceChat
{
    // The proximity voice transport uses 20 ms of 8 kHz mono PCM compressed with
    // IMA ADPCM. The server owns proximity checks; clients only capture, encode,
    // send and play frames forwarded by the GameServer.
    constexpr unsigned char PacketCode = 0xF4;
    constexpr unsigned char FrameSubCode = 0x20;
    constexpr int SampleRate = 8000;
    constexpr int SamplesPerFrame = 160;
    constexpr int EncodedFrameBytes = 83;

    void Initialize();
    void Shutdown();
    void Update();

    void ToggleMicrophone();
    void ToggleListening();
    bool IsMicrophoneEnabled();
    bool IsListeningEnabled();

    // Accepts the payload after the C1/F4/20 header: sender id (2 bytes) plus
    // a complete ADPCM frame. Invalid or muted frames are ignored.
    void ReceiveFrame(const unsigned char* payload, std::size_t size);
}
