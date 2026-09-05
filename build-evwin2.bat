@echo on
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
cd /d "C:\Users\joaop\Desenvolvimento\openmu\MuMain"
cmake --build --preset windows-x64-release
if not "%errorlevel%"=="0" exit /b 3
echo BUILD_OK
