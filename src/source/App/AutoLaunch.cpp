//*****************************************************************************
// File: AutoLaunch.cpp
//*****************************************************************************

#include "stdafx.h"
#include "AutoLaunch.h"

namespace LauncherBoot
{
namespace
{
    constexpr wchar_t kServerEnv[] = L"LUXVIEW_MU_SERVER";
    constexpr wchar_t kAccountEnv[] = L"LUXVIEW_MU_ACCOUNT";
    constexpr wchar_t kPasswordEnv[] = L"LUXVIEW_MU_PASSWORD";
    constexpr int kMaxServerId = 60000;

    bool g_initialized = false;
    int g_autoServerId = -1;
    wchar_t g_autoAccount[MAX_USERNAME_SIZE + 1] = {};
    wchar_t g_autoPassword[MAX_PASSWORD_SIZE + 1] = {};

    // Captures an environment variable and immediately removes it so helper
    // processes spawned by the client never inherit the credentials.
    void CaptureEnv(const wchar_t* name, wchar_t* output, size_t outputSize)
    {
        output[0] = L'\0';
        GetEnvironmentVariableW(name, output, static_cast<DWORD>(outputSize));
        SetEnvironmentVariableW(name, NULL);
    }
}

void Initialize()
{
    if (g_initialized)
        return;

    g_initialized = true;

    wchar_t serverText[32] = {};
    CaptureEnv(kServerEnv, serverText, _countof(serverText));
    if (serverText[0] != L'\0')
    {
        const int serverId = _wtoi(serverText);
        if (serverId >= 0 && serverId < kMaxServerId)
            g_autoServerId = serverId;
    }

    wchar_t account[64] = {};
    wchar_t password[64] = {};
    CaptureEnv(kAccountEnv, account, _countof(account));
    CaptureEnv(kPasswordEnv, password, _countof(password));
    if (account[0] != L'\0' && password[0] != L'\0')
    {
        wcscpy_s(g_autoAccount, _countof(g_autoAccount), account);
        wcscpy_s(g_autoPassword, _countof(g_autoPassword), password);
    }
}

bool HasAutoServer()
{
    return g_autoServerId >= 0;
}

int GetAutoServerId()
{
    return g_autoServerId;
}

void ConsumeAutoServer()
{
    g_autoServerId = -1;
}

bool HasAutoLogin()
{
    return g_autoAccount[0] != L'\0' && g_autoPassword[0] != L'\0';
}

const wchar_t* GetAutoLoginAccount()
{
    return g_autoAccount;
}

const wchar_t* GetAutoLoginPassword()
{
    return g_autoPassword;
}

void DisableAutoLogin()
{
    g_autoAccount[0] = L'\0';
    g_autoPassword[0] = L'\0';
}
}
