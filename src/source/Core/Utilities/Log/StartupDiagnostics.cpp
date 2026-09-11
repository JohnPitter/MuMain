#include "stdafx.h"
#include "StartupDiagnostics.h"
#include "Core/Utilities/Log/ErrorReport.h"
#include <exception>

namespace
{
    thread_local const wchar_t* startupStage = L"not started";

#if defined(_WIN32) && defined(_MSC_VER)
    int LogStartupException(EXCEPTION_POINTERS* exception)
    {
        constexpr DWORD CppExceptionCode = 0xE06D7363;
        const auto* record = exception->ExceptionRecord;
        if (record->ExceptionCode == CppExceptionCode)
            return EXCEPTION_CONTINUE_SEARCH;

        HMODULE module = nullptr;
        const auto address = record->ExceptionAddress;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
            | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(address), &module);
        wchar_t modulePath[MAX_PATH]{};
        if (module != nullptr)
            GetModuleFileNameW(module, modulePath, MAX_PATH);
        const auto* basename = wcsrchr(modulePath, L'\\');
        const auto offset = reinterpret_cast<uintptr_t>(address) - reinterpret_cast<uintptr_t>(module);
        g_ErrorReport.Write(L"[Startup] fatal stage=%ls code=0x%08lX module=%ls rva=0x%llX\r\n",
            startupStage, record->ExceptionCode, basename ? basename + 1 : modulePath,
            static_cast<unsigned long long>(offset));
        return EXCEPTION_CONTINUE_SEARCH;
    }
#endif

    void RunWithCppLogging(void (*initialize)())
    {
        try
        {
            initialize();
        }
        catch (...)
        {
            g_ErrorReport.Write(L"[Startup] exception stage=%ls; propagating\r\n", startupStage);
            throw;
        }
    }
}

namespace Core::Diagnostics
{
    void StartupCheckpoint(const wchar_t* stage)
    {
        startupStage = stage;
        g_ErrorReport.Write(L"[Startup] %ls\r\n", stage);
    }

    void RunStartup(void (*initialize)())
    {
#if defined(_WIN32) && defined(_MSC_VER)
        __try
        {
            RunWithCppLogging(initialize);
        }
        __except (LogStartupException(GetExceptionInformation()))
        {
            // The filter always propagates; execution cannot resume after a fault.
        }
#else
        RunWithCppLogging(initialize);
#endif
    }
}
