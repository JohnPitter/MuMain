// Shim for building the production RadioEngine.cpp inside the harness target.
// Resolves the client's stdafx.h include to plain Win32 + CRT basics, the same
// approach as tests/radio/stdafx_shim (that one can't be reused directly: the
// harness additionally links SDL, so it must not collide with it anyway).
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
