#pragma once

namespace CfgSections
{
    inline constexpr wchar_t CfgSectionWindow[]     = L"Window";
    inline constexpr wchar_t CfgSectionGraphics[]   = L"Graphics";
    inline constexpr wchar_t CfgSectionAudio[]      = L"Audio";
    inline constexpr wchar_t CfgSectionUI[]         = L"UI";
    inline constexpr wchar_t CfgSectionLogin[]      = L"LOGIN";
    inline constexpr wchar_t CfgSectionConnectionSettings[] = L"CONNECTION SETTINGS";
    inline constexpr wchar_t CfgSectionCamera[] = L"Camera";
    inline constexpr wchar_t CfgSectionRender[] = L"Render";
    // In-game Options window toggles/sliders that are persisted nowhere else
    // (volumes live in "Audio", window state in "Window").
    inline constexpr wchar_t CfgSectionOptions[] = L"Options";
}

namespace CfgKeys
{
    // Window
    inline constexpr wchar_t CfgKeyWidth[]      = L"Width";
    inline constexpr wchar_t CfgKeyHeight[]     = L"Height";
    inline constexpr wchar_t CfgKeyWindowed[]   = L"Windowed";

    // Audio — volume 0 = off, >0 = on (no separate Enabled flag).
    inline constexpr wchar_t CfgKeySoundVolume[]  = L"SoundVolume";
    inline constexpr wchar_t CfgKeyMusicVolume[] = L"MusicVolume";

    // Login
    inline constexpr wchar_t CfgKeyRememberMe[]        = L"RememberMe";
    inline constexpr wchar_t CfgKeySavePassword[]      = L"SavePassword";
    inline constexpr wchar_t CfgKeyLanguage[]          = L"Language";
    inline constexpr wchar_t CfgKeyEncryptedUsername[] = L"EncryptedUsername";
    inline constexpr wchar_t CfgKeyEncryptedPassword[] = L"EncryptedPassword";

    // Connection
    inline constexpr wchar_t CfgKeyServerIP[]   = L"ServerIP";
    inline constexpr wchar_t CfgKeyServerPort[] = L"ServerPort";

    // UI
    inline constexpr wchar_t CfgKeyUILocale[] = L"Locale";
    inline constexpr wchar_t CfgKeyFont[]     = L"Font";
    inline constexpr wchar_t CfgKeyFixedToolbar[] = L"FixedToolbar";
    inline constexpr wchar_t CfgKeyFixedToolbarScale[] = L"FixedToolbarScale";
    inline constexpr wchar_t CfgKeyFixedToolbarLayoutVersion[] = L"FixedToolbarLayoutVersion";

    // Camera
    inline constexpr wchar_t CfgKeyZoom[] = L"Zoom";

    // Options (in-game Options window)
    inline constexpr wchar_t CfgKeyAutoAttack[]       = L"AutoAttack";
    inline constexpr wchar_t CfgKeyWhisperSound[]     = L"WhisperSound";
    inline constexpr wchar_t CfgKeySlideHelp[]        = L"SlideHelp";
    inline constexpr wchar_t CfgKeyRenderLevel[]      = L"RenderLevel";
    inline constexpr wchar_t CfgKeyRenderAllEffects[] = L"RenderAllEffects";

    // Render
    // DXP-08: Core Profile GL context flip. 0 = compatibility (rollback), 1 = core.
    inline constexpr wchar_t CfgKeyCoreProfile[] = L"CoreProfile";
    // GLP-08: ceiling on the requested core-profile GL context version, e.g. "4.3". Empty
    // (default) tries the highest of {4.5, 4.3, 3.3} the driver will grant. Rollback path for a
    // driver that mishandles the descending attempt loop.
    inline constexpr wchar_t CfgKeyMaxGLVersion[] = L"MaxGLVersion";
}

namespace CfgDefaults
{
    inline constexpr int  CfgDefaultWindowWidth  = 1024;
    inline constexpr int  CfgDefaultWindowHeight = 768;
    inline constexpr bool CfgDefaultWindowed     = true;

    inline constexpr int  CfgDefaultSoundVolume = 5;
    inline constexpr int  CfgDefaultMusicVolume = 5;

    inline constexpr bool CfgDefaultRememberMe = false;
    inline constexpr bool CfgDefaultSavePassword = false;
    inline constexpr wchar_t CfgDefaultLanguage[] = L"Eng";
    inline constexpr wchar_t CfgDefaultEncryptedUsername[] = L"";
    inline constexpr wchar_t CfgDefaultEncryptedPassword[] = L"";

    inline constexpr wchar_t CfgDefaultServerIP[] = L"127.127.127.127";
    inline constexpr int CfgDefaultServerPort = 44406;

    inline constexpr int CfgDefaultZoom = 1735;  // OrbitalCamera DEFAULT_RADIUS — matches Default-cam camera-to-Hero distance

    // I18N locale code; PT-BR is the default for this client build.
    inline constexpr wchar_t CfgDefaultUILocale[] = L"pt";

    // UI font family name. Empty = each platform's built-in default (Tahoma on
    // Windows, fontconfig "sans-serif" on Linux), so the look is unchanged until
    // the user picks a font. Any value is passed through as the GDI face name.
    inline constexpr wchar_t CfgDefaultFont[] = L"";

    // The classic HUD fills the bottom of the window. The fixed-width variant
    // is still available as an explicit configuration for compatible layouts.
    inline constexpr bool CfgDefaultFixedToolbar = false;
    inline constexpr float CfgDefaultFixedToolbarScale = 1.25f;
    inline constexpr int CfgCurrentFixedToolbarLayoutVersion = 1;

    // DXP-08 Stage G: flipped to default-on after DXP-08a/DXP-09 prerequisites were fixed and
    // soak-confirmed clean under CoreProfile=1 (2026-08-01). Set CoreProfile=0 in config.ini to
    // opt back into the compatibility-profile rollback path.
    inline constexpr bool CfgDefaultCoreProfile = true;

    // GLP-08: empty = no cap, try the highest core context available.
    inline constexpr wchar_t CfgDefaultMaxGLVersion[] = L"";

    // In-game Options window defaults. These mirror the values the options
    // window used to hardcode in its constructor before local persistence
    // existed, so a fresh install behaves exactly as before.
    inline constexpr bool CfgDefaultAutoAttack       = true;
    inline constexpr bool CfgDefaultWhisperSound     = false;
    inline constexpr bool CfgDefaultSlideHelp        = true;
    // "+Effect limitation" slider: 0..5. Runtime consumers map it to effect
    // detail (e.g. ZzzObject GetPipeline caps at RenderLevel*2+5).
    inline constexpr int  CfgDefaultRenderLevel      = 4;
    inline constexpr bool CfgDefaultRenderAllEffects = true;
}

namespace CfgLimits
{
    // Slider ranges of the Options window. Sliders (volume 0..10, effect
    // limitation 0..5) and Load()-time sanitizing of config.ini values share
    // these pure clamps so a hand-edited or truncated ini can never push an
    // out-of-range level into the renderer or the audio mixer.
    inline constexpr int MaxVolumeLevel = 10;
    inline constexpr int MaxRenderLevel = 5;

    inline int ClampVolumeLevel(int level)
    {
        return level < 0 ? 0 : (level > MaxVolumeLevel ? MaxVolumeLevel : level);
    }

    inline int ClampRenderLevel(int level)
    {
        return level < 0 ? 0 : (level > MaxRenderLevel ? MaxRenderLevel : level);
    }
}
