#include "stdafx.h"
#include "doctest.h"
#include "Core/Utilities/Log/StartupDiagnostics.h"
#include "Core/Utilities/Log/ErrorReport.h"
#include <cstdarg>
#include <stdexcept>
#include <string>

namespace
{
    std::wstring recorded;
    int calls = 0;
    void SuccessfulStartup() { ++calls; }
    void FailingStartup() { throw std::runtime_error("test failure"); }
    void NativeFailure() { RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr); }

    bool ObserveNativeFailure()
    {
        __try { Core::Diagnostics::RunStartup(NativeFailure); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return true; }
        return false;
    }
}

CErrorReport g_ErrorReport;
CErrorReport::CErrorReport() : m_hFile(nullptr), m_lpszFileName{}, m_iKey(0) {}
CErrorReport::~CErrorReport() = default;
void CErrorReport::Write(const wchar_t* format, ...)
{
    wchar_t buffer[1024]{};
    va_list arguments;
    va_start(arguments, format);
    vswprintf(buffer, 1024, format, arguments);
    va_end(arguments);
    recorded += buffer;
}

TEST_CASE("Startup diagnostics runs initialization exactly once")
{
    calls = 0;
    recorded.clear();
    Core::Diagnostics::StartupCheckpoint(L"login.begin");
    Core::Diagnostics::RunStartup(SuccessfulStartup);
    CHECK(calls == 1);
    CHECK(recorded == L"[Startup] login.begin\r\n");
}

TEST_CASE("Startup diagnostics logs the latest phase and propagates C++ failure")
{
    recorded.clear();
    Core::Diagnostics::StartupCheckpoint(L"world.clear-characters");
    CHECK_THROWS_AS(Core::Diagnostics::RunStartup(FailingStartup), std::runtime_error);
    CHECK(recorded.find(L"exception stage=world.clear-characters") != std::wstring::npos);
    CHECK(recorded.find(L"test failure") == std::wstring::npos);
}

TEST_CASE("Startup diagnostics records native failure without swallowing it")
{
    recorded.clear();
    Core::Diagnostics::StartupCheckpoint(L"world.load-models");
    CHECK(ObserveNativeFailure());
    CHECK(recorded.find(L"fatal stage=world.load-models code=0xC0000005") != std::wstring::npos);
    CHECK(recorded.find(L" rva=0x") != std::wstring::npos);
}
