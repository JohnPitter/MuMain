// Test-build shim for the radio parser library. The production stdafx.h
// (src/source/App/stdafx.h) drags in GLEW, SDL and the whole engine; the two
// radio parser translation units only need the CRT and the Win32 base. This
// shim directory is placed FIRST on this target's include path so
// `#include "stdafx.h"` resolves here instead. The client build is untouched.
#pragma once

#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <string>
