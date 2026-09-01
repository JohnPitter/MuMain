//*****************************************************************************
// File: AutoLaunch.h
//*****************************************************************************
// Launcher-driven startup: the LuxView launcher passes the pre-selected game
// server and account credentials through environment variables so the client
// can skip the server picker and login window and land on character select.
//
// AutoLogin credentials are reused when the player changes GameServer from the
// in-game menu (the picker still appears because AutoServer is consumed).
// They are cleared only on Exit Game. Cold start without launcher env still
// shows the login form.
//*****************************************************************************

#pragma once

namespace LauncherBoot
{
    // Reads the launcher environment once; call early in WinMain.
    void Initialize();

    // Pre-selected game server id (ConnectServer index); -1 when absent.
    bool HasAutoServer();
    int GetAutoServerId();
    void ConsumeAutoServer();

    // Launcher account credentials; empty when absent.
    bool HasAutoLogin();
    const wchar_t* GetAutoLoginAccount();
    const wchar_t* GetAutoLoginPassword();
    void DisableAutoLogin();
}
