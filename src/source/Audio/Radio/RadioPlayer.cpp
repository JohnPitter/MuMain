#include "stdafx.h"
#include "Audio/Radio/RadioPlayer.h"

#include <algorithm>

#include "Audio/Radio/RadioStationList.h"
#include "Core/Utilities/Log/ErrorReport.h"
#include "Data/GameConfig/GameConfig.h"

namespace
{
    std::vector<Audio::Radio::RadioStation> g_stations;
    int g_selectedStation = 0;
}

namespace Audio::Radio
{
    void InitializeRadio()
    {
        const std::wstring path = GetStationsFilePath();
        g_stations = LoadStationsFromFile(path);
        if (g_stations.empty())
        {
            g_ErrorReport.Write(L"[radio] no stations in %ls (owner-editable file)\r\n", path.c_str());
        }

        GameConfig& config = GameConfig::GetInstance();
        g_selectedStation = std::clamp(config.GetRadioStationIndex(), 0,
            std::max(0, static_cast<int>(g_stations.size()) - 1));
        RadioEngine::Instance().SetVolume(std::clamp(config.GetRadioVolume(), kMinVolume, kMaxVolume));

        // Default off: only come up when the config explicitly asks for it
        // and a station actually exists.
        if (config.GetRadioEnabled() && !g_stations.empty())
        {
            const RadioStation& station = g_stations[g_selectedStation];
            RadioEngine::Instance().Start(station.Url.c_str(), station.Name.c_str());
        }
    }

    void ShutdownRadio()
    {
        RadioEngine::Instance().Shutdown();
    }

    int GetStationCount()
    {
        return static_cast<int>(g_stations.size());
    }

    const wchar_t* GetStationName(int index)
    {
        if (index < 0 || index >= static_cast<int>(g_stations.size()))
        {
            return L"";
        }
        return g_stations[index].Name.c_str();
    }

    int GetSelectedStation()
    {
        return g_selectedStation;
    }

    void SelectStation(int index)
    {
        if (index < 0 || index >= static_cast<int>(g_stations.size()))
        {
            return;
        }

        g_selectedStation = index;
        GameConfig& config = GameConfig::GetInstance();
        config.SetRadioStationIndex(index);
        config.Save();  // setters only mutate; the caller persists (options-window pattern)

        if (IsEnabled())
        {
            // Live switch: switching stations while playing must not require
            // toggling the radio off and on again.
            const RadioStation& station = g_stations[index];
            RadioEngine::Instance().Start(station.Url.c_str(), station.Name.c_str());
        }
    }

    bool IsEnabled()
    {
        return GameConfig::GetInstance().GetRadioEnabled();
    }

    void SetEnabled(bool on)
    {
        GameConfig& config = GameConfig::GetInstance();
        config.SetRadioEnabled(on);
        config.Save();  // setters only mutate; the caller persists

        if (on && !g_stations.empty())
        {
            const RadioStation& station = g_stations[g_selectedStation];
            RadioEngine::Instance().Start(station.Url.c_str(), station.Name.c_str());
        }
        else if (!on)
        {
            RadioEngine::Instance().Stop();
        }
    }

    int GetVolume()
    {
        return std::clamp(GameConfig::GetInstance().GetRadioVolume(), kMinVolume, kMaxVolume);
    }

    void SetVolume(int level)
    {
        const int clamped = std::clamp(level, kMinVolume, kMaxVolume);
        GameConfig& config = GameConfig::GetInstance();
        config.SetRadioVolume(clamped);
        config.Save();  // setters only mutate; the caller persists
        RadioEngine::Instance().SetVolume(clamped);
    }

    void GetStatus(RadioEngine::Snapshot& out)
    {
        RadioEngine::Instance().GetStatus(out);
    }
}
