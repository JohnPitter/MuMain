#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include "Core/Platform/WinCompat.h"

class GameConfig
{
public:
    static GameConfig& GetInstance();

    void Load();
    void Save();

    // Window
    int  GetWindowWidth()  const { return m_windowWidth; }
    int  GetWindowHeight() const { return m_windowHeight; }
    bool GetWindowMode()   const { return m_windowMode; }

    void SetWindowSize(int width, int height);
    void SetWindowMode(bool windowed);

    // Audio — volume 0 = off, >0 = on. No separate Enabled flag.
    int  GetSoundVolume()  const { return m_soundVolume; }
    int  GetMusicVolume()  const { return m_musicVolume; }

    void SetSoundVolume(int level);
    void SetMusicVolume(int level);

    // Login
    bool GetRememberMe() const { return m_rememberMe; }
    void SetRememberMe(bool remember);

    // Whether the password (not just the username) may be persisted. Off by
    // default: "remember me" saves only the username unless the player opts in
    // on a machine they trust.
    bool GetSavePassword() const { return m_savePassword; }
    void SetSavePassword(bool save);

    // Drops the saved username and password from config.ini and revokes the
    // save-password consent. Used when the player edits the credentials.
    void ClearCredentials();

    std::wstring GetLanguageSelection() const { return m_languageSelection; }
    void SetLanguageSelection(const std::wstring& lang);

    void SetEncryptedUsername(const std::wstring& encryptedUsername);
    std::wstring GetEncryptedUsername() const { return m_encryptedUsername; }

    void SetEncryptedPassword(const std::wstring& encryptedPassword);
    std::wstring GetEncryptedPassword() const { return m_encryptedPassword; }

    // Connection
    std::wstring GetServerIP() const { return m_serverIP; }
    int GetServerPort() const { return m_serverPort; }

    void SetServerIP(const std::wstring& ip);
    void SetServerPort(int port);

    // UI — I18N locale code ("en", "de", ...) for the typed translation
    // accessors. Distinct from GetLanguageSelection above, which is the
    // legacy "Eng"/"Por"/"Spn" data-dir prefix used by .bmd asset loaders.
    std::wstring GetUILocale() const { return m_uiLocale; }
    void SetUILocale(const std::wstring& locale);

    // UI font family name (GDI face name). Empty = platform default.
    std::wstring GetFontSelection() const { return m_fontSelection; }
    void SetFontSelection(const std::wstring& font);

    // Bottom HUD toolbar at 1:1 reference size, centered. Default on.
    bool GetFixedToolbar() const { return m_fixedToolbar; }
    void SetFixedToolbar(bool fixed);

    float GetFixedToolbarScale() const { return m_fixedToolbarScale; }
    void SetFixedToolbarScale(float scale);

    // Chat commands - the favourites and the named templates of the command
    // window. They belong to the installation, not to a character.
    // A template is stored as "name|command|value|value|...".
    const std::vector<std::wstring>& GetChatCommandFavourites() const { return m_chatCommandFavourites; }
    void SetChatCommandFavourites(const std::vector<std::wstring>& favourites);

    const std::vector<std::wstring>& GetChatCommandTemplates() const { return m_chatCommandTemplates; }
    void SetChatCommandTemplates(const std::vector<std::wstring>& templates);

    // Cosmetic title drawn above the nameplate. 0 = follow Character.State / PK.
    int GetCharacterTitleId(const std::wstring& characterName) const;
    void SetCharacterTitleId(const std::wstring& characterName, int titleId);

    // Camera
    int GetZoom() const { return m_zoom; }
    void SetZoom(int zoom);

    // In-game Options window (see CNewUIOptionWindow). These are client-wide
    // preferences: persisted locally on every change and restored when the
    // options window is constructed at boot. AutoAttack/WhisperSound/SlideHelp
    // are additionally synced per-character to the game server via
    // SaveOptions() (ZzzOpenData.cpp) — the local copy covers the pre-login
    // state and clients that never receive the server's key-configuration
    // packet back.
    bool GetAutoAttack() const { return m_autoAttack; }
    void SetAutoAttack(bool autoAttack);

    bool GetWhisperSound() const { return m_whisperSound; }
    void SetWhisperSound(bool whisperSound);

    bool GetSlideHelp() const { return m_slideHelp; }
    void SetSlideHelp(bool slideHelp);

    int GetRenderLevel() const { return m_renderLevel; }
    void SetRenderLevel(int level);

    bool GetRenderAllEffects() const { return m_renderAllEffects; }
    void SetRenderAllEffects(bool renderAllEffects);

    // Helpers
    static std::wstring BinaryToHex(const BYTE* data, DWORD size);
    static std::vector<BYTE> HexToBinary(const std::wstring& hex);

    void DecryptCredentials(wchar_t* outUser, wchar_t* outPass, size_t userBufSize, size_t passBufSize);
    void EncryptAndSaveCredentials(const wchar_t* user, const wchar_t* pass);

private:
    GameConfig();
    GameConfig(const GameConfig&) = delete;
    GameConfig& operator=(const GameConfig&) = delete;

    std::filesystem::path m_configPath;

    int  m_windowWidth;
    int  m_windowHeight;
    bool m_windowMode;

    int  m_soundVolume;
    int  m_musicVolume;

    std::vector<std::wstring> m_chatCommandFavourites;
    std::vector<std::wstring> m_chatCommandTemplates;

    bool m_rememberMe;
    bool m_savePassword;
    std::wstring m_languageSelection;
    std::wstring m_encryptedUsername;
    std::wstring m_encryptedPassword;

    std::wstring m_serverIP;
    int m_serverPort;

    std::wstring m_uiLocale;
    std::wstring m_fontSelection;
    bool m_fixedToolbar;
    float m_fixedToolbarScale;

    int m_zoom;

    bool m_autoAttack;
    bool m_whisperSound;
    bool m_slideHelp;
    int  m_renderLevel;
    bool m_renderAllEffects;

    int ReadInt(const wchar_t* section, const wchar_t* key, int defaultValue);
    void WriteInt(const wchar_t* section, const wchar_t* key, int value);

    bool ReadBool(const wchar_t* section, const wchar_t* key, bool defaultValue);
    void WriteBool(const wchar_t* section, const wchar_t* key, bool value);

    std::vector<std::wstring> ReadStringList(const wchar_t* section, const wchar_t* keyPrefix);
    void WriteStringList(const wchar_t* section, const wchar_t* keyPrefix, const std::vector<std::wstring>& values);
    std::wstring ReadString(const wchar_t* section, const wchar_t* key, const std::wstring& defaultValue);
    void WriteString(const wchar_t* section, const wchar_t* key, const std::wstring& value);

    void RemoveObsoleteKey(const wchar_t* section, const wchar_t* key);
    void RemoveObsoleteSection(const wchar_t* section);

    std::wstring DecryptSetting(const std::wstring& hexInput);
    std::wstring EncryptSetting(const wchar_t* input);
};
