@echo off
setlocal
for /f "tokens=2 delims=:" %%c in ('chcp') do set "MU_PREVIOUS_CODEPAGE=%%c"
chcp 65001 >nul
if errorlevel 1 exit /b 1
cmake %*
set "MU_CMAKE_EXIT=%errorlevel%"
chcp %MU_PREVIOUS_CODEPAGE% >nul
exit /b %MU_CMAKE_EXIT%
