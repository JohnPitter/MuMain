#pragma once

namespace Core::Diagnostics
{
    void StartupCheckpoint(const wchar_t* stage);
    void RunStartup(void (*initialize)());
}
