#pragma once

// Application-level radio API: owns the station list, the persisted
// [Radio] config and the engine lifecycle. The UI layer only talks to this
// namespace, never to the engine or WinHTTP directly.

#include "Audio/Radio/RadioEngine.h"

namespace Audio::Radio
{
    // Volume bounds on the UI's 0..100 scale.
    constexpr int kMinVolume = 0;
    constexpr int kMaxVolume = 100;
    constexpr int kDefaultVolume = 30;

    // Loads Data/Local/RadioStations.ini and the persisted [Radio] config.
    // Applies the saved station/volume; the radio itself stays OFF unless the
    // config explicitly says Enabled=1 (default off).
    void InitializeRadio();

    // Full stop + release. Called from DestroySound() before the mixer goes.
    void ShutdownRadio();

    int GetStationCount();
    const wchar_t* GetStationName(int index);

    int GetSelectedStation();
    void SelectStation(int index);      // persists; live-restarts if playing

    bool IsEnabled();
    void SetEnabled(bool on);           // starts/stops streaming; persists

    int GetVolume();                    // 0..100
    void SetVolume(int level);          // applies to the mixer track; persists

    // Thread-safe snapshot for the HUD label / config window.
    void GetStatus(RadioEngine::Snapshot& out);
}
