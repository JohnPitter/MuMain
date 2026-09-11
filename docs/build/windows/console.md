# Windows - Terminal (CMake presets)

Native MSVC build from a **Developer Command Prompt / Developer PowerShell for
VS** (so `cl`, CMake, and Ninja are on `PATH`), using the bundled presets.

See [the build guide](../README.md) for shared concepts. To cross-compile the
Windows client from Linux/WSL instead, see [wsl.md](wsl.md).

## Prerequisites

- Visual Studio C++ build tools (CMake + Ninja).
- The **.NET 10 SDK**.

## Configure and build

```powershell
# Configure (pick one)
.\tools\cmake-windows.cmd --preset windows-x64                # 64-bit
.\tools\cmake-windows.cmd --preset windows-x64-mueditor       # 64-bit + editor
.\tools\cmake-windows.cmd --preset windows-x86                # 32-bit
.\tools\cmake-windows.cmd --preset windows-x86-mueditor       # 32-bit + editor

# Build (pick the matching Debug/Release build preset)
.\tools\cmake-windows.cmd --build --preset windows-x64-mueditor-debug
.\tools\cmake-windows.cmd --build --preset windows-x64-mueditor-release
```

The configure presets are listed in `CMakePresets.json`; each has
`-debug`/`-release` build presets.

The wrapper uses UTF-8 consistently for compiler detection and compilation,
then restores the terminal code page. Ninja/MSVC builds verify that the actual
`/showIncludes` prefix matches the configured prefix before compiling engine code.
A mismatch can otherwise silently omit header dependencies and link incompatible
versions of a character structure into one executable.

After a prefix mismatch or an old build with zero header dependencies, configure
and rebuild in a **new** directory with the wrapper; fixing the prefix alone does
not repair already stale objects. Before publishing, `ninja -t deps` must list
`w_CharacterInfo.h` for both `GIPetManager.cpp.obj` and `Winmain.cpp.obj`.

For a C++-only repair using a separately verified, compatible published connection
DLL, configure with `-DMU_BUILD_CLIENT_LIBRARY=OFF`. This skips Native AOT and its
packet source regeneration; provide that DLL separately when packaging. The
default remains `ON`. Run `test_character_startup_cleanup` to exercise the real
character/pet cleanup without launching a game window or connecting to a server.

## Run

```powershell
cd out/build/windows-x64-mueditor/src/Debug
.\Main.exe
```

Run from the build output's `src/<config>` directory so the client finds its
assets, `config.ini`, and `MUnique.Client.Library.dll` (all placed there by the
post-build step).
