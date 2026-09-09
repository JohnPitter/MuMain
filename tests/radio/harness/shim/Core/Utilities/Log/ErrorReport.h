// Harness stand-in for the client's CErrorReport: same call surface the radio
// code uses (Write with %hs/%ls/%d/... formatting), tee'ed to stdout and a log
// file instead of MuError.log.
#pragma once

class CErrorReport
{
public:
    void Write(const wchar_t* format, ...);
    void AddSeparator() {}
};

extern CErrorReport g_ErrorReport;
